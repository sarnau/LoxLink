#ifndef ETHERNET_H
#define ETHERNET_H

#include "lan_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/***
 *  Ethernet
 ***/
#define ETH_TYPE_ARP htons(0x0806)
#define ETH_TYPE_IP htons(0x0800)

typedef struct eth_frame {
  uint8_t to_addr[6];
  uint8_t from_addr[6];
  uint16_t type;
  uint8_t data[];
} eth_frame_t;

void eth_pool(void);
void eth_send(eth_frame_t *frame, uint16_t len);
void eth_reply(eth_frame_t *frame, uint16_t len);
void eth_resend(eth_frame_t *frame, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif