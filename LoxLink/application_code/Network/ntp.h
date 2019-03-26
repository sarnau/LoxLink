#ifndef NTP_H
#define NTP_H

#include "lan_config.h"
#ifdef WITH_NTP
#include "ethernet.h"

#define NTP_SERVER  gLan_IPv4_gateway // the default NTP server is our gateway
#define NTP_LOCAL_PORT htons(14444) // destination port for NTP replies from the gateway

#ifdef __cplusplus
extern "C" {
#endif

void ntp_filter(eth_frame_t *frame, uint16_t len);
void ntp_poll(void);

// returns the current UNIX-time, 0 = no time received
uint32_t ntp_current_time(void);

#ifdef __cplusplus
}
#endif

#endif
#endif
