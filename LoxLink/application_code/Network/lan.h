#include "enc28j60.h"

#ifdef __cplusplus
extern "C" {
#endif

/***
 *  Options
 ***/
#define WITH_ICMP // enable support for ping
#define WITH_DHCP // DHCP IP lookup, instead of a static IP
#define WITH_UDP  // sending/receiving UDP packages
#define WITH_TCP  // TCP connection support
//#define WITH_TCP_REXMIT

/***
 *  Config
 ***/
#ifndef WITH_DHCP
#define IP_ADDR inet_addr(192, 168, 178, 213)
#define IP_SUBNET_MASK inet_addr(255, 255, 255, 0)
#define IP_DEFAULT_GATEWAY inet_addr(192, 168, 178, 1)
#endif

#define ARP_CACHE_SIZE 3
#define IP_PACKET_TTL 64

#ifdef WITH_TCP
#define TCP_MAX_CONNECTIONS 5
#define TCP_WINDOW_SIZE 65535
#define TCP_SYN_MSS 512
#ifdef WITH_TCP_REXMIT
#define TCP_REXMIT_TIMEOUT 1000
#define TCP_REXMIT_LIMIT 5
#else
#define TCP_CONN_TIMEOUT 2500
#endif
#endif

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
 *  Ethernet
 ***/
#define ETH_TYPE_ARP htons(0x0806)
#define ETH_TYPE_IP htons(0x0800)

typedef struct eth_frame {
  uint8_t to_addr[6];
  uint8_t from_addr[6];
  uint16_t type;
  uint8_t data[];
} eth_frame_t;

/***
 *  IP
 ***/
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17

typedef struct ip_packet {
  uint8_t ver_head_len;
  uint8_t tos;
  uint16_t total_len;
  uint16_t fragment_id;
  uint16_t flags_framgent_offset;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t cksum;
  uint32_t from_addr;
  uint32_t to_addr;
  uint8_t data[];
} ip_packet_t;

/***
 *  UDP
 ***/
#ifdef WITH_UDP
typedef struct udp_packet {
  uint16_t from_port;
  uint16_t to_port;
  uint16_t len;
  uint16_t cksum;
  uint8_t data[];
} udp_packet_t;
#endif

/***
 *  TCP
 ***/
#ifdef WITH_TCP
typedef struct tcp_packet {
  uint16_t from_port;
  uint16_t to_port;
  uint32_t seq_num;
  uint32_t ack_num;
  uint8_t data_offset;
  uint8_t flags;
  uint16_t window;
  uint16_t cksum;
  uint16_t urgent_ptr;
  uint8_t data[];
} tcp_packet_t;

#define tcp_head_size(tcp) (((tcp)->data_offset & 0xf0) >> 2)
#define tcp_get_data(tcp) ((uint8_t *)(tcp) + tcp_head_size(tcp))

#define TCP_OPTION_PUSH 0x01
#define TCP_OPTION_CLOSE 0x02
#endif

/***
 *  LAN
 ***/
extern uint8_t gLan_net_buf[];
extern uint32_t gLan_ip_addr;
extern uint32_t gLan_ip_mask;
extern uint32_t gLan_ip_gateway;

// LAN calls
void lan_init(void);
void lan_poll(void);
int lan_up(void);

#ifdef WITH_UDP
// UDP callback
void udp_packet(eth_frame_t *frame, uint16_t len);

// UDP calls
uint8_t udp_send(eth_frame_t *frame, uint16_t len);
void udp_reply(eth_frame_t *frame, uint16_t len);
#endif

#ifdef WITH_TCP
// TCP callbacks
uint8_t tcp_listen(uint8_t id, eth_frame_t *frame);
void tcp_read(uint8_t id, eth_frame_t *frame, uint8_t re);
void tcp_write(uint8_t id, eth_frame_t *frame, uint16_t len);
void tcp_closed(uint8_t id, uint8_t hard);

// TCP calls
uint8_t tcp_open(uint32_t addr, uint16_t port, uint16_t local_port);
void tcp_send(uint8_t id, eth_frame_t *frame, uint16_t len, uint8_t options);
#endif

#ifdef __cplusplus
}
#endif