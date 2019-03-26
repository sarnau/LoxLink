#include "ethernet.h"
#include "lan.h"
#include <string.h>

/***
 *  Options
 ***/
#define ARP_CACHE_SIZE (3)

/***
 *  ARP
 ***/
#define ARP_HW_TYPE_ETH htons(0x0001)
#define ARP_PROTO_TYPE_IP htons(0x0800)

#define ARP_TYPE_REQUEST htons(1)
#define ARP_TYPE_RESPONSE htons(2)

typedef struct arp_message {
  uint16_t hw_type;
  uint16_t proto_type;
  uint8_t hw_addr_len;
  uint8_t proto_addr_len;
  uint16_t type;
  uint8_t mac_addr_from[6];
  uint8_t ip_addr_from[4];
  uint8_t mac_addr_to[6];
  uint8_t ip_addr_to[4];
} arp_message_t;

static int arp_cache_wr;
static struct arp_cache_entry {
  uint32_t ip_addr;
  uint8_t mac_addr[6];
} arp_cache[ARP_CACHE_SIZE];

/***
 *  search ARP cache
 ***/
uint8_t *arp_search_cache(uint32_t node_ip_addr) {
  for (int i = 0; i < ARP_CACHE_SIZE; ++i) {
    if (arp_cache[i].ip_addr == node_ip_addr)
      return arp_cache[i].mac_addr;
  }
  return 0;
}

/***
 *  resolve MAC address
 *  returns 0 if still resolving
 *  (invalidates gLAN_rx_tx_buffer if not resolved)
 ***/
uint8_t *arp_resolve(uint32_t node_ip_addr) {
  eth_frame_t *frame = (eth_frame_t *)gLAN_rx_tx_buffer;
  arp_message_t *msg = (arp_message_t *)(frame->data);

  // search arp cache
  uint8_t *mac = arp_search_cache(node_ip_addr);
  if (mac)
    return mac;

  // send request
  memset(frame->to_addr, 0xff, 6);
  frame->type = ETH_TYPE_ARP;

  msg->hw_type = ARP_HW_TYPE_ETH;
  msg->proto_type = ARP_PROTO_TYPE_IP;
  msg->hw_addr_len = 6;
  msg->proto_addr_len = 4;
  msg->type = ARP_TYPE_REQUEST;
  memcpy(msg->mac_addr_from, gLAN_MAC_address, 6);
  msg->ip_addr_from[3] = gLAN_IPv4_address >> 24;
  msg->ip_addr_from[2] = gLAN_IPv4_address >> 16;
  msg->ip_addr_from[1] = gLAN_IPv4_address >> 8;
  msg->ip_addr_from[0] = gLAN_IPv4_address >> 0;
  memset(msg->mac_addr_to, 0x00, 6);
  msg->ip_addr_to[3] = node_ip_addr >> 24;
  msg->ip_addr_to[2] = node_ip_addr >> 16;
  msg->ip_addr_to[1] = node_ip_addr >> 8;
  msg->ip_addr_to[0] = node_ip_addr >> 0;
  eth_send(frame, sizeof(arp_message_t));
  return 0;
}

/***
 *  process arp packet
 ***/
void arp_filter(eth_frame_t *frame, uint16_t len) {
  arp_message_t *msg = (void *)(frame->data);
  COMPILE_CHECK(sizeof(arp_message_t) == 28);

  if (len >= sizeof(arp_message_t)) {
    if ((msg->hw_type == ARP_HW_TYPE_ETH) &&
        (msg->proto_type == ARP_PROTO_TYPE_IP) &&
        (ipv4(msg->ip_addr_to) == gLAN_IPv4_address)) {
      switch (msg->type) {
      case ARP_TYPE_REQUEST:
        msg->type = ARP_TYPE_RESPONSE;
        memcpy(msg->mac_addr_to, msg->mac_addr_from, 6);
        memcpy(msg->mac_addr_from, gLAN_MAC_address, 6);
        msg->ip_addr_to[3] = msg->ip_addr_from[3];
        msg->ip_addr_to[2] = msg->ip_addr_from[2];
        msg->ip_addr_to[1] = msg->ip_addr_from[1];
        msg->ip_addr_to[0] = msg->ip_addr_from[0];
        msg->ip_addr_from[3] = gLAN_IPv4_address >> 24;
        msg->ip_addr_from[2] = gLAN_IPv4_address >> 16;
        msg->ip_addr_from[1] = gLAN_IPv4_address >> 8;
        msg->ip_addr_from[0] = gLAN_IPv4_address >> 0;
        eth_reply(frame, sizeof(arp_message_t));
        break;
      case ARP_TYPE_RESPONSE:
        if (!arp_search_cache(ipv4(msg->ip_addr_from))) {
          arp_cache[arp_cache_wr].ip_addr = ipv4(msg->ip_addr_from);
          memcpy(arp_cache[arp_cache_wr].mac_addr, msg->mac_addr_from, 6);
          arp_cache_wr++;
          if (arp_cache_wr == ARP_CACHE_SIZE)
            arp_cache_wr = 0;
        }
        break;
      }
    }
  }
}