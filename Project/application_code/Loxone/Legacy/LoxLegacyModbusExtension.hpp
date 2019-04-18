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

#define Modbus_RX_BUFFERSIZE 64
#define Modbus_TX_BUFFERSIZE 1024

// Modbus commands
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

// Possible error states, returned to the Miniserver
typedef enum {
  tModbusError_ActorResponse = 0,
  tModbusError_NoResponse = 1,
  tModbusError_CRC_Error = 2,
  tModbusError_InvalidResponse = 3,
  tModbusError_InvalidReceiveLength = 4,
  tModbusError_UnexpectedError = 5,
  tModbusError_TxQueueOverrun = 6,
} tModbusError;

// Possible error states, returned to the Miniserver
typedef enum {
  // The values 0x000-0xFFF are the polling cycle, which is in 100ms ticks
  tModbusFlags_1000ms = 0x1000, // multiply the polling cycle by 10
  tModbusFlags_combineTwoRegs = 0x2000,
  tModbusFlags_regOrderHighLow = 0x4000,
  tModbusFlags_littleEndian = 0x8000,
} tModbusFlags;

/***
 *  Modbus configuration data
 ***/
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
  uint8_t parity;         // 0 (none), 1 (even), 2 (odd), 3 (always 0, always 1 - both are no legal vor Modbus anyway)
  uint8_t twoStopBits;    // 0 (1 stop-bit), 1 (2 stop-bits)
  uint8_t protocol;       // 3 (RTU? Always 3)
  uint32_t timingPause;   // 10
  uint32_t timingTimeout; // 1000
  uint32_t entryCount;    // 0 number of entries in the device table
  sModbusDeviceConfig devices[254];
  uint32_t filler; // first value after the CRC32 checksum
} sModbusConfig;


class LoxLegacyModbusExtension : public LoxLegacyExtension {
  StaticQueue_t txQueue;
  sModbusConfig config;
  int32_t deviceTimeout[254];
  uint32_t characterTime_us; // duration for one character transmission in us
  uint32_t timePause;
  uint32_t timeTimeout;

  void set_tx_mode(bool txMode);
  void config_load(void);
  void rs485_setup(void);
  bool _transmitBuffer(int devIndex, const uint8_t *buffer, size_t byteCount);
  bool transmitBuffer(int devIndex, const uint8_t *buffer, size_t byteCount);
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