#include "ntp.h"

#ifdef WITH_NTP
#include "ip.h"
#include "lan.h"
#include "stm32f1xx_hal.h" // HAL_GetUID() and HAL_GetTick()
#include "udp.h"
#include <string.h>

#define NTP_SRV_PORT htons(123)

typedef struct ntp_timestamp {
  uint32_t seconds;
  uint32_t fraction;
} ntp_timestamp_t;

typedef struct ntp_message {
  uint8_t status;
  uint8_t type;
  uint16_t precision;
  uint32_t est_error;
  uint32_t est_drift_rate;
  uint32_t ref_clock_id;
  ntp_timestamp_t ref_timestamp;
  ntp_timestamp_t orig_timestamp;
  ntp_timestamp_t recv_timestamp;
  ntp_timestamp_t xmit_timestamp;
} ntp_message_t;

static uint32_t gNTP_next_update;
static uint32_t gNTP_currentTime;

static uint8_t ntp_request(uint32_t srv_ip) {
  eth_frame_t *frame = (eth_frame_t *)gLAN_rx_tx_buffer;
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  udp_packet_t *udp = (udp_packet_t *)(ip->data);
  ntp_message_t *ntp = (ntp_message_t *)(udp->data);

  ip->to_addr = srv_ip;
  udp->to_port = NTP_SRV_PORT;
  udp->from_port = NTP_LOCAL_PORT;
  memset(ntp, 0, sizeof(ntp_message_t));
  ntp->status = 0x08;
  return udp_send(frame, sizeof(ntp_message_t));
}

void ntp_filter(eth_frame_t *frame, uint16_t len) {
  ntp_message_t *ntp = (ntp_message_t *)frame;

  if (len >= sizeof(ntp_message_t)) {
    uint32_t temp = ntp->xmit_timestamp.seconds;
    gNTP_currentTime = (ntohl(temp) - 2208988800UL);
  }
}

void ntp_poll(void) {
  // Time to send NTP request?
  uint32_t currentTicks = HAL_GetTick() / 1000;
  if (currentTicks >= gNTP_next_update) {
    if (!ntp_request(NTP_SERVER))
      gNTP_next_update = currentTicks + 2;
    else
      gNTP_next_update = currentTicks + 60;
  }
}

uint32_t ntp_current_time(void) {
  return gNTP_currentTime;
}

#endif