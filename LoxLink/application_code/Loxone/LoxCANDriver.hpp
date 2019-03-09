//
//  LoxCANDriver.hpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxCANDriver_hpp
#define LoxCANDriver_hpp

#include "FreeRTOS.h"
#include "LoxCanMessage.hpp"
#include "queue.h"
#include "stm32f1xx_hal_conf.h"

class LoxExtension;

typedef enum {
  tLoxCANDriverType_LoxoneLink,
  tLoxCANDriverType_TreeBus,
} tLoxCANDriverType;

class LoxCANDriver {
  tLoxCANDriverType driverType;
  int extensionCount;
  LoxExtension *extensions[16]; // up to 16 extensions per driver

  StaticQueue_t transmitQueue;
  static void vCANRXTask(void *pvParameters);
  static void vCANTXTask(void *pvParameters);

 public:
  struct {         // CAN bus statistics
    uint32_t Rcv;  // number of received CAN bus packages
    uint32_t Sent; // number of sent CAN messages
    uint32_t RQ;   // number of entries in the receive queue
    uint32_t mRQ;  // maximum number of entries in the receive queue
    uint32_t TQ;   // number of entries in the transmit queue
    uint32_t mTQ;  // maximum number of entries in the transmit queue
    uint32_t QOvf; // number of dropped packages, because the transmit queue was full
    uint32_t Err;  // incremented, whenever the CAN Last error code was != 0
    uint32_t HWE;  // Hardware error: incremented, whenever the Error Passive limit has been reached (Receive Error Counter or Transmit Error Counter>127).
  } statistics;

public: // internal method
  static void CANSysTick(void);

public:
  LoxCANDriver(tLoxCANDriverType type);
  void Startup(void);

  tLoxCANDriverType GetDriverType() const;

  // add an extension to the driver
  void AddExtension(LoxExtension *);

  // setup various CAN filters. At least one is required to receive messages!
  void FilterAllowAll(uint32_t filterBank);
  void FilterSetup(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment);
  void FilterSetupNAT(int filterIndex, LoxCmdNATBus_t busType, uint8_t extensionNAT);

  // CAN bus statistics and errors
  void StatisticsPrint() const;
  void StatisticsReset();
  uint32_t GetErrorCounter() const;
  uint8_t GetTransmitErrorCounter() const;
  uint8_t GetReceiveErrorCounter() const;

  // a ms delay, uses FreeRTOS
  void Delay(int msDelay) const;

  // send a message onto the CAN bus
  void SendMessage(LoxCanMessage &message);
};

#endif /* LoxCANDriver_hpp */