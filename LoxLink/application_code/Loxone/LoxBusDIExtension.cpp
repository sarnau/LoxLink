//
//  LoxBusDIExtension.cpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright © 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusDIExtension.hpp"
#include "global_functions.hpp"
#include <stdio.h>

LoxBusDIExtension::LoxBusDIExtension(LoxCANDriver& driver, uint32_t serial)
    : LoxNATExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_DIExtension << 24), eDeviceType_t_DIExtension, 0, 9021122, 1, sizeof(config), &config)
    , hardwareBitmask(0)
    , lastBitmaskTime(0)
    , lastFrequencyTime(0)
{
}

/***
 *  Send current status back. This happens on value changes and typically on being assigned a NAT,
 *  to provide the Miniserver with the current value after a reboot.
 ***/
void LoxBusDIExtension::SendValues()
{
    send_digital_value(0, this->hardwareBitmask);
}

/***
 *  A new configuration has been uploaded. Here the extension need to get reconfigured accordingly.
 ***/
void LoxBusDIExtension::ConfigUpdate(void)
{
    printf("Config updated: 0x%04x\n", config.frequencyInputsBitmask);
}

/***
 *  10ms Timer to be called 100x per second
 ***/
void LoxBusDIExtension::Timer10ms(void)
{
    LoxNATExtension::Timer10ms();

    // simulate the inputs changing every second. They are sent back on every value change,
    // but not faster than 20ms (= 50Hz)
    this->lastBitmaskTime += 10;
    if (this->lastBitmaskTime >= 100) {
        this->lastBitmaskTime = 0;
        this->hardwareBitmask = this->hardwareBitmask + 1;
        send_digital_value(0, this->hardwareBitmask);
    }

    // the frequency is sent every second
    this->lastFrequencyTime += 10;
    if (this->lastFrequencyTime >= 100) {
        this->lastFrequencyTime = 0;
        // and send a test frequency (between 0…100Hz) on input 1
        send_frequency_value(0, (random_range(0, 150) / 10) * 10);
    }
}

#if 0
void DigitalInputs_10ms_callback_send_value(void)
{
    if (gMillisecondTimerFrequency >= 1000) { // frequencies are sent once per second
        gMillisecondFrequency = 0;
        LoxDigitalInput *di = gDI_currentStatus;
        for (int index = 0; index < 20; ++index, ++di) {
            if (!di->isImpulseCounter) // ignore all non frequency counters
                continue;
            uint32_t freq = di->impulseValuePerSecond;
            if (freq) { // a valid frequency?
                lox_send_frequency_value(freq, index);
                di->isCurrentlyOff = 0;
            } else if (!di->isCurrentlyOff) { // only once send the 0Hz as a sign for no signal
                di->isCurrentlyOff = 1;
                lox_send_frequency_value(0, index);
            }
        }
    }
    gMillisecondFrequency += 10;

    if (gMillisecondSendValueTimer == 20 * (gMillisecondSendValueTimer / 20)) // Throttle to 50Hz
    {
        // only transmit, if the value changed
        if(gDI_CurrentBitmask != gDI_LastBitmask) {
            gDI_LastBitmask = gDI_CurrentBitmask;
            lox_send_digital_input(gDI_CurrentBitmask);
            gMillisecondSendValueTimer = 0;
        }
    }
    gMillisecondSendValueTimer += 10;
}

void DigitalInputs_TIM3_Handler(void) // called at 1000Hz
{
    if (TIM_GetITStatus(&TIM3_BASE, TIM_IT_Update)) {
        TIM_ClearITPendingBit(&TIM3_BASE, TIM_IT_Update);

        LoxDigitalInput *di = gDI_currentStatus;
        for (int index = 0; index < 20; ++index, ++di) {
            bool inputStatus = gpio_read_bit(di->registerBitmask);

            // has the bit changed?
            if (di->currentStatus != inputStatus) {
                di->currentStatus = inputStatus;

                // is the input an impulse counter?
                if (di->isImpulseCounter) {
                    if (inputStatus) // count only raising flanks
                        ++di->impulseCounter;
                } else {
                    // a regular input, we just record the current bitstatus
                    if (inputStatus)
                        gDI_CurrentBitmask |= 1 << index;
                    else
                        gDI_CurrentBitmask &= ~(1 << index);
                }
            }
            // once per second, the frequency counter bit gets stored for transmission
            if (di->isImpulseCounter && (gMicrosecondTimer % 1000000) == 0) {
                di->impulseValuePerSecond = di->impulseCounter;
                di->impulseCounter = 0;
            }
        }
        gMicrosecondTimer = (gMicrosecondTimer + 1000) % 1000000;
    }
}
#endif
