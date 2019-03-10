//
//  LoxLegacyRelayExtension.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxLegacyRelayExtension.hpp"

/***
 *  Update the relays
 ***/
void LoxLegacyRelayExtension::update_relays(uint16_t bitmask) {
  if (this->shutdownFlag)
    bitmask = 0;
  this->harewareDigitalOutBitmask = bitmask;
  printf("### Relay Status 0x%x\n", this->harewareDigitalOutBitmask);
}

/***
 *  Constructor
 ***/
LoxLegacyRelayExtension::LoxLegacyRelayExtension(LoxCANDriver &driver, uint32_t serial)
  : LoxLegacyExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_RelayExtension << 24), eDeviceType_t_RelayExtension, 2, 9000822), harewareDigitalOutBitmask(0), forceSendTemperature(false), shutdownFlag(false) {
}

/***
 *  10ms Timer to be called 100x per second
 ***/
void LoxLegacyRelayExtension::Timer10ms(void) {
  bool doSend = this->aliveCountdown <= 0 or this->forceSendTemperature;
  LoxLegacyExtension::Timer10ms();
  if (not this->isMuted and doSend) {
    this->forceSendTemperature = false;
    // convert temperature in Celsius into Luminary System Temperature (as returned by the ADC in the CPU)
    float temperature = 70.3;
    uint32_t value = ((1475 - (temperature * 10)) * 1024) / 2245;
    // Reverse conversion: tempC = (1475-(value*2245/1024))/10
    sendCommandWithValues(system_temperature, 0x00, this->shutdownFlag << 8, value);
  }
}

void LoxLegacyRelayExtension::PacketToExtension(LoxCanMessage &message) {
  switch (message.commandLegacy) {
  case digital_output_value:
    update_relays(message.value32);
    // confirm that we received the command
    sendCommandWithValues(digital_output_value, 0, 0, this->harewareDigitalOutBitmask);
    break;
  case LED_flash_position: // force send the temperature after reboot
    this->forceSendTemperature = true;
    LoxLegacyExtension::PacketToExtension(message);
    break;
  default:
    LoxLegacyExtension::PacketToExtension(message);
    break;
  }
}