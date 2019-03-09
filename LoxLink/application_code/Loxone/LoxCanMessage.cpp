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

#if DEBUG

const char *const LoxCanMessage::NATcommandString(LoxMsgNATCommand_t command) const {
  switch (command) {
  case Version_Request:
    return "Version_Request";
  case Start:
    return "Start";
  case Device_Version:
    return "Device_Version";
  case Config_Equal:
    return "Config_Equal";
  case Ping:
    return "Ping";
  case Pong:
    return "Pong";
  case Park_Devices:
    return "Park_Devices";
  case Alive_Packet:
    return "Alive_Packet";
  case Sync_Packet:
    return "Sync_Packet";
  case Identify_LED:
    return "Identify_LED";
  case Config_Data:
    return "Config_Data";
  case WebServicesText:
    return "WebServicesText";
  case DeviceLog:
    return "DeviceLog";
  case CAN_Diagnosis_Reply:
    return "CAN_Diagnosis_Reply";
  case CAN_Diagnosis_Request:
    return "CAN_Diagnosis_Request";
  case CAN_Error_Reply:
    return "CAN_Error_Reply";
  case CAN_Error_Request:
    return "CAN_Error_Request";
  case Tree_Shortcut:
    return "Tree_Shortcut";
  case Tree_Shortcut_Test:
    return "Tree_Shortcut_Test";
  case KNX_Send_Telegram:
    return "KNX_Send_Telegram";
  case KNX_Group_Address_Config:
    return "KNX_Group_Address_Config";
  case Digital_Value:
    return "Digital_Value";
  case Analog_Value:
    return "Analog_Value";
  case RGBW:
    return "RGBW";
  case Frequency:
    return "Frequency";
  case AccessCodeInput:
    return "AccessCodeInput";
  case Keypad_NfcId:
    return "Keypad_NfcId";
  case Composite_RGBW:
    return "Composite_RGBW";
  case TreeKeypad_Send:
    return "TreeKeypad_Send";
  case Composite_White:
    return "Composite_White";
  case CryptoValueDigital:
    return "CryptoValueDigital";
  case CryptoValueAnalog:
    return "CryptoValueAnalog";
  case CryptoValueAccessCodeInput:
    return "CryptoValueAccessCodeInput";
  case CryptoNfcId:
    return "CryptoNfcId";
  case CryptoKeyPacket:
    return "CryptoKeyPacket";
  case CryptoDeviceIdResponse:
    return "CryptoDeviceIdResponse";
  case CryptoDeviceIdRequest:
    return "CryptoDeviceIdRequest";
  case CryptoChallengeRequestFromServer:
    return "CryptoChallengeRequestFromServer";
  case CryptoChallengeRequestToServer:
    return "CryptoChallengeRequestToServer";
  case Fragment_Start:
    return "Fragment_Start";
  case Fragment_Data:
    return "Fragment_Data";
  case Update_Reply:
    return "Update_Reply";
  case Identify_Unknown_Extensions:
    return "Identify_Unknown_Extensions";
  case KNX_Monitor:
    return "KNX_Monitor";
  case Search_Devices:
    return "Search_Devices";
  case Search_Reply:
    return "Search_Reply";
  case NAT_Offer:
    return "NAT_Offer";
  case NAT_Index_Request:
    return "NAT_Index_Request";
  default:
    break;
  }
  return NULL;
}

void LoxCanMessage::print(LoxCANDriver &driver) const {
  assert(sizeof(LoxCanMessage) == 12); //, "LoxCanMessage size wrong");

  printf("msg:");
  if (this->isNATmessage(driver)) { // NAT command
    switch (this->directionNat) {
    case LoxCmdNATDirection_t_fromDevice:
      printf("D");
      break;
    case LoxCmdNATDirection_t_illegal:
      printf("?");
      break;
    case LoxCmdNATDirection_t_fromServerShortcut:
      printf("s");
      break;
    case LoxCmdNATDirection_t_fromServer:
      printf("S");
      break;
    }
    printf(this->fragmented ? "F" : " ");
    printf(" NAT:%02x/%02x ", this->extensionNat, this->deviceNAT);
    const char *natStr = NATcommandString(this->commandNat);
    if (natStr)
      printf(natStr);
    else
      printf("Cmd:%02x", this->commandNat);
    printf(" : ");
    if (this->commandNat == Fragment_Start) {
      natStr = NATcommandString(LoxMsgNATCommand_t(this->data[0]));
      if (natStr)
        printf("(%s) ", natStr);
    }
  } else { // legacy command
    printf("0x%x ", this->identifier);
    printf("Dir:%d ", this->directionLegacy);
    printf("Hw:%02x ", this->hardwareType);
    printf("Serial:%06x ", this->serial);
    printf("Cmd:%d/%02x ", this->commandDirection, this->commandLegacy);
  }
  printf("%02x.%02x.%02x.%02x.%02x.%02x.%02x\n", this->data[0], this->data[1], this->data[2], this->data[3], this->data[4], this->data[5], this->data[6]);
}

#endif
