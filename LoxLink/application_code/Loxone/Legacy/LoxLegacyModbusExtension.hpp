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
#if EXTENSION_MODBUS
#include "queue.h"

#define Modbus_RX_BUFFERSIZE 512
#define Modbus_TX_BUFFERSIZE 512

typedef enum {
  tModbusCode_ReadCoils = 1,
  tModbusCode_ReadDiscreteInputs = 2,
  tModbusCode_ReadHoldingRegisters = 3,
  tModbusCode_ReadInputRegister = 4,
  tModbusCode_WriteSingleCoil = 5,
  tModbusCode_WriteSingleRegister = 6,
  tModbusCode_ReadExceptionStatus = 7,
  tModbusCode_Diagnostic = 8,
  tModbusCode_GetComEventCounter = 11,
  tModbusCode_GetComEventLog = 12,
  tModbusCode_WriteMultipleCoils = 15,
  tModbusCode_WriteMultipleRegisters = 16,
  tModbusCode_ReportServerID = 17,
  tModbusCode_ReadFileRecord = 20,
  tModbusCode_WriteFileRecord = 21,
  tModbusCode_MaskWriteRegister = 22,
  tModbusCode_ReadWriteMultipleRegisters = 23,
  tModbusCode_ReadDeviceIdentification = 43,
} tModbusCode;

typedef struct {
  uint8_t address;
  uint8_t functionCode;
  uint16_t regNumber;
  uint32_t pollingCycle;
} sModbusDeviceConfig;

typedef struct {            // default values:
  uint32_t magic;           // 0xFEEDFEED
  uint16_t version;         // 1
  uint8_t manualTimingFlag; // 0
  uint8_t unused;
  uint32_t baudrate;      // 19200 baud
  uint8_t wordLength;     // 8 bits
  uint8_t parity;         // 0 (none)
  uint8_t twoStopBits;    // 1
  uint8_t protocol;       // 3
  uint32_t timingPause;   // 10
  uint32_t timingTimeout; // 1000
  uint32_t entryCount;    // 0
  sModbusDeviceConfig devices[254];
  uint32_t filler; // first value after the CRC32 checksum
} sModbusConfig;

class LoxLegacyModbusExtension : public LoxLegacyExtension {
  uint8_t sendCount;
  uint8_t sendFill;
  uint8_t sendCRC;
  uint8_t sendData[256]; // max. size of one send buffer as received via several messages from the Miniserver
  StaticQueue_t txQueue;
  sModbusConfig config;

  void set_tx_mode(bool txMode);
  void config_load(void);
  void forwardBuffer(const uint8_t *buffer, size_t byteCount);
  void sendBuffer(const uint8_t *buffer, size_t byteCount);
  static void vModbusRXTask(void *pvParameters);
  static void vModbusTXTask(void *pvParameters);

  virtual void PacketToExtension(LoxCanMessage &message);
  virtual void FragmentedPacketToExtension(LoxMsgLegacyFragmentedCommand_t fragCommand, const void *fragData, int size);
  virtual void StartRequest();

public:
  LoxLegacyModbusExtension(LoxCANBaseDriver &driver, uint32_t serial);

  virtual void Startup(void);
};

#endif

#endif /* LoxLegacyModbusExtension_hpp */