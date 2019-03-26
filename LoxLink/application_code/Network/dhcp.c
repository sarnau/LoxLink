#include "dhcp.h"
#include "lan.h"
#include "stm32f1xx_hal.h" // HAL_GetUID() and HAL_GetTick()
#include "udp.h"
#include <string.h>

#ifdef WITH_DHCP

#define DHCP_SERVER_PORT htons(67)
#define DHCP_CLIENT_PORT htons(68)

#define DHCP_OP_REQUEST 1
#define DHCP_OP_REPLY 2

#define DHCP_HW_ADDR_TYPE_ETH 1

#define DHCP_FLAG_BROADCAST htons(0x8000)

#define DHCP_MAGIC_COOKIE htonl(0x63825363)

typedef struct dhcp_message {
  uint8_t operation;
  uint8_t hw_addr_type;
  uint8_t hw_addr_len;
  uint8_t unused1;
  uint32_t transaction_id;
  uint16_t second_count;
  uint16_t flags;
  uint32_t client_addr;
  uint32_t offered_addr;
  uint32_t server_addr;
  uint32_t unused2;
  uint8_t hw_addr[16];
  uint8_t unused3[192];
  uint32_t magic_cookie;
  uint8_t options[];
} dhcp_message_t;

#define DHCP_CODE_PAD 0
#define DHCP_CODE_SUBNETMASK 1
#define DHCP_CODE_GATEWAY 3
#define DHCP_CODE_HOSTNAME 12
#define DHCP_CODE_REQUESTEDADDR 50
#define DHCP_CODE_LEASETIME 51
#define DHCP_CODE_MESSAGETYPE 53
#define DHCP_CODE_DHCPSERVER 54
#define DHCP_CODE_RENEWTIME 58
#define DHCP_CODE_REBINDTIME 59
#define DHCP_CODE_END 255

typedef struct dhcp_option {
  uint8_t code;
  uint8_t len;
  uint8_t data[];
} dhcp_option_t;

#define DHCP_MESSAGE_DISCOVER 1
#define DHCP_MESSAGE_OFFER 2
#define DHCP_MESSAGE_REQUEST 3
#define DHCP_MESSAGE_DECLINE 4
#define DHCP_MESSAGE_ACK 5
#define DHCP_MESSAGE_NAK 6
#define DHCP_MESSAGE_RELEASE 7
#define DHCP_MESSAGE_INFORM 8

typedef enum dhcp_status_code {
  DHCP_INIT,
  DHCP_ASSIGNED,
  DHCP_WAITING_OFFER,
  DHCP_WAITING_ACK
} dhcp_status_code_t;

dhcp_status_code_t dhcp_status;
static uint32_t dhcp_server;
static uint32_t dhcp_renew_time;
static uint32_t dhcp_retry_time;
static uint32_t dhcp_transaction_id;
uint32_t gLAN_IPv4_address;
uint32_t gLAN_IPv4_subnet_mask;
uint32_t gLan_IPv4_gateway;

#define DHCP_HOSTNAME_MAX_LEN 24
static char dhcp_hostname[DHCP_HOSTNAME_MAX_LEN] = "STM32-";


#define dhcp_add_option(ptr, optcode, type, value) \
  ((dhcp_option_t *)ptr)->code = optcode;          \
  ((dhcp_option_t *)ptr)->len = sizeof(type);      \
  *(type *)(((dhcp_option_t *)ptr)->data) = value; \
  ptr += sizeof(dhcp_option_t) + sizeof(type);     \
  if (sizeof(type) & 1)                            \
    *(ptr++) = 0;

void dhcp_filter(eth_frame_t *frame, uint16_t len) {
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  udp_packet_t *udp = (udp_packet_t *)(ip->data);
  dhcp_message_t *dhcp = (dhcp_message_t *)(udp->data);
  uint32_t offered_net_mask = 0, offered_gateway = 0;
  uint32_t lease_time = 0, renew_time = 0, renew_server = 0;
  uint8_t type = 0;

  // Check if DHCP messages directed to us
  if ((len >= sizeof(dhcp_message_t)) &&
      (dhcp->operation == DHCP_OP_REPLY) &&
      (dhcp->transaction_id == dhcp_transaction_id) &&
      (dhcp->magic_cookie == DHCP_MAGIC_COOKIE)) {
    len -= sizeof(dhcp_message_t);

    // parse DHCP message
    uint8_t *op = dhcp->options;
    while (len >= sizeof(dhcp_option_t)) {
      dhcp_option_t *option = (dhcp_option_t *)op;
      if (option->code == DHCP_CODE_PAD) {
        op++;
        len--;
      } else if (option->code == DHCP_CODE_END) {
        break;
      } else {
        switch (option->code) {
        case DHCP_CODE_MESSAGETYPE:
          type = *(option->data);
          break;
        case DHCP_CODE_SUBNETMASK:
          offered_net_mask = *(uint32_t *)(option->data);
          break;
        case DHCP_CODE_GATEWAY:
          offered_gateway = *(uint32_t *)(option->data);
          break;
        case DHCP_CODE_DHCPSERVER:
          renew_server = *(uint32_t *)(option->data);
          break;
        case DHCP_CODE_LEASETIME:
        case DHCP_CODE_RENEWTIME: {
          uint32_t temp = *(uint32_t *)(option->data);
          lease_time = ntohl(temp);
          if (lease_time > 21600)
            lease_time = 21600;
          break;
        }
        }
        uint8_t optlen = sizeof(dhcp_option_t) + option->len;
        op += optlen;
        len -= optlen;
      }
    }

    if (!renew_server)
      renew_server = ip->from_addr;

    switch (type) {
    // DHCP offer?
    case DHCP_MESSAGE_OFFER:
      if ((dhcp_status == DHCP_WAITING_OFFER) && (dhcp->offered_addr != 0)) {
        dhcp_status = DHCP_WAITING_ACK;

        // send DHCP request
        ip->to_addr = inet_addr(255, 255, 255, 255);

        udp->to_port = DHCP_SERVER_PORT;
        udp->from_port = DHCP_CLIENT_PORT;

        op = dhcp->options;
        if (dhcp_hostname[0]) {
          // generate a base serial from the STM32 UID
          uint32_t uid[3];
          HAL_GetUID(uid);
          size_t len = strlen(dhcp_hostname);
          sprintf(dhcp_hostname + len, "%08X", uid[0] ^ uid[1] ^ uid[2]);
          len = strlen(dhcp_hostname);
          if (len < DHCP_HOSTNAME_MAX_LEN) {
            *op++ = DHCP_CODE_HOSTNAME;
            *op++ = len;
            strcpy((char *)op, dhcp_hostname);
            op += len;
          }
        }
        dhcp_add_option(op, DHCP_CODE_MESSAGETYPE, uint8_t, DHCP_MESSAGE_REQUEST);
        dhcp_add_option(op, DHCP_CODE_REQUESTEDADDR, uint32_t, dhcp->offered_addr);
        dhcp_add_option(op, DHCP_CODE_DHCPSERVER, uint32_t, renew_server);
        *(op++) = DHCP_CODE_END;

        dhcp->operation = DHCP_OP_REQUEST;
        dhcp->offered_addr = 0;
        dhcp->server_addr = 0;
        dhcp->flags = DHCP_FLAG_BROADCAST;

        udp_send(frame, (uint8_t *)op - (uint8_t *)dhcp);
      }
      break;

    // DHCP ack?
    case DHCP_MESSAGE_ACK:
      if ((dhcp_status == DHCP_WAITING_ACK) && lease_time) {
        if (!renew_time)
          renew_time = lease_time / 2;

        dhcp_status = DHCP_ASSIGNED;
        dhcp_server = renew_server;
        dhcp_renew_time = HAL_GetTick() / 1000 + renew_time;
        dhcp_retry_time = HAL_GetTick() / 1000 + lease_time;

        // network up
        gLAN_IPv4_address = dhcp->offered_addr;
        gLAN_IPv4_subnet_mask = offered_net_mask;
        gLan_IPv4_gateway = offered_gateway;
      }
      break;
    }
  }
}

void dhcp_poll(void) {
  eth_frame_t *frame = (eth_frame_t *)gLAN_rx_tx_buffer;
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  udp_packet_t *udp = (udp_packet_t *)(ip->data);
  dhcp_message_t *dhcp = (dhcp_message_t *)(udp->data);
  uint8_t *op;

  // time to initiate DHCP
  //  (startup/lease end)
  if ((HAL_GetTick() / 1000) >= dhcp_retry_time) {
    dhcp_status = DHCP_WAITING_OFFER;
    dhcp_retry_time = HAL_GetTick() / 1000 + 15;
    dhcp_transaction_id = HAL_GetTick() + (HAL_GetTick() << 16);

    // network down
    gLAN_IPv4_address = 0;
    gLAN_IPv4_subnet_mask = 0;
    gLan_IPv4_gateway = 0;

    // send DHCP discover
    ip->to_addr = inet_addr(255, 255, 255, 255);

    udp->to_port = DHCP_SERVER_PORT;
    udp->from_port = DHCP_CLIENT_PORT;

    memset(dhcp, 0, sizeof(dhcp_message_t));
    dhcp->operation = DHCP_OP_REQUEST;
    dhcp->hw_addr_type = DHCP_HW_ADDR_TYPE_ETH;
    dhcp->hw_addr_len = 6;
    dhcp->transaction_id = dhcp_transaction_id;
    dhcp->flags = DHCP_FLAG_BROADCAST;
    memcpy(dhcp->hw_addr, gLAN_MAC_address, 6);
    dhcp->magic_cookie = DHCP_MAGIC_COOKIE;

    op = dhcp->options;
    dhcp_add_option(op, DHCP_CODE_MESSAGETYPE,
      uint8_t, DHCP_MESSAGE_DISCOVER);
    *(op++) = DHCP_CODE_END;

    udp_send(frame, (uint8_t *)op - (uint8_t *)dhcp);
  }

  // time to renew lease
  if (((HAL_GetTick() / 1000) >= dhcp_renew_time) &&
      (dhcp_status == DHCP_ASSIGNED)) {
    dhcp_transaction_id = HAL_GetTick() + (HAL_GetTick() << 16);

    // send DHCP request
    ip->to_addr = dhcp_server;

    udp->to_port = DHCP_SERVER_PORT;
    udp->from_port = DHCP_CLIENT_PORT;

    memset(dhcp, 0, sizeof(dhcp_message_t));
    dhcp->operation = DHCP_OP_REQUEST;
    dhcp->hw_addr_type = DHCP_HW_ADDR_TYPE_ETH;
    dhcp->hw_addr_len = 6;
    dhcp->transaction_id = dhcp_transaction_id;
    dhcp->client_addr = gLAN_IPv4_address;
    memcpy(dhcp->hw_addr, gLAN_MAC_address, 6);
    dhcp->magic_cookie = DHCP_MAGIC_COOKIE;

    op = dhcp->options;
    dhcp_add_option(op, DHCP_CODE_MESSAGETYPE, uint8_t, DHCP_MESSAGE_REQUEST);
    dhcp_add_option(op, DHCP_CODE_REQUESTEDADDR, uint32_t, gLAN_IPv4_address);
    dhcp_add_option(op, DHCP_CODE_DHCPSERVER, uint32_t, dhcp_server);
    *(op++) = DHCP_CODE_END;

    if (!udp_send(frame, (uint8_t *)op - (uint8_t *)dhcp)) {
      dhcp_renew_time = HAL_GetTick() / 1000 + 5;
      return;
    }

    dhcp_status = DHCP_WAITING_ACK;
  }
}

void dhcp_init(void) {
  COMPILE_CHECK(sizeof(dhcp_message_t) == 240);
  COMPILE_CHECK(sizeof(dhcp_option_t) == 2);

  dhcp_retry_time = HAL_GetTick() / 1000 + 2;
}
#endif