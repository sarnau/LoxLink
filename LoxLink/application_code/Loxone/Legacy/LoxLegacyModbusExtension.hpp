//
//  LoxLegacyModbusExtension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxLegacyModbusExtension_hpp
#define LoxLegacyModbusExtension_hpp

#include "FreeRTOS.h"
#include "LoxLegacyExtension.hpp"
#if EXTENSION_MODUS
#include "queue.h"

#define Modbus_RX_BUFFERSIZE 512
#define Modbus_TX_BUFFERSIZE 512

// The different state, in which the extension can be
typedef enum {
  eModbusChecksumMode_none = 0,
  eModbusChecksumMode_XOR = 1,
  eModbusChecksumMode_Sum = 2,
  eModbusChecksumMode_CRC = 3,
  eModbusChecksumMode_ModbusCRC = 4,
  eModbusChecksumMode_Fronius = 5,
} eModbusChecksumMode;


class LoxLegacyModbusExtension : public LoxLegacyExtension {
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
  eModbusChecksumMode checksumMode;

  void forwardBuffer(const uint8_t *buffer, size_t byteCount);
  void sendBuffer(const uint8_t *buffer, size_t byteCount);
  static void vModbusRXTask(void *pvParameters);
  static void vModbusTXTask(void *pvParameters);

  virtual void PacketToExtension(LoxCanMessage &message);

public:
  LoxLegacyModbusExtension(LoxCANBaseDriver &driver, uint32_t serial);

  virtual void Startup(void);
};

#endif

#endif /* LoxLegacyModbusExtension_hpp */