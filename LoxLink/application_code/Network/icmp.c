#include "lan.h"
#include "ip.h"

#ifdef WITH_ICMP

#define ICMP_TYPE_ECHO_RQ 8
#define ICMP_TYPE_ECHO_RPLY 0

typedef struct icmp_echo_packet {
  uint8_t type;
  uint8_t code;
  uint16_t cksum;
  uint16_t id;
  uint16_t seq;
  uint8_t data[];
} icmp_echo_packet_t;

/***
 *  process ICMP packet
 ***/
void icmp_filter(eth_frame_t *frame, uint16_t len) {
  COMPILE_CHECK(sizeof(icmp_echo_packet_t) == 8);
  ip_packet_t *packet = (ip_packet_t *)frame->data;
  icmp_echo_packet_t *icmp = (icmp_echo_packet_t *)packet->data;

  if (len >= sizeof(icmp_echo_packet_t)) {
    if (icmp->type == ICMP_TYPE_ECHO_RQ) {
      icmp->type = ICMP_TYPE_ECHO_RPLY;
      icmp->cksum += 8; // update cksum
      ip_reply(frame, len);
    }
  }
}

#endif