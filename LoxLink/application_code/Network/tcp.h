#ifndef TCP_H
#define TCP_H

#include "lan_config.h"
#ifdef WITH_TCP

#include "ethernet.h"

//#define WITH_TCP_REXMIT


#ifdef __cplusplus
extern "C" {
#endif

#define TCP_MAX_CONNECTIONS 5

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

void tcp_filter(eth_frame_t *frame, uint16_t len);
void tcp_poll(void);

// TCP callbacks
uint8_t tcp_listen(uint8_t id, eth_frame_t *frame);
void tcp_read(uint8_t id, eth_frame_t *frame, uint8_t re);
void tcp_write(uint8_t id, eth_frame_t *frame, uint16_t len);
void tcp_closed(uint8_t id, uint8_t hard);

// TCP calls
uint8_t tcp_open(uint32_t addr, uint16_t port, uint16_t local_port);
void tcp_send(uint8_t id, eth_frame_t *frame, uint16_t len, uint8_t options);

#ifdef __cplusplus
}
#endif

#endif

#endif