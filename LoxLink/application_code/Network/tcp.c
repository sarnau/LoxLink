#include "tcp.h"
#ifdef WITH_TCP

#include "ip.h"
#include "lan.h"
#include "stm32f1xx_hal.h"


#define TCP_WINDOW_SIZE 65535
#define TCP_SYN_MSS 512
#ifdef WITH_TCP_REXMIT
#define TCP_REXMIT_TIMEOUT 1000
#define TCP_REXMIT_LIMIT 5
#else
#define TCP_CONN_TIMEOUT 2500
#endif


#define TCP_FLAG_URG 0x20
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_FIN 0x01

typedef enum tcp_status_code {
  TCP_CLOSED,
  TCP_SYN_SENT,
  TCP_SYN_RECEIVED,
  TCP_ESTABLISHED,
  TCP_FIN_WAIT
} tcp_status_code_t;

typedef struct tcp_state {
  tcp_status_code_t status;
  uint32_t event_time;
  uint32_t seq_num;
  uint32_t ack_num;
  uint32_t remote_addr;
  uint16_t remote_port;
  uint16_t local_port;
#ifdef WITH_TCP_REXMIT
  uint8_t is_closing;
  uint8_t rexmit_count;
  uint32_t seq_num_saved;
#endif
} tcp_state_t;

typedef enum tcp_sending_mode {
  TCP_SENDING_SEND,
  TCP_SENDING_REPLY,
  TCP_SENDING_RESEND
} tcp_sending_mode_t;

// TCP connection pool
tcp_state_t tcp_pool[TCP_MAX_CONNECTIONS];

/***
 *  TCP (ver. 3.0)
 *  lots of indian bydlocode here
 *
 *  History:
 *      1.0 first attempt
 *      2.0 second attempt, first suitable working variant
 *      2.1 added normal seq/ack management
 *      3.0 added rexmit feature
 ***/

// packet sending mode
static tcp_sending_mode_t tcp_send_mode;

// "ack sent" flag
static uint8_t tcp_ack_sent;

/***
 *  send TCP packet
 *  must be set manually:
 *  - tcp.flags
 ***/
uint8_t tcp_xmit(tcp_state_t *st, eth_frame_t *frame, uint16_t len) {
  uint8_t status = 1;
  uint16_t temp, plen = len;

  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  tcp_packet_t *tcp = (tcp_packet_t *)(ip->data);

  if (tcp_send_mode == TCP_SENDING_SEND) {
    // set packet fields
    ip->to_addr = st->remote_addr;
    ip->from_addr = gLan_ip_addr;
    ip->protocol = IP_PROTOCOL_TCP;
    tcp->to_port = st->remote_port;
    tcp->from_port = st->local_port;
  }

  if (tcp_send_mode == TCP_SENDING_REPLY) {
    // exchange src/dst ports
    temp = tcp->from_port;
    tcp->from_port = tcp->to_port;
    tcp->to_port = temp;
  }

  if (tcp_send_mode != TCP_SENDING_RESEND) {
    // fill packet header ("static" fields)
    tcp->window = htons(TCP_WINDOW_SIZE);
    tcp->urgent_ptr = 0;
  }

  if (tcp->flags & TCP_FLAG_SYN) {
    // add MSS option (max. segment size)
    tcp->data_offset = (sizeof(tcp_packet_t) + 4) << 2;
    tcp->data[0] = 2; //option: MSS
    tcp->data[1] = 4; //option len
    tcp->data[2] = TCP_SYN_MSS >> 8;
    tcp->data[3] = TCP_SYN_MSS & 0xff;
    plen = 4;
  } else {
    tcp->data_offset = sizeof(tcp_packet_t) << 2;
  }

  // set stream pointers
  tcp->seq_num = htonl(st->seq_num);
  tcp->ack_num = htonl(st->ack_num);

  // set checksum
  plen += sizeof(tcp_packet_t);
  tcp->cksum = 0;
  tcp->cksum = ip_cksum(plen + IP_PROTOCOL_TCP,
    (uint8_t *)tcp - 8, plen + 8);

  // send packet
  switch (tcp_send_mode) {
  case TCP_SENDING_SEND:
    status = ip_send(frame, plen);
    tcp_send_mode = TCP_SENDING_RESEND;
    break;
  case TCP_SENDING_REPLY:
    ip_reply(frame, plen);
    tcp_send_mode = TCP_SENDING_RESEND;
    break;
  case TCP_SENDING_RESEND:
    ip_resend(frame, plen);
    break;
  }

  // advance sequence number
  st->seq_num += len;
  if ((tcp->flags & TCP_FLAG_SYN) || (tcp->flags & TCP_FLAG_FIN))
    st->seq_num++;

  // set "ACK sent" flag
  if ((tcp->flags & TCP_FLAG_ACK) && (status))
    tcp_ack_sent = 1;

  return status;
}

/***
 *  sending SYN to peer
 *  return: 0xff - error, other value - connection id (not established)
 ***/
uint8_t tcp_open(uint32_t addr, uint16_t port, uint16_t local_port) {
  eth_frame_t *frame = (eth_frame_t *)gLan_net_buf;
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  tcp_packet_t *tcp = (tcp_packet_t *)(ip->data);
  tcp_state_t *st = 0, *pst;
  uint8_t id;
  uint32_t seq_num;

  // search for free conection slot
  for (id = 0; id < TCP_MAX_CONNECTIONS; ++id) {
    pst = tcp_pool + id;

    if (pst->status == TCP_CLOSED) {
      st = pst;
      break;
    }
  }

  // free connection slot found
  if (st) {
    // add new connection
    seq_num = HAL_GetTick() + (HAL_GetTick() << 16);

    st->status = TCP_SYN_SENT;
    st->event_time = HAL_GetTick();
    st->seq_num = seq_num;
    st->ack_num = 0;
    st->remote_addr = addr;
    st->remote_port = port;
    st->local_port = local_port;

#ifdef WITH_TCP_REXMIT
    st->is_closing = 0;
    st->rexmit_count = 0;
    st->seq_num_saved = seq_num;
#endif

    // send packet
    tcp_send_mode = TCP_SENDING_SEND;
    tcp->flags = TCP_FLAG_SYN;
    if (tcp_xmit(st, frame, 0))
      return id;

    st->status = TCP_CLOSED;
  }

  return 0xff;
}

/***
 *  send TCP data
 *  don't use anywhere except tcp_write callback!
 ***/
void tcp_send(uint8_t id, eth_frame_t *frame, uint16_t len, uint8_t options) {
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  tcp_packet_t *tcp = (tcp_packet_t *)(ip->data);
  tcp_state_t *st = tcp_pool + id;
  uint8_t flags = TCP_FLAG_ACK;

  // check if connection established
  if (st->status != TCP_ESTABLISHED)
    return;

  // send PSH/ACK
  if (options & TCP_OPTION_PUSH)
    flags |= TCP_FLAG_PSH;

  // send FIN/ACK
  if (options & TCP_OPTION_CLOSE) {
    flags |= TCP_FLAG_FIN;
    st->status = TCP_FIN_WAIT;
  }

  // send packet
  tcp->flags = flags;
  tcp_xmit(st, frame, len);
}

/***
 *  processing tcp packets
 ***/
void tcp_filter(eth_frame_t *frame, uint16_t len) {
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  tcp_packet_t *tcp = (tcp_packet_t *)(ip->data);
  tcp_state_t *st = 0, *pst;
  uint8_t id, tcpflags;

  if (ip->to_addr != gLan_ip_addr)
    return;

  // tcp data length
  len -= tcp_head_size(tcp);

  // me needs only SYN/FIN/ACK/RST
  tcpflags = tcp->flags & (TCP_FLAG_SYN | TCP_FLAG_ACK |
                            TCP_FLAG_RST | TCP_FLAG_FIN);

  // sending packets back
  tcp_send_mode = TCP_SENDING_REPLY;
  tcp_ack_sent = 0;

  // search connection pool for connection
  //      to specific port from specific host/port
  for (id = 0; id < TCP_MAX_CONNECTIONS; ++id) {
    pst = tcp_pool + id;

    if ((pst->status != TCP_CLOSED) &&
        (ip->from_addr == pst->remote_addr) &&
        (tcp->from_port == pst->remote_port) &&
        (tcp->to_port == pst->local_port)) {
      st = pst;
      break;
    }
  }

  // connection not found/new connection
  if (!st) {
    // received SYN - initiating new connection
    if (tcpflags == TCP_FLAG_SYN) {
      // search for free slot for connection
      for (id = 0; id < TCP_MAX_CONNECTIONS; ++id) {
        pst = tcp_pool + id;

        if (pst->status == TCP_CLOSED) {
          st = pst;
          break;
        }
      }

      // slot found and app accepts connection?
      if (st && tcp_listen(id, frame)) {
        // add embrionic connection to pool
        st->status = TCP_SYN_RECEIVED;
        st->event_time = HAL_GetTick();
        st->seq_num = HAL_GetTick() + (HAL_GetTick() << 16);
        st->ack_num = ntohl(tcp->seq_num) + 1;
        st->remote_addr = ip->from_addr;
        st->remote_port = tcp->from_port;
        st->local_port = tcp->to_port;

#ifdef WITH_TCP_REXMIT
        st->is_closing = 0;
        st->rexmit_count = 0;
        st->seq_num_saved = st->seq_num;
#endif

        // send SYN/ACK
        tcp->flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
        tcp_xmit(st, frame, 0);
      }
    }
  }

  else {
    // connection reset by peer?
    if (tcpflags & TCP_FLAG_RST) {
      if ((st->status == TCP_ESTABLISHED) ||
          (st->status == TCP_FIN_WAIT)) {
        tcp_closed(id, 1);
      }
      st->status = TCP_CLOSED;
      return;
    }

    // me needs only ack packet
    if ((ntohl(tcp->seq_num) != st->ack_num) ||
        (ntohl(tcp->ack_num) != st->seq_num) ||
        (!(tcpflags & TCP_FLAG_ACK))) {
      return;
    }

#ifdef WITH_TCP_REXMIT
    // save sequence number
    st->seq_num_saved = st->seq_num;

    // reset rexmit counter
    st->rexmit_count = 0;
#endif

    // update ack pointer
    st->ack_num += len;
    if ((tcpflags & TCP_FLAG_FIN) || (tcpflags & TCP_FLAG_SYN))
      st->ack_num++;

    // reset timeout counter
    st->event_time = HAL_GetTick();

    switch (st->status) {

    // SYN sent by me (active open, step 1)
    // awaiting SYN/ACK (active open, step 2)
    case TCP_SYN_SENT:

      // received packet must be SYN/ACK
      if (tcpflags != (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
        st->status = TCP_CLOSED;
        break;
      }

      // send ACK (active open, step 3)
      tcp->flags = TCP_FLAG_ACK;
      tcp_xmit(st, frame, 0);

      // connection is now established
      st->status = TCP_ESTABLISHED;

      // app can send some data
      tcp_read(id, frame, 0);

      break;

    // SYN received my me (passive open, step 1)
    // SYN/ACK sent by me (passive open, step 2)
    // awaiting ACK (passive open, step 3)
    case TCP_SYN_RECEIVED:

      // received packet must be ACK
      if (tcpflags != TCP_FLAG_ACK) {
        st->status = TCP_CLOSED;
        break;
      }

      // connection is now established
      st->status = TCP_ESTABLISHED;

      // app can send some data
      tcp_read(id, frame, 0);

      break;

    // connection established
    // awaiting ACK or FIN/ACK
    case TCP_ESTABLISHED:

      // received FIN/ACK?
      // (passive close, step 1)
      if (tcpflags == (TCP_FLAG_FIN | TCP_FLAG_ACK)) {
        // feed data to app
        if (len)
          tcp_write(id, frame, len);

        // send FIN/ACK (passive close, step 2)
        tcp->flags = TCP_FLAG_FIN | TCP_FLAG_ACK;
        tcp_xmit(st, frame, 0);

        // connection is now closed
        st->status = TCP_CLOSED;
        tcp_closed(id, 0);
      }

      // received ACK
      else if (tcpflags == TCP_FLAG_ACK) {
        // feed data to app
        if (len)
          tcp_write(id, frame, len);

        // app can send some data
        tcp_read(id, frame, 0);

        // send ACK
        if ((len) && (!tcp_ack_sent)) {
          tcp->flags = TCP_FLAG_ACK;
          tcp_xmit(st, frame, 0);
        }
      }

      break;

    // FIN/ACK sent by me (active close, step 1)
    // awaiting ACK or FIN/ACK
    case TCP_FIN_WAIT:

      // received FIN/ACK?
      // (active close, step 2)
      if (tcpflags == (TCP_FLAG_FIN | TCP_FLAG_ACK)) {
        // feed data to app
        if (len)
          tcp_write(id, frame, len);

        // send ACK (active close, step 3)
        tcp->flags = TCP_FLAG_ACK;
        tcp_xmit(st, frame, 0);

        // connection is now closed
        st->status = TCP_CLOSED;
        tcp_closed(id, 0);
      }

      // received ACK+data?
      // (buffer flushing by peer)
      else if ((tcpflags == TCP_FLAG_ACK) && (len)) {
        // feed data to app
        tcp_write(id, frame, len);

        // send ACK
        tcp->flags = TCP_FLAG_ACK;
        tcp_xmit(st, frame, 0);

#ifdef WITH_TCP_REXMIT
        // our data+FIN/ACK acked
        st->is_closing = 1;
#endif
      }

      break;

    default:
      break;
    }
  }
}

/***
 *  periodic event
 ***/
void tcp_poll(void) {
  COMPILE_CHECK(sizeof(tcp_packet_t) == 20);

#ifdef WITH_TCP_REXMIT
  eth_frame_t *frame = (eth_frame_t *)gLan_net_buf;
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  tcp_packet_t *tcp = (tcp_packet_t *)(ip->data);
#endif

  uint8_t id;
  tcp_state_t *st;

  for (id = 0; id < TCP_MAX_CONNECTIONS; ++id) {
    st = tcp_pool + id;

#ifdef WITH_TCP_REXMIT
    // connection timed out?
    if ((st->status != TCP_CLOSED) &&
        (HAL_GetTick() - st->event_time > TCP_REXMIT_TIMEOUT)) {
      // rexmit limit reached?
      if (st->rexmit_count > TCP_REXMIT_LIMIT) {
        // close connection
        st->status = TCP_CLOSED;
        tcp_closed(id, 1);
      }

      // should rexmit?
      else {
        // reset timeout counter
        st->event_time = HAL_GetTick();

        // increment rexmit counter
        st->rexmit_count++;

        // load previous state
        st->seq_num = st->seq_num_saved;

        // will send packets
        tcp_send_mode = TCP_SENDING_SEND;
        tcp_ack_sent = 0;

        // rexmit
        switch (st->status) {
        // rexmit SYN
        case TCP_SYN_SENT:
          tcp->flags = TCP_FLAG_SYN;
          tcp_xmit(st, frame, 0);
          break;

        // rexmit SYN/ACK
        case TCP_SYN_RECEIVED:
          tcp->flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
          tcp_xmit(st, frame, 0);
          break;

        // rexmit data+FIN/ACK or ACK (in FIN_WAIT state)
        case TCP_FIN_WAIT:

          // data+FIN/ACK acked?
          if (st->is_closing) {
            tcp->flags = TCP_FLAG_ACK;
            tcp_xmit(st, frame, 0);
            break;
          }

          // rexmit data+FIN/ACK
          st->status = TCP_ESTABLISHED;

        // rexmit data
        case TCP_ESTABLISHED:
          tcp_read(id, frame, 1);
          if (!tcp_ack_sent) {
            tcp->flags = TCP_FLAG_ACK;
            tcp_xmit(st, frame, 0);
          }
          break;

        default:
          break;
        }
      }
    }
#else
    // check if connection timed out
    if ((st->status != TCP_CLOSED) &&
        (HAL_GetTick() - st->event_time > TCP_CONN_TIMEOUT)) {
      // kill connection
      st->status = TCP_CLOSED;
      tcp_closed(id, 1);
    }
#endif
  }
}

#endif