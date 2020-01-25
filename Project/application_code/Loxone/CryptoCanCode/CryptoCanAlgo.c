#include "CryptoCanAlgo.h"
#include "aes.h"
#include "hash.h"
#include <string.h>

void CryptoCanAlgo_DecryptInitPacket(uint8_t *data, uint32_t serial)
{
    uint32_t aesKey[4];
    uint32_t aesIV[4];
    for(int i=0; i<4; ++i) {
        aesKey[i] = ~serial ^ CryptoCanAlgoKey[i];
        aesIV[i] = serial ^ CryptoCanAlgoIV[i];
    }
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (uint8_t*)aesKey, (uint8_t*)aesIV);
    AES_CBC_decrypt_buffer(&ctx, data, 16);
}

void CryptoCanAlgo_DecryptInitPacketLegacy(uint8_t *data, uint32_t size, uint32_t serial)
{
    uint32_t aesKey[4];
    uint32_t aesIV[4];
    for(int i=0; i<4; ++i) {
        aesKey[i] = ~serial ^ CryptoCanAlgoLegacyKey[i];
        aesIV[i] = serial ^ CryptoCanAlgoLegacyIV[i];
    }
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (uint8_t*)aesKey, (uint8_t*)aesIV);
    AES_CBC_decrypt_buffer(&ctx, data, size);
}

void CryptoCanAlgo_EncryptInitPacketLegacy(uint8_t *data, uint32_t size, uint32_t serial)
{
    uint32_t aesKey[4];
    uint32_t aesIV[4];
    for(int i=0; i<4; ++i) {
        aesKey[i] = ~serial ^ CryptoCanAlgoLegacyKey[i];
        aesIV[i] = serial ^ CryptoCanAlgoLegacyIV[i];
    }
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (uint8_t*)aesKey, (uint8_t*)aesIV);
    AES_CBC_encrypt_buffer(&ctx, data, size);
}

void CryptoCanAlgo_DecryptDataPacket(uint8_t *data, uint32_t *key, uint32_t iv)
{
    uint32_t aesKey[4];
    uint32_t aesIV[4];
    for(int i=0; i<4; ++i) {
        aesKey[i] = iv ^ key[i];
        aesIV[i] = iv;
    }
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (uint8_t*)aesKey, (uint8_t*)aesIV);
    AES_CBC_decrypt_buffer(&ctx, data, 16);
}

void CryptoCanAlgo_EncryptDataPacket(uint8_t *data, uint32_t *key, uint32_t iv)
{
    uint32_t aesKey[4];
    uint32_t aesIV[4];
    for(int i=0; i<4; ++i) {
        aesKey[i] = iv ^ key[i];
        aesIV[i] = iv;
    }
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (uint8_t*)aesKey, (uint8_t*)aesIV);
    AES_CBC_encrypt_buffer(&ctx, data, 16);
}

void CryptoCanAlgo_SolveChallenge(uint32_t random, uint32_t serial, const uint8_t *deviceID, uint32_t *aesKey, uint32_t *aesIV)
{
    uint8_t buffer[20];
    memcpy(buffer, deviceID, 12);
    memcpy(buffer + 12, &random, sizeof(random));
    memcpy(buffer + 16, &serial, sizeof(serial));
    aesKey[0] = RSHash(buffer,sizeof(buffer));
    aesKey[1] = JSHash(buffer,sizeof(buffer));
    aesKey[2] = DJBHash(buffer,sizeof(buffer));
    aesKey[3] = DEKHash(buffer,sizeof(buffer));
    for(int i=0; i<sizeof(buffer); ++i)
        buffer[i] ^= 0xa5;
    aesIV[0] = RSHash(buffer,sizeof(buffer));
}

void CryptoCanAlgo_SolveChallengeLegacy(uint32_t random, uint32_t serial, const uint8_t *deviceID, uint32_t *aesKey, uint32_t *aesIV)
{
    uint8_t buffer[20];
    memcpy(buffer, deviceID, 12);
    memcpy(buffer + 12, &random, sizeof(random));
    memcpy(buffer + 16, &serial, sizeof(serial));
    aesKey[0] = RSHash(buffer,sizeof(buffer));
    aesKey[1] = JSHash(buffer,sizeof(buffer));
    aesKey[2] = DJBHash(buffer,sizeof(buffer));
    aesKey[3] = DEKHash(buffer,sizeof(buffer));
    aesIV[0] = BPHash(buffer,sizeof(buffer));
}
