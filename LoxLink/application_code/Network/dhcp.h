#ifndef DHCP_H
#define DHCP_H

#include "lan_config.h"
#ifdef WITH_DHCP

#include "ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DHCP_CLIENT_PORT htons(68)

void dhcp_init(void);
void dhcp_filter(eth_frame_t *frame, uint16_t len);
void dhcp_poll(void);

#ifdef __cplusplus
}
#endif

#endif

#endif