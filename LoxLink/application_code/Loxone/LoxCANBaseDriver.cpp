//
//  LoxCANBaseDriver.cpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxCANBaseDriver.hpp"
#include "FreeRTOS.h"
#include "LoxExtension.hpp"
#include "task.h"
#if DEBUG
#include <stdio.h>
#endif
#include <string.h>

/***
 *  constructor
 ***/
LoxCANBaseDriver::LoxCANBaseDriver(tLoxCANDriverType type) : driverType(type), extensionCount(0) {
}

/***
 *  Initialize the CAN bus and all tasks, etc
 ***/
void LoxCANBaseDriver::Startup(void) {
}

/***
 *  Loxone Link or Tree Bus
 ***/
tLoxCANDriverType LoxCANBaseDriver::GetDriverType() const {
  return this->driverType;
}

/***
 *  Add an extension to this driver
 ***/
void LoxCANBaseDriver::AddExtension(LoxExtension *extension) {
  if (this->extensionCount == sizeof(this->extensions) / sizeof(this->extensions[0]))
    return;
  this->extensions[this->extensionCount++] = extension;
}

/***
 *  Setup a NAT filter
 ***/
void LoxCANBaseDriver::FilterSetupNAT(int filterIndex, LoxCmdNATBus_t busType, uint8_t extensionNAT) {
  LoxCanMessage msg;
  msg.busType = busType;
  msg.directionNat = LoxCmdNATDirection_t_fromServer;
  msg.extensionNat = extensionNAT;
  FilterSetup(filterIndex, msg.identifier, 0x1F2FF000, 0);
#if DEBUG
  printf("Filter #%d mask:%08x value:%08x\n", filterIndex, 0x1F2FF000, msg.identifier);
#endif
}

/***
 *  CAN error reporting and statistics
 ***/
#if DEBUG
void LoxCANBaseDriver::StatisticsPrint() const {
  printf("Sent:%d;", this->statistics.Sent);
  printf("Rcv:%d;", this->statistics.Rcv);
  printf("Err:%d;", this->statistics.Err);
  printf("REC:%d;", this->GetReceiveErrorCounter());
  printf("TEC:%d;", this->GetTransmitErrorCounter());
  printf("HWE:%d;", this->statistics.HWE);
  printf("TQ:%d;", this->statistics.TQ);
  printf("mTQ:%d;", this->statistics.mTQ);
  printf("QOvf:%d;", this->statistics.QOvf);
  printf("RQ:%d;", this->statistics.RQ);
  printf("mRQ:%d;\n", this->statistics.mRQ);
}
#endif

void LoxCANBaseDriver::StatisticsReset() {
  memset(&this->statistics, 0, sizeof(this->statistics));
}

/***
 *  A ms delay, implemented via RTOS
 ***/
void LoxCANBaseDriver::Delay(int msDelay) const {
  vTaskDelay(pdMS_TO_TICKS(msDelay));
}

/***
 *  Received a message
 ***/
void LoxCANBaseDriver::ReceiveMessage(LoxCanMessage &message) {
  ++this->statistics.Rcv;
#if DEBUG
  printf("CANR:");
  message.print(*this);
#endif
  for (int i = 0; i < this->extensionCount; ++i) {
    this->extensions[i]->ReceiveMessage(message);
  }
}

/***
 *  Forward a 10ms heartbeat to all extensions
 ***/
void LoxCANBaseDriver::Heartbeat(void)

{
  for (int i = 0; i < this->extensionCount; ++i)
    this->extensions[i]->Timer10ms();
}