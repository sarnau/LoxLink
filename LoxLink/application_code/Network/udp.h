#ifndef UDP_H
#define UDP_H

#include "lan_config.h"
#ifdef WITH_UDP

#include "ip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct udp_packet {
  uint16_t from_port;
  uint16_t to_port;
  uint16_t len;
  uint16_t cksum;
  uint8_t data[];
} udp_packet_t;

void udp_filter(eth_frame_t *frame, uint16_t len);

// UDP callback
void udp_packet(eth_frame_t *frame, uint16_t len);

// UDP calls
uint8_t udp_send(eth_frame_t *frame, uint16_t len);
void udp_reply(eth_frame_t *frame, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif

#endif