#include "lan.h"
#include "arp.h"
#include "dhcp.h"
#include "enc28j60.h"
#include "icmp.h"
#include "stm32f1xx_hal.h" // HAL_GetUID()
#include "tcp.h"
#include <string.h>

// MAC address
uint8_t gLan_MAC_address[6];

// IP address/mask/gateway
#ifndef WITH_DHCP
uint32_t gLan_ip_addr = IP_ADDR;
uint32_t gLan_ip_mask = IP_SUBNET_MASK;
uint32_t gLan_ip_gateway = IP_DEFAULT_GATEWAY;
#endif

/***
 *  LAN
 ***/
void lan_init() {
  // generate a MAC address from the STM32 UID
  uint32_t uid[3];
  HAL_GetUID(uid);
  uint32_t val = uid[0] ^ uid[1] ^ uid[2];
  gLan_MAC_address[0] = 0x22;
  gLan_MAC_address[1] = 0x22;
  gLan_MAC_address[2] = val >> 24;
  gLan_MAC_address[3] = val >> 16;
  gLan_MAC_address[4] = val >> 8;
  gLan_MAC_address[5] = val >> 0;
  ENC28J60_init(gLan_MAC_address);

#ifdef WITH_DHCP
  dhcp_init();
#endif
}

void lan_poll() {
  eth_pool();

#ifdef WITH_DHCP
  dhcp_poll();
#endif

#ifdef WITH_TCP
  tcp_poll();
#endif
}