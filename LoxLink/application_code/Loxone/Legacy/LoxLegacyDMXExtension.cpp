//
//  LoxLegacyDMXExtension.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxLegacyDMXExtension.hpp"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint8_t Type;
  uint8_t Slewrate;
  uint16_t Channel;
  uint8_t Data[4];
} DMX_Actor;

typedef struct {
  DMX_Actor actor;
  uint32_t DeviceId;
} DMX_Dimming;

typedef struct {
  DMX_Actor actor;
  uint16_t Time;
  uint16_t unused;
} DMX_Composite;

typedef struct {
  uint8_t Slewrate[4];
  uint8_t Gamma[4];
  uint8_t RGBW[4];
  uint16_t Channel;
  uint32_t DeviceId;
} DMX_Init_RDM;

/***
 *  Constructor
 ***/
LoxLegacyDMXExtension::LoxLegacyDMXExtension(LoxCANBaseDriver &driver, uint32_t serial)
  : LoxLegacyExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_DMXExtension << 24), eDeviceType_t_DMXExtension, 0, 9000915, fragData, sizeof(fragData)) {
}

/***
 *  Send a reply after a device search
 ***/
void LoxLegacyDMXExtension::search_reply(uint16_t channel, uint16_t manufacturerID, uint32_t deviceID, bool isLast) {
  if (manufacturerID == 0 and deviceID == 0) {
    sendCommandWithValues(dmx_search_last_reply_high, 0, 0, 0);
  } else {
    LoxMsgLegacyCommand_t command = isLast
                                      ? (channel < 0x100 ? dmx_search_last_reply_low : dmx_search_last_reply_high)
                                      : (channel < 0x100 ? dmx_search_reply_low : dmx_search_reply_high);
    sendCommandWithValues(command, deviceID & 0xff, (deviceID >> 8) & 0xFFFF, ((deviceID >> 24) & 0xff) | (manufacturerID << 8) | ((channel & 0xFF) << 24));
  }
}

void LoxLegacyDMXExtension::PacketToExtension(LoxCanMessage &message) {
  switch (message.commandLegacy) {
  case dmx_search:
    printf("Search for DMX devices\n");
    if (true) {
      search_reply(0, 0x2637, 0xE0D80800, false); // 0xDxxxxxxx doesn't support Slewrate and it will be set to 0. 0xExxxxxxx works fine.
      search_reply(4, 0x5431, 0xaabbccdd, true);
    } else {
      search_reply(0, 0, 0, true); // no devices found
    }
    break;
  case DMX_learn:
    printf("Learn DMX: %d %d\n", message.data[0] | (message.data[1] << 8), message.data[2]);
    break;
  default:
    LoxLegacyExtension::PacketToExtension(message);
    break;
  }
}

const char *const LoxLegacyDMXExtension::DMX_DeviceTypeString(uint8_t type) const {
  switch (type) {
  case 0:
    return "Standard";
  case 1:
    return "Standard RGB";
  case 2:
    return "Standard RGBW";
  case 3:
    return "Standard Lumitech";
  case 8:
    return "Smart";
  case 9:
    return "Smart RGB";
  case 10:
    return "Smart RGBW";
  case 11:
    return "Smart Lumitech";
  }
  return "Unknown";
}

void LoxLegacyDMXExtension::FragmentedPacketToExtension(LoxMsgLegacyFragmentedCommand_t fragCommand, const void *fragData, int size) {
  // WARNING The Lumitech case is very different compared to RGB or RGBW:
  // data[0] =   0  => Lumitech Dual White
  // data[1] = Brightness in % (0-255 => 0-100%)
  // data[2] = Color temperature in Kevin (0-255 => 2700K-6500K)
  // data[3] = unused

  // data[0] = 101  => Lumitech RGB
  // data[1] = Red in % (0-255 => 0-100%)
  // data[2] = Blue in % (0-255 => 0-100%)
  // data[3] = Green in % (0-255 => 0-100%)

  // Gamma == is the perception correction in the Loxone Config. It means that the output value is squared before transmission.
  switch (fragCommand) {
  case FragCmd_DMX_actor: {
    const DMX_Actor *package = (const DMX_Actor *)fragData;
    const char *const gammaStr = ((package->Slewrate & 0x80) == 0x80) ? "yes" : "no";
    printf("DMX actor: Type:%s, Slewrate: %d%%, Gamma:%s, Channel:%d, Data:%d %d %d %d\n", DMX_DeviceTypeString(package->Type), package->Slewrate & 0x7F, gammaStr, package->Channel, package->Data[0], package->Data[1], package->Data[2], package->Data[3]);
    break;
  }
  case FragCmd_DMX_dimming: {
    const DMX_Dimming *package = (const DMX_Dimming *)fragData;
    const char *const gammaStr = ((package->actor.Slewrate & 0x80) == 0x80) ? "yes" : "no";
    printf("DMX dimming: Type:%s, Slewrate: %d%%, Gamma:%s, Channel:%d, Data:%d %d %d %d, DeviceId:0x%08x\n", DMX_DeviceTypeString(package->actor.Type), package->actor.Slewrate & 0x7F, gammaStr, package->actor.Channel, package->actor.Data[0], package->actor.Data[1], package->actor.Data[2], package->actor.Data[3], package->DeviceId);
    break;
  }
  case FragCmd_DMX_composite_actor: {
    const DMX_Composite *package = (const DMX_Composite *)fragData;
    const char *const gammaStr = ((package->actor.Slewrate & 0x80) == 0x80) ? "yes" : "no";
    uint16_t timeInMs = package->Time * 100; // time in 100ms units
    if (timeInMs & 0x4000)                   // time in seconds?
      timeInMs *= 10;
    const char *const percentStr = ((package->Time & 0x8000) == 0x8000) ? "/100%" : "";
    if (package->actor.Type == 11) {       // Lumitech
      if (package->actor.Data[0] == 101) { // RGB
        printf("DMX composite: Type:%s, Slewrate: %d%%, Gamma:%s, Channel:%d, RGB:%.1f%% %.1f%% %.1f%%, Time:%dms%s\n", DMX_DeviceTypeString(package->actor.Type), package->actor.Slewrate & 0x7F, gammaStr, package->actor.Channel, package->actor.Data[1] * 100.0 / 255, package->actor.Data[2] * 100.0 / 255, package->actor.Data[3] * 100.0 / 255, timeInMs, percentStr);
      } else {
        printf("DMX composite: Type:%s, Slewrate: %d%%, Gamma:%s, Channel:%d, Dual White:%.1f%% %dK, Time:%dms%s\n", DMX_DeviceTypeString(package->actor.Type), package->actor.Slewrate & 0x7F, gammaStr, package->actor.Channel, package->actor.Data[1] * 100.0 / 255, (package->actor.Data[2] * (6500 - 2700)) / 255 + 2700, timeInMs, percentStr);
      }
    } else {
      printf("DMX composite: Type:%s, Slewrate: %d%%, Gamma:%s, Channel:%d, Data:%.1f%% %.1f%% %.1f%% %.1f%%, Time:%dms%s\n", DMX_DeviceTypeString(package->actor.Type), package->actor.Slewrate & 0x7F, gammaStr, package->actor.Channel, package->actor.Data[0] * 100.0 / 255, package->actor.Data[1] * 100.0 / 255, package->actor.Data[2] * 100.0 / 255, package->actor.Data[3] * 100.0 / 255, timeInMs, percentStr);
    }
    break;
  }
  case FragCmd_DMX_init_rdm_device: {
    const DMX_Init_RDM *package = (const DMX_Init_RDM *)fragData;
    printf("DMX RDM Init: Slewrate:%d%% %d%% %d%% %d%%, Gamma:%d %d %d %d, RGBW:%d %d %d %d, Channel:%d, DeviceId:0x%08x\n",
      package->Slewrate[0], package->Slewrate[1], package->Slewrate[2], package->Slewrate[3],
      package->Gamma[0], package->Gamma[1], package->Gamma[2], package->Gamma[3],
      package->RGBW[0], package->RGBW[1], package->RGBW[2], package->RGBW[3],
      package->Channel, package->DeviceId);
    break;
  }
  default:
    break;
  }
}

void LoxLegacyDMXExtension::StartRequest()
{
    sendCommandWithValues(config_checksum, 0, 0, 0); // required, otherwise it is considered offline
}
