#ifndef _CRYPTOCANALGO_H_
#define _CRYPTOCANALGO_H_

#include <stdint.h>
#include "secrets.h"

// This code only works on little endian (casts between uint32_t <-> uint8_t). Thats fine for ARM and x86.

extern void CryptoCanAlgo_DecryptInitPacket(uint8_t *data, uint32_t serial);

extern void CryptoCanAlgo_DecryptInitPacketLegacy(uint8_t *data, uint32_t size, uint32_t serial);
extern void CryptoCanAlgo_EncryptInitPacketLegacy(uint8_t *data, uint32_t size, uint32_t serial);

extern void CryptoCanAlgo_DecryptDataPacket(uint8_t *data, uint32_t *key, uint32_t iv);
extern void CryptoCanAlgo_EncryptDataPacket(uint8_t *data, uint32_t *key, uint32_t iv);

extern void CryptoCanAlgo_SolveChallenge(uint32_t random, uint32_t serial, const uint8_t *deviceID, uint32_t *aesKey, uint32_t *aesIV);
extern void CryptoCanAlgo_SolveChallengeLegacy(uint32_t random, uint32_t serial, const uint8_t *deviceID, uint32_t *aesKey, uint32_t *aesIV);

#endif
