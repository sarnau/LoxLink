#ifndef ICMP_H
#define ICMP_H

#include "lan_config.h"
#ifdef WITH_ICMP

#include "ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif

void icmp_filter(eth_frame_t *frame, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif

#endif