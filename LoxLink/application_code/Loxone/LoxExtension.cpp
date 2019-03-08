//
//  LoxExtension.cpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright © 2019 Markus Fritze. All rights reserved.
//

#include "LoxExtension.hpp"
#include "LED.hpp"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/***
 *  Update the extension state
 ***/
void LoxExtension::SetState(eDeviceState state)
{
    if (state != this->state) {
        switch (state) {
        case eDeviceState_offline:
            gLED.blink_red();
            break;
        case eDeviceState_parked:
            gLED.blink_orange();
            break;
        case eDeviceState_online:
            gLED.blink_green();
            break;
        }
        this->state = state;
    }
}

LoxExtension::LoxExtension(LoxCANDriver& driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version)
    : driver(driver)
    , serial(serial)
    , device_type(device_type)
    , hardware_version(hardware_version)
    , version(version)
    , state(eDeviceState(-1)) // illegal state to force the SetState() to update
{
    assert(serial != 0);
    printf("LoxExtension(%07x,%04x,%d,%d)\n", this->serial, this->device_type, this->hardware_version, this->version);
    SetState(eDeviceState_offline);
    driver.AddExtension(this);
}

void LoxExtension::StatisticsPrint() const
{
    printf("Sent:%d;", this->statistics.Sent);
    printf("Rcv:%d;", this->statistics.Rcv);
    printf("Err:%d;", this->statistics.Err);
    printf("REC:%d;", this->driver.GetReceiveErrorCounter());
    printf("TEC:%d;", this->driver.GetTransmitErrorCounter());
    printf("HWE:%d;", this->statistics.HWE);
    printf("TQ:%d;", this->statistics.TQ);
    printf("mTQ:%d;", this->statistics.mTQ);
    printf("QOvf:%d;", this->statistics.QOvf);
    printf("RQ:%d;", this->statistics.RQ);
    printf("mRQ:%d;\n", this->statistics.mRQ);
}

void LoxExtension::StatisticsReset()
{
    memset(&this->statistics, 0, sizeof(this->statistics));
}
