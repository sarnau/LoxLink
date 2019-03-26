#include "udp.h"
#ifdef WITH_UDP

#include "dhcp.h"
#include "lan.h"

/***
 *  send UDP packet
 *  fields must be set:
 *  - ip.dst
 *  - udp.src_port
 *  - udp.dst_port
 *  uint16_t len is UDP data payload length
 ***/
uint8_t udp_send(eth_frame_t *frame, uint16_t len) {
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  udp_packet_t *udp = (udp_packet_t *)(ip->data);

  len += sizeof(udp_packet_t);

  ip->protocol = IP_PROTOCOL_UDP;
  ip->from_addr = gLan_ip_addr;

  udp->len = htons(len);
  udp->cksum = 0;
  udp->cksum = ip_cksum(len + IP_PROTOCOL_UDP, (uint8_t *)udp - 8, len + 8);

  return ip_send(frame, len);
}

/***
 *  reply to UDP packet
 *  len is UDP data payload length
 ***/
void udp_reply(eth_frame_t *frame, uint16_t len) {
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  udp_packet_t *udp = (udp_packet_t *)(ip->data);

  len += sizeof(udp_packet_t);

  ip->to_addr = gLan_ip_addr;

  uint16_t temp = udp->from_port;
  udp->from_port = udp->to_port;
  udp->to_port = temp;

  udp->len = htons(len);

  udp->cksum = 0;
  udp->cksum = ip_cksum(len + IP_PROTOCOL_UDP,
    (uint8_t *)udp - 8, len + 8);

  ip_reply(frame, len);
}

/***
 *  process UDP packet
 ***/
void udp_filter(eth_frame_t *frame, uint16_t len) {
  COMPILE_CHECK(sizeof(udp_packet_t) == 8);

  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  udp_packet_t *udp = (udp_packet_t *)(ip->data);

  if (len >= sizeof(udp_packet_t)) {
    len = ntohs(udp->len) - sizeof(udp_packet_t);

    switch (udp->to_port) {
#ifdef WITH_DHCP
    case DHCP_CLIENT_PORT:
      dhcp_filter(frame, len);
      break;
#endif
    default:
      udp_packet(frame, len);
      break;
    }
  }
}

#endif