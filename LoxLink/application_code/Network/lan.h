#ifndef LAN_H
#define LAN_H

#include "lan_config.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t gLAN_rx_tx_buffer[]; // Shared receive/transmit buffer
extern uint8_t gLAN_MAC_address[6];
extern uint32_t gLAN_IPv4_address;    // != 0 => our IPv4 address
extern uint32_t gLAN_IPv4_subnet_mask;
extern uint32_t gLan_IPv4_gateway;

#define gLAN_IPv4_broadcast_address (gLAN_IPv4_address | ~gLAN_IPv4_subnet_mask)

/***
 *  Big-Endian conversion
 ***/
#define htons(a) ((((a) >> 8) & 0xff) | (((a) << 8) & 0xff00))
#define ntohs(a) htons(a)

#define htonl(a) ((((a) >> 24) & 0xff) | (((a) >> 8) & 0xff00) | (((a & 0xffffff) << 8) & 0xff0000) | (((a & 0xff) << 24) & 0xff000000))
#define ntohl(a) htonl(a)

#define inet_addr(a, b, c, d) (((uint32_t)a) | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24))
#define ipv4(a) ((a[3] << 24) | (a[2] << 16) | (a[1] << 8) | (a[0] << 0))

/***
 *  Available functions
 ***/
void lan_init(void); // initialize at boot time, request DHCP, if enabled
void lan_poll(void); // called in a loop to check for new packages, DHCP leases, etc

#ifdef __cplusplus
}
#endif

#endif