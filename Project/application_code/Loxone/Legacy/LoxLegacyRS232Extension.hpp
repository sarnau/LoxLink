//
//  LoxLegacyRS232Extension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxLegacyRS232Extension_hpp
#define LoxLegacyRS232Extension_hpp

#include "LoxLegacyExtension.hpp"
#if EXTENSION_RS232
#include "queue.h"

#define RS232_RX_BUFFERSIZE 512
#define RS232_TX_BUFFERSIZE 512

// The different state, in which the extension can be
typedef enum {
  eRS232ChecksumMode_none = 0,
  eRS232ChecksumMode_XOR = 1,
  eRS232ChecksumMode_Sum = 2,
  eRS232ChecksumMode_CRC = 3,
  eRS232ChecksumMode_ModbusCRC = 4,
  eRS232ChecksumMode_Fronius = 5,
} eRS232ChecksumMode;


class LoxLegacyRS232Extension : public LoxLegacyExtension {
  uint8_t sendCount;
  uint8_t sendFill;
  uint8_t sendCRC;
  uint8_t sendData[256]; // max. size of one send buffer as received via several messages from the Miniserver
  StaticQueue_t txQueue;
  bool hasAck;
  uint8_t ack_byte;
  bool hasNak;
  uint8_t nak_byte;
  bool hasEndCharacter;
  uint8_t endCharacter;
  eRS232ChecksumMode checksumMode;

  void forwardBuffer(const uint8_t *buffer, size_t byteCount);
  void sendBuffer(const uint8_t *buffer, size_t byteCount);
  static void vRS232RXTask(void *pvParameters);
  static void vRS232TXTask(void *pvParameters);

  virtual void PacketToExtension(LoxCanMessage &message);

public:
  LoxLegacyRS232Extension(LoxCANBaseDriver &driver, uint32_t serial);

  virtual void Startup(void);
};
#endif

#endif /* LoxLegacyRS232Extension_hpp */
