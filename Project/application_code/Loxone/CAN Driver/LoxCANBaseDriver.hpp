//
//  LoxCANBaseDriver.hpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxCANBaseDriver_hpp
#define LoxCANBaseDriver_hpp

#include "LoxCanMessage.hpp"
#include <ctl_api.h>

class LoxExtension;

typedef enum {
  tLoxCANDriverType_LoxoneLink,
  tLoxCANDriverType_TreeBus,
} tLoxCANDriverType;

class LoxCANBaseDriver {
  tLoxCANDriverType driverType;
  int extensionCount;
  LoxExtension *extensions[16]; // up to 16 extensions per driver

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

public:
  LoxCANBaseDriver(tLoxCANDriverType type);
  virtual void Startup(void);

  //tLoxCANDriverType GetDriverType() const;
  bool isTreeBusDriver() const { return this->driverType == tLoxCANDriverType_TreeBus; };
  bool isLoxoneLinkBusDriver() const { return this->driverType == tLoxCANDriverType_LoxoneLink; };

  // add an extension to the driver
  void AddExtension(LoxExtension *driver);

  // setup various CAN filters. At least one is required to receive messages!
  virtual void FilterAllowAll(uint32_t filterBank) = 0;
  virtual void FilterSetup(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment) = 0;
  void FilterSetupNAT(int filterIndex, LoxCmdNATBus_t busType, uint8_t extensionNAT);

  // CAN bus statistics and errors
#if DEBUG
  void StatisticsPrint() const;
#endif
  void StatisticsReset();
  virtual uint32_t GetErrorCounter() const = 0;
  virtual uint8_t GetTransmitErrorCounter() const = 0;
  virtual uint8_t GetReceiveErrorCounter() const = 0;

  // a ms delay, uses FreeRTOS
  void Delay(CTL_TIME_t msDelay) const;

  // send a message onto the CAN bus
  virtual void SendMessage(LoxCanMessage &message) = 0;

  // received a message from the CAN bus and forward it to the extensions
  void ReceiveMessage(LoxCanMessage &message);

  // forward 10ms timer heartbeat to extensions
  void Heartbeat(void);
};

#endif /* LoxCANBaseDriver_hpp */