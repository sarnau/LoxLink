#ifndef ARP_H
#define ARP_H

#include "lan_config.h"

#include "ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t *arp_resolve(uint32_t node_ip_addr);
void arp_filter(eth_frame_t *frame, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif