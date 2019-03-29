#include "ethernet.h"
#include "arp.h"
#include "enc28j60.h"
#include "ip.h"
#include "lan.h"
#include <string.h>
#include <stdio.h>

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


#if 0
static void debug_print_buffer(const void *data, size_t size) {
  const int LineLength = 16;
  const uint8_t *dp = (const uint8_t *)data;
  printf("\n");
  static char pbuf[64];
  for (int loffset = 0; loffset < size; loffset += LineLength) {
    int poffset = 0;
    sprintf(pbuf+poffset, "%04x : ", loffset); poffset += 7;
    for (int i = 0; i < LineLength; ++i) {
      int offset = loffset + i;
      if (offset < size) {
        sprintf(pbuf+poffset,"%02x ", dp[offset]); poffset += 3;
      } else {
        sprintf(pbuf+poffset,"   "); poffset += 3;
      }
    }
    sprintf(pbuf+poffset," "); poffset += 1;
    for (int i = 0; i < LineLength; ++i) {
      int offset = loffset + i;
      if (offset >= size)
        break;
      uint8_t c = dp[offset];
      if (c < 0x20 || c >= 0x7f)
        c = '.';
      sprintf(pbuf+poffset,"%c", c); poffset += 1;
    }
    sprintf(pbuf+poffset,"\n");
    printf(pbuf);
  }
}
#endif

/***
 *  Check for arrival of new packages
 ***/
void eth_poll(void) {
  uint16_t len;
  while ((len = ENC28J60_receivePacket(gLAN_rx_tx_buffer, sizeof(gLAN_rx_tx_buffer)))) {
    eth_frame_t *frame = (eth_frame_t *)gLAN_rx_tx_buffer;
#if 0
    // ignore broadcast
    if(gLAN_rx_tx_buffer[0] != 0xff && gLAN_rx_tx_buffer[1] != 0xff && gLAN_rx_tx_buffer[2] != 0xff && gLAN_rx_tx_buffer[3] != 0xff && gLAN_rx_tx_buffer[4] != 0xff && gLAN_rx_tx_buffer[5] != 0xff) {
        debug_print_buffer(gLAN_rx_tx_buffer, len);
    }
#endif
    eth_filter(frame, len);
  }
}