//
//  LoxCANBaseDriver.cpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxCANBaseDriver.hpp"
#include "LoxExtension.hpp"
#if DEBUG
#include <__cross_studio_io.h>
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
  for (int i = 0; i < this->extensionCount; ++i)
    this->extensions[i]->Startup();
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
#if DEBUG && 0
  debug_printf("Filter #%d mask:%08x value:%08x\n", filterIndex, 0x1F2FF000, msg.identifier);
#endif
}

/***
 *  CAN error reporting and statistics
 ***/
#if DEBUG
void LoxCANBaseDriver::StatisticsPrint() const {
  debug_printf("Sent:%d;", this->statistics.Sent);
  debug_printf("Rcv:%d;", this->statistics.Rcv);
  debug_printf("Err:%d;", this->statistics.Err);
  debug_printf("REC:%d;", this->GetReceiveErrorCounter());
  debug_printf("TEC:%d;", this->GetTransmitErrorCounter());
  debug_printf("HWE:%d;", this->statistics.HWE);
  debug_printf("TQ:%d;", this->statistics.TQ);
  debug_printf("mTQ:%d;", this->statistics.mTQ);
  debug_printf("QOvf:%d;", this->statistics.QOvf);
  debug_printf("RQ:%d;", this->statistics.RQ);
  debug_printf("mRQ:%d;\n", this->statistics.mRQ);
}
#endif

void LoxCANBaseDriver::StatisticsReset() {
  memset(&this->statistics, 0, sizeof(this->statistics));
}

/***
 *  A ms delay, implemented via RTOS
 ***/
void LoxCANBaseDriver::Delay(CTL_TIME_t msDelay) const {
  ctl_timeout_wait(ctl_get_current_time() + msDelay + 1); // +1 to always round-up
}

/*** 
 *  Received a message
 ***/
void LoxCANBaseDriver::ReceiveMessage(LoxCanMessage &message) {
  ++this->statistics.Rcv;
#if DEBUG && 1
  debug_printf("CANR:");
  message.print(*this);
#endif
  for (int i = 0; i < this->extensionCount; ++i) {
    this->extensions[i]->ReceiveMessage(message);
  }
}

/***
 *  Forward a 10ms heartbeat to all extensions
 ***/
void LoxCANBaseDriver::Timer10ms(void)
{
  for (int i = 0; i < this->extensionCount; ++i)
    this->extensions[i]->Timer10ms();
}