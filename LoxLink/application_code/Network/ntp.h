#ifndef NTP_H
#define NTP_H

#include "lan_config.h"
#ifdef WITH_NTP
#include "ethernet.h"

#define NTP_LOCAL_PORT htons(1337) // destination port for NTP replies from the gateway

#ifdef __cplusplus
extern "C" {
#endif

void ntp_filter(eth_frame_t *frame, uint16_t len);
void ntp_poll(void);

// set the NTP server, this is called automatically by the DHCP client or
// has to be called manually, if DHCP is disabled. Often the local gateway
// is also a NTP server.
// Setting the server will also trigger an NTP request.
void ntp_set_server(uint32_t ip);

// returns the current UNIX-time, 0 = no time received
uint32_t ntp_current_time(void);

#ifdef __cplusplus
}
#endif

#endif
#endif