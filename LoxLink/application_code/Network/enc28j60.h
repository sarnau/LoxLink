#ifndef ENC28J60_H
#define ENC28J60_H

#include "lan_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENC28J60_MAX_FRAMELEN (1500) // maximum the controller accepts

void ENC28J60_init(const uint8_t *macadr);
int ENC28J60_isLinkUp();
void ENC28J60_sendPacket(const uint8_t *data, uint16_t len);
uint16_t ENC28J60_receivePacket(uint8_t *buf, uint16_t buflen);

#ifdef __cplusplus
}
#endif

#endif