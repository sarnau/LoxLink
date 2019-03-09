//
//  MMM_LoxCanMessage.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxCanMessage.hpp"
#include "LoxCANDriver.hpp"
#include <assert.h>
#include <stdio.h>

bool LoxCanMessage::isNATmessage(LoxCANDriver &driver) const {
  return (tLoxCANDriverType_LoxoneLink == driver.GetDriverType() && this->busType == LoxCmdNATBus_t_LoxoneLink) || (tLoxCANDriverType_TreeBus == driver.GetDriverType() && this->busType == LoxCmdNATBus_t_TreeBus);
}

void LoxCanMessage::print(LoxCANDriver &driver) const {
  assert(sizeof(LoxCanMessage) == 12); //, "LoxCanMessage size wrong");

  printf("LoxCanMessage:0x%x ", this->identifier);
  if (this->isNATmessage(driver)) {
    printf("Dir:%d ", this->directionNat);
    printf("Frag:%d ", this->fragmented);
    printf("NAT:%02x/%02x ", this->extensionNat, this->deviceNAT);
    printf("Cmd:%02x ", this->commandNat);
  } else {
    printf("Dir:%d ", this->directionLegacy);
    printf("Hw:%02x ", this->hardwareType);
    printf("Serial:%06x ", this->serial);
    printf("Cmd:%d/%02x ", this->commandDirection, this->commandLegacy);
  }
  printf("%02x.%02x.%02x.%02x.%02x.%02x.%02x\n", this->data[0], this->data[1], this->data[2], this->data[3], this->data[4], this->data[5], this->data[6]);
}