//
//  LoxBusTreeExtensionCANDriver.cpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeExtensionCANDriver.hpp"
#include "LoxBusTreeExtension.hpp"
#include <stdio.h>

LoxBusTreeExtensionCANDriver::LoxBusTreeExtensionCANDriver(LoxBusTreeExtension *parentTreeExtension, eTreeBranch treeBranch) : LoxCANBaseDriver(tLoxCANDriverType_TreeBus), parentTreeExtension(parentTreeExtension), treeBranch(treeBranch) {
}

/***
 *  CAN error reporting and statistics
 ***/
uint32_t LoxBusTreeExtensionCANDriver::GetErrorCounter() const {
  return 0; // never any errors
}

uint8_t LoxBusTreeExtensionCANDriver::GetTransmitErrorCounter() const {
  return 0; // never any errors
}

uint8_t LoxBusTreeExtensionCANDriver::GetReceiveErrorCounter() const {
  return 0; // never any errors
}

/***
 *  Send the message from the device back to the Tree Base Extension
 ***/
void LoxBusTreeExtensionCANDriver::SendMessage(LoxCanMessage &message) {
    this->parentTreeExtension->from_treebus_to_loxonelink(this->treeBranch, message);
}