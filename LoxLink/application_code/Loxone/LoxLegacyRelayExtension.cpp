//
//  LoxLegacyRelayExtension.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxLegacyRelayExtension.hpp"
#include "system.hpp"
#include <stdlib.h>

/***
 *  Update the relays
 ***/
void LoxLegacyRelayExtension::update_relays(uint16_t bitmask) {
  if (this->temperatureOverheatingFlag)
    bitmask = 0;
  this->harewareDigitalOutBitmask = bitmask;
//  printf("### Relay Status 0x%x\n", this->harewareDigitalOutBitmask);
}

/***
 *  Constructor
 ***/
LoxLegacyRelayExtension::LoxLegacyRelayExtension(LoxCANDriver &driver, uint32_t serial)
  : LoxLegacyExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_RelayExtension << 24), eDeviceType_t_RelayExtension, 2, 9000822), harewareDigitalOutBitmask(0), temperatureForceSend(false), temperatureOverheatingFlag(false), temperatureMsTimer(0), temperature(0) {
}

/***
 *  10ms Timer to be called 100x per second
 ***/
void LoxLegacyRelayExtension::Timer10ms(void) {
  bool doSend = this->aliveCountdown <= 0 or this->temperatureForceSend;
  LoxLegacyExtension::Timer10ms();
  this->temperatureMsTimer += 10;
  if (this->temperatureMsTimer >= 1000 or doSend) { // once per second
    this->temperatureMsTimer = 0;
    float temperature = MX_read_temperature();
    if (temperature >= 87) { // too hot?
      this->temperatureOverheatingFlag = true;
    } else if (temperature < 72) {          // cooled down enough to get out of shutdown mode?
      if (this->temperatureOverheatingFlag) // were we in overheating mode and now its fine again?
        NVIC_SystemReset();                 // then just reboot the extension
    }
    // did the temperature change a lot or is this a force/regular update?
    if (abs(int(temperature - this->temperature)) >= 5 or doSend) {
      this->temperature = temperature;
      this->temperatureForceSend = false;
      // https://www.st.com/content/ccc/resource/technical/document/application_note/b9/21/44/4e/cf/6f/46/fa/DM00035957.pdf/files/DM00035957.pdf/jcr:content/translations/en.DM00035957.pdf
      // https://electronics.stackexchange.com/questions/324321/reading-internal-temperature-sensor-stm32
      // convert temperature in Celsius into Luminary System Temperature (as returned by the ADC in the CPU)

      // hardware version < 2 only sends the luminary system temperature from STM32
      // starting with hardware version 2, two options are supported:
      // value8 == 0: value32 = temperature in Celcius * 10
      // value8 == 1: value32 = luminary system temperature

      const bool sendTempInCelcius = true;
      if (sendTempInCelcius) {
        sendCommandWithValues(system_temperature, 0, this->temperatureOverheatingFlag << 8, temperature * 10);
      } else {
        sendCommandWithValues(system_temperature, 1, this->temperatureOverheatingFlag << 8, ((1475 - (temperature * 10)) * 1024) / 2245);
        // Reverse conversion: tempC = (1475-(value*2245/1024))/10
      }
      // If the unit is overheating, turn the relays off
      if (this->temperatureOverheatingFlag) {
        update_relays(0);
      }
    }
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
    this->temperatureForceSend = true;
    LoxLegacyExtension::PacketToExtension(message);
    break;
  default:
    LoxLegacyExtension::PacketToExtension(message);
    break;
  }
}