//
//  LoxCANDriver.cpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright © 2019 Markus Fritze. All rights reserved.
//

#include "LoxCANDriver.hpp"
#include "LoxExtension.hpp"
#include "MMM_can.hpp"
#include <stdio.h>

LoxCANDriver::LoxCANDriver(tLoxCANDriverType type)
    : driverType(type)
    , extensionCount(0)
{
}

tLoxCANDriverType LoxCANDriver::GetDriverType() const
{
    return this->driverType;
}

void LoxCANDriver::AddExtension(LoxExtension* extension)
{
    if (this->extensionCount == sizeof(this->extensions) / sizeof(this->extensions[0]))
        return;
    this->extensions[this->extensionCount++] = extension;
}

void LoxCANDriver::SetupCANFilter(int filterIndex, LoxCmdNATBus_t busType, uint8_t extensionNAT)
{
    LoxCanMessage msg;
    msg.busType = busType;
    msg.directionNat = LoxCmdNATDirection_t_fromServerShortcut;
    msg.extensionNat = extensionNAT;
    printf("Filter #%d mask:%08x value:%08x\n", filterIndex, 0x1F2FF000, msg.identifier);
}

uint8_t LoxCANDriver::GetTransmitErrorCounter() const
{
    return 0;
}

uint8_t LoxCANDriver::GetReceiveErrorCounter() const
{
    return 0;
}

void LoxCANDriver::Delay(int msDelay) const
{
  vTaskDelay(pdMS_TO_TICKS(msDelay));
}

void LoxCANDriver::SendMessage(LoxCanMessage& message)
{
    MMM_CAN_Send(message);
}

void LoxCANDriver::ReceiveMessage(LoxCanMessage& message)
{
    for (int i = 0; i < this->extensionCount; ++i)
        this->extensions[i]->ReceiveMessage(message);
}
