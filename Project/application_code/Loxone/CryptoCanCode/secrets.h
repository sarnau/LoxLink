#ifndef _SECRETS_H_
#define _SECRETS_H_

#include <stdint.h>

extern const uint8_t CryptoEncryptedAESKey[16];
extern const uint8_t CryptoEncryptedAESIV[16];
extern const uint32_t CryptoCanAlgoKey[4];
extern const uint32_t CryptoCanAlgoIV[4];

extern const uint32_t CryptoCanAlgoLegacyKey[4];
extern const uint32_t CryptoCanAlgoLegacyIV[4];

// This device ID key is used for all legacy and NAT extensions.
// Tree devices are returning their own device ID, which is the STM32 UID.
extern const uint8_t CryptoMasterDeviceID[12];

#endif
