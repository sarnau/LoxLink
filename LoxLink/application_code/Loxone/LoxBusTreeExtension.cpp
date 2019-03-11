//
//  LoxBusTreeExtension.cpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeExtension.hpp"
#include "global_functions.hpp"
#include <stdio.h>

LoxBusTreeExtension::LoxBusTreeExtension(LoxCANDriver &driver, uint32_t serial, eAliveReason_t alive)
  : LoxNATExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_TreeBaseExtension << 24), eDeviceType_t_TreeBaseExtension, 0, 9030305, 1, sizeof(config), &config, alive) {
}

/***
 *  A direct message received
 ***/
void LoxBusTreeExtension::ReceiveDirect(LoxCanMessage &message) {
  if (message.deviceNAT != 0) { // forward to a tree device?
    // TODO
  } else {
    LoxCanMessage msg;
    switch (message.commandNat) {
    case CAN_Diagnosis_Request:
      if (message.value16 != 0) { // for this device (!=0 => for a Tree bus)
        send_can_status(CAN_Diagnosis_Reply, eTreeBranch(message.value16));
      }
      break;
    case CAN_Error_Request:
      if (message.value16 != 0) { // for this device (!=0 => for a Tree bus)
        send_can_status(CAN_Error_Reply, eTreeBranch(message.value16));
      }
      break;
    default:
      break;
    }
    LoxNATExtension::ReceiveDirect(message);
  }
}