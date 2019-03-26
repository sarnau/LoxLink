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

/*
 * Values for peer.leap, sys_leap
 */
#define LEAP_NOWARNING 0x0 /* normal, no leap second warning */
#define LEAP_ADDSECOND 0x1 /* last minute of day has 61 seconds */
#define LEAP_DELSECOND 0x2 /* last minute of day has 59 seconds */
#define LEAP_NOTINSYNC 0x3 /* overload, clock is free running */

/*
 * Values for peer mode and packet mode. Only the modes through
 * MODE_BROADCAST and MODE_BCLIENT appear in the transition
 * function. MODE_CONTROL and MODE_PRIVATE can appear in packets,
 * but those never survive to the transition function.
 * is a
/ */
#define MODE_UNSPEC 0    /* unspecified (old version) */
#define MODE_ACTIVE 1    /* symmetric active mode */
#define MODE_PASSIVE 2   /* symmetric passive mode */
#define MODE_CLIENT 3    /* client mode */
#define MODE_SERVER 4    /* server mode */
#define MODE_BROADCAST 5 /* broadcast mode */
/*
 * These can appear in packets
 */
#define MODE_CONTROL 6 /* control mode */
#define MODE_PRIVATE 7 /* private mode */
/*
 * This is a madeup mode for broadcast client.
 */
#define MODE_BCLIENT 6 /* broadcast client mode */

/*
 * Values for peer.stratum, sys_stratum
 */
#define STRATUM_REFCLOCK ((u_char)0) /* default stratum */
/* A stratum of 0 in the packet is mapped to 16 internally */
#define STRATUM_PKT_UNSPEC ((u_char)0) /* unspecified in packet */
#define STRATUM_UNSPEC ((u_char)16)    /* unspecified */

/*
 * NTP packet format.
 */
typedef struct ntp_message {
  uint8_t li_vn_mode;      // peer leap indicator
  uint8_t stratum;         // peer stratum
  uint8_t ppoll;           // peer poll interval
  uint8_t precision;       // peer clock precision
  uint32_t rootdelay;      // roundtrip delay to primary source
  uint32_t rootdisp;       // dispersion to primary source
  uint32_t refid;          // reference id
  ntp_timestamp_t reftime; // last update time
  ntp_timestamp_t org;     // originate time stamp
  ntp_timestamp_t rec;     // receive time stamp
  ntp_timestamp_t xmt;     // transmit time stamp
} ntp_message_t;

/*
 * Stuff for extracting things from li_vn_mode
 */
#define PKT_MODE(li_vn_mode) ((u_char)((li_vn_mode)&0x7))
#define PKT_VERSION(li_vn_mode) ((u_char)(((li_vn_mode) >> 3) & 0x7))
#define PKT_LEAP(li_vn_mode) ((u_char)(((li_vn_mode) >> 6) & 0x3))

/*
 * Stuff for putting things back into li_vn_mode in packets and vn_mode
 * in ntp_monitor.c's mon_entry.
 */
#define VN_MODE(v, m) ((((v)&7) << 3) | ((m)&0x7))
#define PKT_LI_VN_MODE(l, v, m) ((((l)&3) << 6) | VN_MODE((v), (m)))

static uint32_t gNTP_next_update;
static uint32_t gNTP_currentTime;
static uint32_t gNTP_server;

static uint8_t ntp_request(void) {
  uint32_t srv_ip = gNTP_server;
  if(!srv_ip)
    srv_ip = gLan_IPv4_gateway; // the default NTP server is our gateway
  if(!srv_ip) // no known NTP server
    return 0;

  eth_frame_t *frame = (eth_frame_t *)gLAN_rx_tx_buffer;
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  udp_packet_t *udp = (udp_packet_t *)(ip->data);
  ntp_message_t *ntp = (ntp_message_t *)(udp->data);

  ip->to_addr = srv_ip;
  udp->to_port = NTP_SRV_PORT;
  udp->from_port = NTP_LOCAL_PORT;
  memset(ntp, 0, sizeof(ntp_message_t));
  ntp->li_vn_mode = PKT_LI_VN_MODE(LEAP_NOTINSYNC, 4, MODE_CLIENT); // NTP 4, Client request
  return udp_send(frame, sizeof(ntp_message_t));
}

void ntp_filter(eth_frame_t *frame, uint16_t len) {
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  udp_packet_t *udp = (udp_packet_t *)(ip->data);
  ntp_message_t *ntp = (ntp_message_t *)(udp->data);

  if (len >= sizeof(ntp_message_t)) {
    uint32_t temp = ntp->xmt.seconds;
    gNTP_currentTime = (ntohl(temp) - 2208988800UL); // minus 70 years, NTP is starts on 1/1/1900, UNIX on 1/1/1970
  }
}

void ntp_poll(void) {
  // Time to send NTP request?
  uint32_t currentTicks = HAL_GetTick() / 1000;
  if (currentTicks >= gNTP_next_update && gLan_IPv4_gateway != 0) {
    if (!ntp_request())
      gNTP_next_update = currentTicks + 2;
    else
      gNTP_next_update = currentTicks + 60;
  }
}

uint32_t ntp_current_time(void) {
  return gNTP_currentTime;
}

void ntp_set_server(uint32_t ip) {
    gNTP_server = ip;
    gNTP_next_update = 0; // force an NTP update
}

#endif