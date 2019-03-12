//
//  MMM_LoxCanMessage.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxCanMessage.hpp"
#include "LoxCANBaseDriver.hpp"
#include <assert.h>
#include <stdio.h>

bool LoxCanMessage::isNATmessage(LoxCANBaseDriver &driver) const {
  return (tLoxCANDriverType_LoxoneLink == driver.GetDriverType() && this->busType == LoxCmdNATBus_t_LoxoneLink) || (tLoxCANDriverType_TreeBus == driver.GetDriverType() && this->busType == LoxCmdNATBus_t_TreeBus);
}

#if DEBUG
const char *const LoxCanMessage::LegacyCommandString(LoxMsgLegacyCommand_t command, eDeviceType_t hardware) const {
  switch (command) {
  case identify:
    return "identify";
  case software_update_init:
    return "software_update_init";
  case reboot_all:
    return "reboot_all";
  case software_update_verify:
    return "software_update_verify";
  case config_acknowledge:
    return "config_acknowledge";
  case BC_ACK:
    return "BC_ACK";
  case BC_NAK:
    return "BC_NAK";
  case start_request:
    return "start_request";
  case identify_LED:
    return "identify_LED";
  case alive:
    return "alive";
  case software_update_init_modules:
    return "software_update_init_modules";
  case identify_unknown_extensions:
    return "identify_unknown_extensions";
  case extension_offline:
    return "extension_offline";
  case sync_ticks:
    return "sync_ticks";
  case LED_flash_position:
    return "LED_flash_position";
  case alive_reply:
    return "alive_reply";
  case analog_input_config_0:
    return "analog_input_config_0";
  case analog_input_config_1:
    return "analog_input_config_1";
  case webrequest:
    return "webrequest";
  case crashreport:
    return "crashreport";
  case air_parameter:
    return "air_parameter";
  case debug:
    return "debug";
  case request_statistics:
    return "request_statistics";
  case air_OtauRxPacket:
    return "air_OtauRxPacket";
  case analog_input_value:
    return "analog_input_value";
  case Enocean_config:
    return "Enocean_config";
  case Enocean_learn:
    return "Enocean_learn";
  case Enocean_value:
    return "Enocean_value";
  case sync_date_time:
    return "sync_date_time";
  case analog_output_value:
    return "analog_output_value";
  case analog_output_config:
    return "analog_output_config";
  case software_update_retry_page:
    return "software_update_retry_page";
  case log_level:
    return "log_level";
  case config_check_CRC:
    return "config_check_CRC";
  case park_extension:
    return "park_extension";
  case LinkDiagnosis_reply:
    return "LinkDiagnosis_reply";
  case LinkDiagnosis_request:
    return "LinkDiagnosis_request";
  case digital_input_config_0:
    return "digital_input_config_0";
  case digital_input_config_1:
    return "digital_input_config_1";
  case digital_input_config_2:
    return "digital_input_config_2";
  case digital_input_config_3:
    return "digital_input_config_3";
  case fragmented_package:
    return "fragmented_package";
  case fragmented_package_large_data:
    return "fragmented_package_large_data";
  case fragmented_package_large_start:
    return "fragmented_package_large_start";
  case digital_input_value:
    return "digital_input_value";
  case digital_input_frequency:
    return "digital_input_frequency";
  case system_temperature:
    return "system_temperature";
  case software_update_page_crc:
    return "software_update_page_crc";
  case air_reboot_devices:
    return "air_reboot_devices";
  case mute_all:
    return "mute_all";
  case Modbus_485_SensorValue:
    return "Modbus_485_SensorValue";
  case Modbus_485_WriteSingleCoil:
    return "Modbus_485_WriteSingleCoil";
  case Modbus_485_WriteSingleRegister:
    return "Modbus_485_WriteSingleRegister";
  case Modbus_485_WriteMultipleRegisters:
    return "Modbus_485_WriteMultipleRegisters";
  case digital_output_value:
    if (hardwareType == eDeviceType_t_ModbusExtension)
      return "Modbus_485_WriteMultipleRegisters2";
    return "digital_output_value";
  case Modbus_485_WriteSingleRegister4:
    return "Modbus_485_WriteSingleRegister4";
  case Modbus_485_WriteMultipleRegisters4:
    return "Modbus_485_WriteMultipleRegisters4";
  case OneWire_polling_cycle:
    if (hardwareType == eDeviceType_t_ModbusExtension)
      return "Modbus_485_WriteMultipleCoils";
    return "OneWire_polling_cycle";
  case set_monitor:
    return "set_monitor";
  case OneWire_search:
    return "OneWire_search";
  case OneWire_analog_value:
    return "OneWire_analog_value";
  case RS232_config_hardware:
    return "RS232_config_hardware";
  case IR_learn:
    return "IR_learn";
  case IR_sensor_value:
    return "IR_sensor_value";
  case IR_raw_value:
    if (hardwareType == eDeviceType_t_DaliExtension)
      return "Dali_change_address";
    return "IR_raw_value";
  case Dali_monitor_data:
    return "Dali_monitor_data";
  case Dali_state:
    return "Dali_state";
  case RS232_send_bytes:
    return "RS232_send_bytes";
  case OneWire_NAT_serial:
    return "OneWire_NAT_serial";
  case OneWire_NAT_index_checksum:
    return "OneWire_NAT_index_checksum";
  case DMX_learn:
    return "DMX_learn";
  case EnOcean_ValueFromUnknownSensor1:
    return "EnOcean_ValueFromUnknownSensor1";
  case EnOcean_ValueFromUnknownSensor2A:
    return "EnOcean_ValueFromUnknownSensor2A";
  case EnOcean_ValueFromUnknownSensor2B:
    return "EnOcean_ValueFromUnknownSensor2B";
  case config_checksum:
    return "config_checksum";
  case config_checksum_request:
    return "config_checksum_request";
  case OneWire_iButton_arrive:
    return "OneWire_iButton_arrive";
  case OneWire_iButton_depart:
    return "OneWire_iButton_depart";
  case RS232_config_protocol:
    return "RS232_config_protocol";
  }
  return NULL;
}

const char *const LoxCanMessage::HardwareNameString(eDeviceType_t hardware) const {
  switch (hardware) {
  case eDeviceType_t_Miniserver:
    return "Miniserver";
  case eDeviceType_t_Extension:
    return "Extension";
  case eDeviceType_t_DimmerExtension:
    return "Dimmer Extension";
  case eDeviceType_t_EnOceanExtension:
    return "EnOcean Extension";
  case eDeviceType_t_DMXExtension:
    return "DMX Extension";
  case eDeviceType_t_OneWireExtension:
    return "1-Wire Extension";
  case eDeviceType_t_RS232Extension:
    return "RS232 Extension";
  case eDeviceType_t_RS485Extension:
    return "RS485 Extension";
  case eDeviceType_t_IRExtension:
    return "IR Extension";
  case eDeviceType_t_ModbusExtension:
    return "Modbus 485 Extension";
  case eDeviceType_t_FroelingExtension:
    return "Froeling Extension";
  case eDeviceType_t_RelayExtension:
    return "Relay Extension";
  case eDeviceType_t_AirBaseExtension:
    return "Air Base Extension";
  case eDeviceType_t_DaliExtension:
    return "Dali Extension";
  case eDeviceType_t_Modbus232Extension:
    return "Modbus 232 Extension";
  case eDeviceType_t_FroelingExtensionSerial:
    return "Froeling Extension";
  case eDeviceType_t_ValveActuatorTree: // <https://shop.loxone.com/enuk/valve-actuator.html>
    return "Valve Actuator Tree";
  case eDeviceType_t_MotionSensorTree: // <https://shop.loxone.com/enuk/motion-sensor.html>
    return "Motion Sensor Tree";
  case eDeviceType_t_TouchTree: // <https://shop.loxone.com/enuk/loxone-touch.html>
    return "Touch Tree";
  case eDeviceType_t_UniversalTree: // unreleased
    return "Universal Tree";
  case eDeviceType_t_TouchPureTree: // <https://shop.loxone.com/enuk/loxone-touch-pure.html>
    return "Touch Pure Tree";
  case eDeviceType_t_LEDCeilingLightTree: // <https://shop.loxone.com/enuk/led-ceiling-light-rgbw.html>
    return "LED Ceiling Light RGBW Tree";
  case eDeviceType_t_LEDSurfaceMountSpotRGBWTree: // not sure
    return "LED Surface Mount Spot RGBW Tree";
  case eDeviceType_t_LEDSpotRGBWTreeGen1: // <https://shop.loxone.com/enuk/led-spot-rgbw-global.html>
    return "LED Sport RGBW Gen 1 Tree";
  case eDeviceType_t_NFCCodeTouchTree: // <https://shop.loxone.com/enuk/nfc-code-touch.html>
    return "NFC Code Touch Tree";
  case eDeviceType_t_WeatherStationTree: // <https://shop.loxone.com/enuk/weather-station.html>
    return "Weather Station Tree";
  case eDeviceType_t_NanoDITree: // <https://shop.loxone.com/enuk/nano-di-tree.html>
    return "Nano DI Tree";
  case eDeviceType_t_RGBW24VDimmerTree: // <https://shop.loxone.com/enuk/rgbw-24v-dimmer.html>, <https://shop.loxone.com/enuk/rgbw-24v-compact-dimmer.html>
    return "RGBW 24V Dimmer Tree";
  case eDeviceType_t_TouchSurfaceTree: // <https://shop.loxone.com/enuk/touch-surface.html>
    return "Touch Surface Tree";
  case eDeviceType_t_LEDSurfaceMountSpotWWTree: // not sure
    return "LED Surface Mount Spot WW Tree";
  case eDeviceType_t_LEDSpotWWTreeGen1: // <https://shop.loxone.com/enuk/led-spots-ww-global.html>
    return "LED Spot WW Gen 1 Tree";
  case eDeviceType_t_RoomComfortSensorTree: // <https://shop.loxone.com/enuk/room-comfort-sensor.html>
    return "Room Comfort Sensor Tree";
  case eDeviceType_t_LEDPendulumSlimRGBWTree: // <https://shop.loxone.com/enuk/led-pendulum-slim-global.html>
    return "LED Pendulum Slim RGBW Tree";
  case eDeviceType_t_AlarmSirenTree: // <https://shop.loxone.com/enuk/alarm-siren.html>
    return "Alarm Siren Tree";
  case eDeviceType_t_DamperTree: // <https://shop.loxone.com/enus/damper-tree.html>
    return "Damper Tree Tree";
  case eDeviceType_t_LeafTree: // <https://shop.loxone.com/dede/leaf-1.html>
    return "Leaf Tree";
  case eDeviceType_t_IntegratedWindowContactTree: // unreleased
    return "Integrated Window Contact Tree";
  case eDeviceType_t_LEDSpotRGBWTree: // <https://shop.loxone.com/enuk/led-ceiling-spots-rgbw-global.html>
    return "LED Spot RGBW Tree";
  case eDeviceType_t_LEDSpotWWTree: // <https://shop.loxone.com/enuk/led-ceiling-spots-ww-global.html>
    return "LED Spot WW Tree";
  default:
    return NULL;
  }
}

const char *const LoxCanMessage::NATCommandString(LoxMsgNATCommand_t command) const {
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

void LoxCanMessage::print(LoxCANBaseDriver &driver) const {
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
    const char *natStr = NATCommandString(this->commandNat);
    if (natStr)
      printf(natStr);
    else
      printf("Cmd:%02x", this->commandNat);
    printf(" : ");
    if (this->commandNat == Fragment_Start) {
      natStr = NATCommandString(LoxMsgNATCommand_t(this->data[0]));
      if (natStr)
        printf("(%s) ", natStr);
    }
  } else { // legacy command
    if (this->serial == 0 && this->hardwareType == eDeviceType_t_Miniserver) {
      printf("Miniserver Broadcast ");
    } else {
      printf((this->directionLegacy == LoxMsgLegacyDirection_t_fromServer) ? "S" : "D");
      printf((this->commandDirection == LoxMsgLegacyCommandDirection_t_fromServer) ? "s" : "d");
      printf(" %s ", HardwareNameString(this->hardwareType));
      printf("%07x ", this->identifier & 0xFFFFFFF);
    }
    //printf("Cmd:%d/", this->commandDirection);
    const char *legStr = LegacyCommandString(this->commandLegacy, this->hardwareType);
    if (legStr) {
      printf(legStr);
    } else {
      printf(legStr);
      printf("Cmd:%02x", this->commandLegacy);
    }
    printf(" : ");
  }
  printf("%02x.%02x.%02x.%02x.%02x.%02x.%02x\n", this->data[0], this->data[1], this->data[2], this->data[3], this->data[4], this->data[5], this->data[6]);
}

#endif