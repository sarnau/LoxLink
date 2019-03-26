#include "ethernet.h"
#include "arp.h"
#include "enc28j60.h"
#include "ip.h"
#include "lan.h"
#include <string.h>

// Shared RX/TD packet buffer
uint8_t gLAN_rx_tx_buffer[ENC28J60_MAX_FRAMELEN];


// send new Ethernet frame to same host
// (can be called directly after eth_send)
void eth_resend(eth_frame_t *frame, uint16_t len) {
  ENC28J60_sendPacket(frame, len + sizeof(eth_frame_t));
}

/***
 *  send Ethernet frame
 *  fields must be set:
 *  - frame.dst
 *  - frame.type
 ***/
void eth_send(eth_frame_t *frame, uint16_t len) {
  memcpy(frame->from_addr, gLAN_MAC_address, 6);
  eth_resend(frame, len);
}

/***
 *  send Ethernet frame back
 ***/
void eth_reply(eth_frame_t *frame, uint16_t len) {
  memcpy(frame->to_addr, frame->from_addr, 6);
  eth_send(frame, len);
}

/***
 *  process Ethernet frame
 ***/
void eth_filter(eth_frame_t *frame, uint16_t len) {
  if (len >= sizeof(eth_frame_t)) {
    switch (frame->type) {
    case ETH_TYPE_ARP:
      arp_filter(frame, len - sizeof(eth_frame_t));
      break;
    case ETH_TYPE_IP:
      ip_filter(frame, len - sizeof(eth_frame_t));
      break;
    }
  }
}

/***
 *  Check for arrival of new packages
 ***/
void eth_poll(void) {
  uint16_t len;
  while ((len = ENC28J60_receivePacket(gLAN_rx_tx_buffer, sizeof(gLAN_rx_tx_buffer)))) {
    eth_frame_t *frame = (eth_frame_t *)gLAN_rx_tx_buffer;
    eth_filter(frame, len);
  }
}