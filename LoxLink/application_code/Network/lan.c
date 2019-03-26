#include "lan.h"
#include "dhcp.h"
#include "enc28j60.h"
#include "tcp.h"
#include "ntp.h"

#include "stm32f1xx_hal.h" // HAL_GetUID()

// MAC address
uint8_t gLAN_MAC_address[6];

// IP address/mask/gateway
#ifndef WITH_DHCP
uint32_t gLAN_IPv4_addr = IP_ADDR;
uint32_t gLAN_IPv4_subnet_mask = IP_SUBNET_MASK;
uint32_t gLan_IPv4_gateway = IP_DEFAULT_GATEWAY;
#endif

void lan_init() {
  // generate a MAC address from the STM32 UID
  uint32_t uid[3];
  HAL_GetUID(uid);
  uint32_t val = uid[0] ^ uid[1] ^ uid[2];
  gLAN_MAC_address[0] = (0x22 & 0xFE) | 0x02; // clear multicast bit, set locally adminstered bit
  gLAN_MAC_address[1] = 0x22;
  gLAN_MAC_address[2] = val >> 24;
  gLAN_MAC_address[3] = val >> 16;
  gLAN_MAC_address[4] = val >> 8;
  gLAN_MAC_address[5] = val >> 0;
  ENC28J60_init(gLAN_MAC_address);

#ifdef WITH_DHCP
  dhcp_init();
#endif
}

void lan_poll() {
  eth_poll();

#ifdef WITH_DHCP
  dhcp_poll();
#endif

#ifdef WITH_TCP
  tcp_poll();
#endif

#ifdef WITH_NTP
  ntp_poll();
#endif
}