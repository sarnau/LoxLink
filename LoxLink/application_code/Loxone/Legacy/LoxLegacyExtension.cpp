//
//  LoxLegacyExtension.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxLegacyExtension.hpp"
#include "LED.hpp"
#include "stm32f1xx_ll_cortex.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/***
 *  Constructor
 ***/
LoxLegacyExtension::LoxLegacyExtension(LoxCANBaseDriver &driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version)
  : LoxExtension(driver, serial, device_type, hardware_version, version), aliveCountdown(0), isMuted(false), forceStartMessage(true), firmwareUpdateActive(false) {
  SetState(eDeviceState_offline);
  gLED.identify_off();
  gLED.blink_red();
}

/***
 *  Send a command to the Miniserver if not muted.
 ***/
void LoxLegacyExtension::sendCommandWithValues(LoxMsgLegacyCommand_t command, uint8_t val8, uint16_t val16, uint32_t val32) {
  if (this->isMuted)
    return;
  LoxCanMessage message;
  message.serial = this->serial;
  message.hardwareType = eDeviceType_t(this->device_type);
  message.directionLegacy = LoxMsgLegacyDirection_t_fromDevice;
  message.commandLegacy = command;
  message.commandDirection = LoxMsgLegacyCommandDirection_t_fromDevice;
  message.value8 = val8;
  message.value16 = val16;
  message.value32 = val32;
  driver.SendMessage(message);
}

/***
 *  Send a fragmented command to the Miniserver
 ***/
void LoxLegacyExtension::send_fragmented_data(LoxMsgLegacyFragmentedCommand_t fragCommand, const void *buffer, uint32_t byteCount) {
  // never send fragmented package if the extension is not active, except for the page CRC command.
  if ((this->state != eDeviceState_online or this->isMuted) and FragCmd_page_CRC_external != fragCommand)
    return;
  LoxCanMessage message;
  message.serial = this->serial;
  message.hardwareType = eDeviceType_t(this->device_type);
  message.directionLegacy = LoxMsgLegacyDirection_t_fromDevice;
  message.commandDirection = LoxMsgLegacyCommandDirection_t_fromDevice;
  if (byteCount > 1530) {      // large fragmented packages
    if (byteCount < 0x10000) { // max. 64kb
      message.commandLegacy = fragmented_package_large_start;
      message.data[0] = 0x00; // unused for the large fragmented package
      message.data[1] = fragCommand;
      message.data[2] = 0x00; // unused
      message.data[3] = byteCount;
      message.data[4] = byteCount >> 8;
      uint16_t checksum = 0x0000;
      for (int i = 0; i < byteCount; ++i)
        checksum += ((uint8_t *)buffer)[i];
      message.data[5] = checksum;
      message.data[6] = checksum >> 8;
      driver.SendMessage(message);
      if (byteCount > 0) {
        message.commandLegacy = fragmented_package_large_data; // 7 bytes per package
        for (uint32_t offset = 0; offset < byteCount; offset += 7) {
          int count = byteCount - offset;
          if (count > 7)
            count = 7;
          memcpy(&message.data[0], ((uint8_t *)buffer) + offset, count);
          driver.SendMessage(message);
        }
      }
    }
  } else { // smaller fragmented package (6 bytes per package * 255 packages = 1530 bytes maximum size)
    message.commandLegacy = fragmented_package;
    message.data[0] = 0x00; // package index 0 = header
    message.data[1] = fragCommand;
    message.data[2] = 0x00; // unused
    message.data[3] = byteCount;
    message.data[4] = byteCount >> 8;
    uint16_t checksum = 0x0000;
    for (int i = 0; i < byteCount; ++i)
      checksum += ((uint8_t *)buffer)[i];
    message.data[5] = checksum;
    message.data[6] = checksum >> 8;
    driver.SendMessage(message);
    if (byteCount > 0) {
      for (uint32_t offset = 0; offset < byteCount; offset += 6) {
        ++message.data[0]; // package index
        int count = byteCount - offset;
        if (count > 6)
          count = 6;
        memcpy(&message.data[1], ((uint8_t *)buffer) + offset, count);
        driver.SendMessage(message);
      }
    }
  }
}

/***
 *  Send a command with version information to the Miniserver
 ***/
void LoxLegacyExtension::sendCommandWithVersion(LoxMsgLegacyCommand_t command) {
  // 0 contains the configuration bitmask, which is used e.g. for the Extension
  // to confirm the complete configuration has arrived.
  sendCommandWithValues(command, this->hardware_version, 0, this->version);
}

/***
 *  10ms Timer to be called 100x per second
 ***/
void LoxLegacyExtension::Timer10ms(void) {
  bool resetAliveCountdown = false;
  if (this->forceStartMessage) { // a start request needed? This happens directly after boot or if requested by the Miniserver
    this->forceStartMessage = false;
    this->isMuted = false;
    sendCommandWithVersion(start_request);
    resetAliveCountdown = true;
  } else if (this->aliveCountdown <= 0) {
    sendCommandWithVersion(alive);
    resetAliveCountdown = true;
  }
  if(resetAliveCountdown)
    this->aliveCountdown = 1000 * ((this->serial & 0x3f) + 6 * 60); // avoid that all alive packages from all extensions are sent at the same time
  this->aliveCountdown -= 10;
}

/***
 *  Multicast to all extensions for broadcast commands
 ***/
void LoxLegacyExtension::PacketMulticastAll(LoxCanMessage &message) {
  switch (message.commandLegacy) {
  case identify_LED:
    gLED.identify_off();
    break;
  case identify_unknown_extensions:
    if (this->state == eDeviceState_parked)
      this->forceStartMessage = true;
    break;
  case extension_offline:
  case park_extension:
    this->isMuted = false;
    SetState(eDeviceState_parked);
    break;
  case sync_ticks:
    gLED.sync(message.value32);
    break;
  default:
    break;
  }
}

/***
 *  Multicast to all extensions of a certain type are used for the software update case only
 ***/
void LoxLegacyExtension::PacketMulticastExtension(LoxCanMessage &message) {
  switch (message.commandLegacy) {
  case software_update_init:
    this->firmwareUpdateActive = false;
    if (message.value8 <= this->hardware_version) {
      if (message.value16 == 0xDEAD or message.value32 != this->version) {
        this->firmwareUpdateActive = true;
        sendCommandWithVersion(BC_ACK);
      } else {
        sendCommandWithVersion(BC_NAK);
      }
    }
    break;
  case reboot_all:
    if (message.value16 == 0xDEAD or message.value32 != this->version) {
      NVIC_SystemReset(); // reboot (with new firmware...)
    }
    this->firmwareUpdateActive = false;
    this->isMuted = false;
    break;
  case software_update_verify:
    if (this->firmwareUpdateActive) {
      this->firmwareNewVersion = message.value32;
      if ((message.value8 == 0 and this->version != this->firmwareNewVersion) or message.value8 == 1) {
        // validate CRCs over the firmware update data
      }
    }
    break;
  case software_update_page_crc:
    if (this->firmwareUpdateActive) {
      if (message.value16 <= sizeof(firmwareUpdateCRCs) / sizeof(firmwareUpdateCRCs[0]))
        this->firmwareUpdateCRCs[message.value16] = message.value32;
    }
    break;
  case mute_all:
    this->isMuted = true;
    break;
  default:
    break;
  }
}

/***
 *  Packages to a specific extension based on it's serial number
 ***/
void LoxLegacyExtension::PacketToExtension(LoxCanMessage &message) {
  switch (message.commandLegacy) {
  case identify: // first direct command from the Miniserver after boot
    this->firmwareUpdateActive = false;
    this->forceStartMessage = true;
    break;
  case identify_LED:
    gLED.identify_on();
    break;
  case alive:
    sendCommandWithVersion(alive_reply);
    break;
  case extension_offline:
  case park_extension:
    SetState(eDeviceState_parked);
    this->isMuted = false;
    break;
  case LED_flash_position:
    gLED.set_sync_offset(message.value32);
    SetState(eDeviceState_online);
    break;
  case LinkDiagnosis_request:
    sendCommandWithValues(LinkDiagnosis_reply, 0, (driver.GetReceiveErrorCounter() & 0x7F) + ((driver.GetTransmitErrorCounter() & 0x7F) << 8), driver.GetErrorCounter());
    break;
  case mute_all:
    this->isMuted = true;
    break;
  default:
    break;
  }
}

/***
 *  Messages on the CAN bus _from_ this extension. These can be ignored, because they come from this extension anyway
 ***/
void LoxLegacyExtension::PacketFromExtension(LoxCanMessage &message) {
}

/***
 *  Packages with firmware update data, sent to all extensions of a certain type.
 ***/
void LoxLegacyExtension::PacketFirmwareUpdate(LoxCanMessage &message) {
}

/***
 *  A message was received. Called from the driver.
 ***/
void LoxLegacyExtension::ReceiveMessage(LoxCanMessage &message) {
  // ignore NAT packages or messages from devices.
  // This is not necessary with a correct CAN filter.
  if (message.isNATmessage(this->driver) or (message.directionLegacy == LoxMsgLegacyDirection_t_fromDevice and message.identifier != 0))
    return;

  // Check for the five different legacy message types:

  // Multicast to all extensions
  if (message.identifier == 0x00000000)
    PacketMulticastAll(message);

  // Multicast to all extensions of a certain type
  else if (message.identifier == (this->device_type << 24))
    PacketMulticastExtension(message);

  // Send to the extension directly
  else if (message.identifier == (this->serial | 0x10000000))
    PacketToExtension(message);

  // Send from the extension (typically ignored)
  else if (message.identifier == this->serial)
    PacketFromExtension(message);

  // Firmware update packet to all extensions of a certain type
  else if ((message.identifier & 0x1FFF0000) == ((this->device_type << 16) | 0x1F000000))
    PacketFirmwareUpdate(message);
}