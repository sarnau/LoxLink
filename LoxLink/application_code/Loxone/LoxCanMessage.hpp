//
//  MMM_LoxCanMessage.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef MMM_LoxCanMessage_hpp
#define MMM_LoxCanMessage_hpp

#include <assert.h>
#include <stdint.h>

class LoxCANBaseDriver;

// All Loxone extension device types
typedef enum /*: uint16_t*/ {
  eDeviceType_t_Miniserver = 0x0000,              // broadcasts sent from the Miniserver
  eDeviceType_t_Extension = 0x0001,               // <https://shop.loxone.com/enuk/extension.html>
  eDeviceType_t_DimmerExtension = 0x0002,         // <https://shop.loxone.com/enuk/extension.html>
  eDeviceType_t_EnOceanExtension = 0x0003,        // <https://shop.loxone.com/enuk/enocean-extension.html>
  eDeviceType_t_DMXExtension = 0x0004,            // <https://shop.loxone.com/enuk/dmx-extension.html>
  eDeviceType_t_OneWireExtension = 0x0005,        // <https://shop.loxone.com/enuk/1-wire-extension.html>
  eDeviceType_t_RS232Extension = 0x0006,          // <https://shop.loxone.com/enuk/rs232-extension.html>
  eDeviceType_t_RS485Extension = 0x0007,          // <https://shop.loxone.com/enuk/rs485-extension.html>
  eDeviceType_t_IRExtension = 0x0008,             // <http://www.loxone.com/dede/service/dokumentation/extensions/ir.html> - discontinued
  eDeviceType_t_ModbusExtension = 0x0009,         // <https://shop.loxone.com/enuk/modbus-extension.html>
  eDeviceType_t_FroelingExtension = 0x000a,       // <https://shop.loxone.com/enuk/froeling-extension.html>
  eDeviceType_t_RelayExtension = 0x000b,          // <https://shop.loxone.com/enuk/relay-extension.html>
  eDeviceType_t_AirBaseExtension = 0x000c,        // <https://shop.loxone.com/enuk/air-base-extension.html>
  eDeviceType_t_DaliExtension = 0x000d,           // <https://shop.loxone.com/enuk/dali-extension.html>
  eDeviceType_t_Modbus232Extension = 0x000e,      // unreleased
  eDeviceType_t_FroelingExtensionSerial = 0x000f, // The Froehling Extension can't receive at this type, because 0x1F is used for the legacy software update. For Loxone Link the extension uses eDeviceType_t_FroelingExtension.
  eDeviceType_t_NATProtocol = 0x0010,             // LoxCmdNATBus_t_LoxoneLink
  eDeviceType_t_TreeBusProtocol = 0x0011,         // LoxCmdNATBus_t_TreeBus
  // the following extensions are all NAT based
  eDeviceType_t_InternormExtension = 0x0012,   // <https://shop.loxone.com/enuk/internorm-extension.html>
  eDeviceType_t_TreeBaseExtension = 0x0013,    // <https://shop.loxone.com/enuk/tree-extension.html>
  eDeviceType_t_DIExtension = 0x0014,          // <https://shop.loxone.com/enuk/di-extension.html>
  eDeviceType_t_KNXExtension = 0x0015,         // unreleased
  eDeviceType_t_AIExtension = 0x0016,          // <https://shop.loxone.com/enuk/ai-extension.html>
  eDeviceType_t_AOExtension = 0x0017,          // <https://shop.loxone.com/enuk/ao-extension.html>
  eDeviceType_t_LegacySoftwareUpdate = 0x001F, // only used for legacy software updates

  // These are Tree devices on the Tree bus behind a Tree Base Extension.
  eDeviceType_t_ValveActuatorTree = 0x8001,           // <https://shop.loxone.com/enuk/valve-actuator.html>
  eDeviceType_t_MotionSensorTree = 0x8002,            // <https://shop.loxone.com/enuk/motion-sensor.html>
  eDeviceType_t_TouchTree = 0x8003,                   // <https://shop.loxone.com/enuk/loxone-touch.html>
  eDeviceType_t_UniversalTree = 0x8004,               // unreleased
  eDeviceType_t_TouchPureTree = 0x8005,               // <https://shop.loxone.com/enuk/loxone-touch-pure.html>
  eDeviceType_t_LEDCeilingLightTree = 0x8006,         // <https://shop.loxone.com/enuk/led-ceiling-light-rgbw.html>
  eDeviceType_t_LEDSurfaceMountSpotRGBWTree = 0x8007, // not sure
  eDeviceType_t_LEDSpotRGBWTreeGen1 = 0x8008,         // <https://shop.loxone.com/enuk/led-spot-rgbw-global.html>
  eDeviceType_t_NFCCodeTouchTree = 0x8009,            // <https://shop.loxone.com/enuk/nfc-code-touch.html>
  eDeviceType_t_WeatherStationTree = 0x800a,          // <https://shop.loxone.com/enuk/weather-station.html>
  eDeviceType_t_NanoDITree = 0x800b,                  // <https://shop.loxone.com/enuk/nano-di-tree.html>
  eDeviceType_t_RGBW24VDimmerTree = 0x800c,           // <https://shop.loxone.com/enuk/rgbw-24v-dimmer.html>, <https://shop.loxone.com/enuk/rgbw-24v-compact-dimmer.html>
  eDeviceType_t_TouchSurfaceTree = 0x800d,            // <https://shop.loxone.com/enuk/touch-surface.html>
  eDeviceType_t_LEDSurfaceMountSpotWWTree = 0x800e,   // not sure
  eDeviceType_t_LEDSpotWWTreeGen1 = 0x800f,           // <https://shop.loxone.com/enuk/led-spots-ww-global.html>
  eDeviceType_t_RoomComfortSensorTree = 0x8010,       // <https://shop.loxone.com/enuk/room-comfort-sensor.html>
  eDeviceType_t_LEDPendulumSlimRGBWTree = 0x8011,     // <https://shop.loxone.com/enuk/led-pendulum-slim-global.html>
  eDeviceType_t_AlarmSirenTree = 0x8012,              // <https://shop.loxone.com/enuk/alarm-siren.html>
  eDeviceType_t_DamperTree = 0x8013,                  // <https://shop.loxone.com/enus/damper-tree.html>
  eDeviceType_t_LeafTree = 0x8014,                    // <https://shop.loxone.com/dede/leaf-1.html>
  eDeviceType_t_IntegratedWindowContactTree = 0x8015, // unreleased
  eDeviceType_t_LEDSpotRGBWTree = 0x8016,             // <https://shop.loxone.com/enuk/led-ceiling-spots-rgbw-global.html>
  eDeviceType_t_LEDSpotWWTree = 0x8017,               // <https://shop.loxone.com/enuk/led-ceiling-spots-ww-global.html>
} eDeviceType_t;

/////////////////////////////////////////////////////////////////
// NAT protocol
typedef enum {
  // starting from 0x00: general commands
  Version_Request = 0x01,
  Start = 0x02,
  Device_Version = 0x03,
  Config_Equal = 0x04,
  Ping = 0x05,
  Pong = 0x06,
  Park_Devices = 0x07,
  Alive_Packet = 0x08,
  Sync_Packet = 0x0C,
  Identify_LED = 0x10,
  Config_Data = 0x11,
  WebServicesText = 0x12,
  DeviceLog = 0x13,
  CAN_Diagnosis_Reply = 0x16,
  CAN_Diagnosis_Request = 0x17,
  CAN_Error_Reply = 0x18,
  CAN_Error_Request = 0x19,
  Tree_Shortcut = 0x1A,
  Tree_Shortcut_Test = 0x1B,
  KNX_Send_Telegram = 0x1C,
  KNX_Group_Address_Config = 0x1D,

  // starting from 0x80: getter/setter values commands
  Digital_Value = 0x80,
  Analog_Value = 0x81,
  RGBW = 0x84,
  Frequency = 0x85,
  AccessCodeInput = 0x86,
  Keypad_NfcId = 0x87,
  Composite_RGBW = 0x88,
  TreeKeypad_Send = 0x89,
  Composite_White = 0x8A,

  // starting from 0x90: encrypted commands
  CryptoValueDigital = 0x90,         // after decryption maps to Digital_Value
  CryptoValueAnalog = 0x91,          // after decryption maps to Analog_Value
  CryptoValueAccessCodeInput = 0x92, // after decryption maps to AccessCodeInput
  CryptoNfcId = 0x93,
  CryptoKeyPacket = 0x94,
  CryptoDeviceIdResponse = 0x98,
  CryptoDeviceIdRequest = 0x99,
  CryptoChallengeRequestFromServer = 0x9A,
  CryptoChallengeRequestToServer = 0x9B,

  // starting from 0xF0: NAT assignments, large packages, update, etc
  Fragment_Start = 0xF0,
  Fragment_Data = 0xF1,
  Update_Reply = 0xF3,
  Identify_Unknown_Extensions = 0xF4,
  KNX_Monitor = 0xF5,
  Search_Devices = 0xFB,
  Search_Reply = 0xFC,
  NAT_Offer = 0xFD,
  NAT_Index_Request = 0xFE,
} LoxMsgNATCommand_t;

// Tree devices always send with a direction of 0.
// The Miniserver always sends messages with both bits set. All devices only test for the upper bit
// to test for a server message. Only the Tree Base Extension and the Miniserver are implementing
// the other cases.
//
// Because the Tree Base Extension has two Tree busses, there is a chance of accidentally
// wiring both Tree busses together. This is an illegal shortcut. If the Tree Base Extension
// receives a Miniserver message from the Tree bus (which is normally impossible, considering
// that the Tree Base Extension is the only one sending messages to the Tree bus). The
// Tree Base Extension deals with these messages in the following way:
// - clear the lower bit in the direction.
// - if the command is `Tree_Shortcut_Test`, just erase the `deviceNAT` and mark the Tree Bus
//   in `value8`, bit 6 is set for the left tree, cleared for the right tree. The Miniserver
//   sends this command to test if a shortcut has been fixed. The deviceNAT in this case is
//   always 0x3F, plus bit 6 marking the right/left Tree.
// - any other Miniserver message is replaced with `Tree_Shortcut` and all data in the package
//   is erased and mark the Tree Bus in `value8`, bit 6 is set for the left tree, cleared for
//   the right tree.
// By doing this, it avoids duplicate packages from the server on the Loxone Bus and allows
// the Miniserver to detect the tree shortcut to report the error.
typedef enum {
  LoxCmdNATDirection_t_fromDevice = 0,         // all messages sent from an extension/device to the Miniserver
  LoxCmdNATDirection_t_illegal = 1,            // this should never occur
  LoxCmdNATDirection_t_fromServerShortcut = 2, // used to detect shortcut messages
  LoxCmdNATDirection_t_fromServer = 3,         // all messages sent out from the Miniserver
} LoxCmdNATDirection_t;

typedef enum {
  LoxCmdNATPackage_t_standard = 0,   // a regular 8 byte package
  LoxCmdNATPackage_t_fragmented = 1, // larger packages, up to 64kb large, which are build from several standard messages with the commands: Fragment_Start and Fragment_Data
} LoxCmdNATPackage_t;

typedef enum {
  // all other types are legacy messages
  LoxCmdNATBus_t_LoxoneLink = 0x10, // The CAN bus at the Miniserver, running at 125kHz
  LoxCmdNATBus_t_TreeBus = 0x11,    // The left/right CAN bus behind a Tree Extension, running at 50kHz
} LoxCmdNATBus_t;

/////////////////////////////////////////////////////////////////
// Legacy protocol
typedef enum {                   // some of these commands have different meanings, depending on the extension
  identify = 0x00,               // send from Miniserver
  software_update_init = 0x01,   // send from Miniserver
  reboot_all = 0x02,             // send from Miniserver
  software_update_verify = 0x03, // send from Miniserver
  config_acknowledge = 0x04,     // send from extension
  BC_ACK = 0x05,
  BC_NAK = 0x06,
  start_request = 0x07,
  identify_LED = 0x08, // send from Miniserver
  alive = 0x09,
  software_update_init_modules = 0x0A, // send from Miniserver
  identify_unknown_extensions = 0x0B,  // send from Miniserver
  extension_offline = 0x0C,            // send from Miniserver
  sync_ticks = 0x0D,                   // send from Miniserver
  LED_flash_position = 0x0E,           // send from Miniserver
  alive_reply = 0x0F,
  analog_input_config_0 = 0x10,
  analog_input_config_1 = 0x11,
  dmx_search_reply_high = 0x12,
  webrequest = 0x12,
  dmx_search_last_reply_low = 0x13,
  crashreport = 0x13,
  //    C485I_14 = 0x14,
  //    C485I_15 = 0x15,
  //    C485I_16 = 0x16,
  //    C485I_17 = 0x17,
  //    C485I_18 = 0x18,
  //    AirAliveRequestForFrequencyChange = 0x1A,
  air_parameter = 0x1B,
  debug = 0x1C,              // send from Miniserver
  request_statistics = 0x1D, // send from Miniserver
  air_OtauRxPacket = 0x1F,   // Air Otau packet, see ZWIR4502 documentation
  analog_input_value = 0x20,
  Enocean_config = 0x21,
  //    Enocean_22 = 0x22,
  Enocean_learn = 0x23,
  Enocean_value = 0x24,
  //    Enocean_25 = 0x25,
  //    Enocean_26 = 0x26,
  //    Enocean_27 = 0x27,
  //    DimmerExtension_InitConfirm = 0x28,
  //    Enocean_29 = 0x29,
  //    Enocean_2A = 0x2A,
  //    Enocean_2B = 0x2B,
  //    DimmerExtension_2C = 0x2C,
  sync_date_time = 0x2D, // send from Miniserver
  //    HandleQueueLevel = 0x2E,
  //    HandleChannelUtilization = 0x2F,
  analog_output_value = 0x30,
  analog_output_config = 0x31,
  //    DataGetChecksum = 0x32, // CBusDeviceHandler::SendDataFile, CBusDeviceHandler::DataGetChecksum, CBusDeviceHandler::HandleDataAnswer
  software_update_retry_page = 0x34, // send from Miniserver
  log_level = 0x35,                  // send from Miniserver
  config_check_CRC = 0x36,           // send from extensions
  park_extension = 0x37,             // send from Miniserver
  LinkDiagnosis_reply = 0x38,        // send from extension
  LinkDiagnosis_request = 0x39,      // send from Miniserver
  digital_input_config_0 = 0x40,
  digital_input_config_1 = 0x41,
  digital_input_config_2 = 0x42,
  digital_input_config_3 = 0x43,
  fragmented_package = 0x44,            // package size less then 1530 bytes
  fragmented_package_large_data = 0x45, // package size less then 64kb
  fragmented_package_large_start = 0x46,
  //    DimmerExtension_47 = 0x47, // prepare composite actor
  digital_input_value = 0x50,
  digital_input_frequency = 0x51,
  system_temperature = 0x53,       // send from extension (only used by Dimmer, Relay, Extension)
  software_update_page_crc = 0x54, // send from Miniserver
  air_reboot_devices = 0x58,       // send from Miniserver
  mute_all = 0x5B,                 // send from Miniserver
  Modbus_485_SensorValue = 0x5C,
  Modbus_485_WriteSingleCoil = 0x5D,
  Modbus_485_WriteSingleRegister = 0x5E,
  Modbus_485_WriteMultipleRegisters = 0x5F,
  Modbus_485_WriteMultipleRegisters2 = 0x60,
  digital_output_value = 0x60,
  Modbus_485_WriteSingleRegister4 = 0x61,
  //    Enocean_61 = 0x61,
  Modbus_485_WriteMultipleRegisters4 = 0x62,
  //    Enocean_62 = 0x62,
  Modbus_485_WriteMultipleCoils = 0x63,
  OneWire_polling_cycle = 0x63,
  set_monitor = 0x64,
  dmx_search = 0x64,
  dmx_search_reply_low = 0x64,
  dmx_search_last_reply_high = 0x65,
  OneWire_search = 0x65,
  OneWire_analog_value = 0x66,
  //    Air_67 = 0x67,
  RS232_config_hardware = 0x69,
  IR_learn = 0x6A,
  IR_sensor_value = 0x6B,
  //    Dali_ValueReceived = 0x6B,
  //    Dali_6C = 0x6C,
  //    C485I_6C = 0x6C,
  IR_raw_value = 0x6D,
  Dali_change_address = 0x6D,
  Dali_monitor_data = 0x6E,
  Dali_state = 0x6F,
  //    ValueFromC232C485SensorB = 0x70, // ??? The RS232 doesn't send that (anymore?), it is always send as a fragmented package
  RS232_send_bytes = 0x71,
  OneWire_NAT_serial = 0x72,
  OneWire_NAT_index_checksum = 0x73,
  DMX_learn = 0x74,
  EnOcean_ValueFromUnknownSensor1 = 0x75,
  EnOcean_ValueFromUnknownSensor2A = 0x76,
  EnOcean_ValueFromUnknownSensor2B = 0x77,
  config_checksum = 0x78,
  config_checksum_request = 0x79,
  OneWire_iButton_arrive = 0x7A,
  OneWire_iButton_depart = 0x7B,
  RS232_config_protocol = 0x7C,
} LoxMsgLegacyCommand_t;

typedef enum {
  LoxMsgLegacyDirection_t_fromDevice = 0, // all messages sent from an extension to the Miniserver
  LoxMsgLegacyDirection_t_fromServer = 1, // all messages sent out from the Miniserver
} LoxMsgLegacyDirection_t;

typedef enum {                                   // This bit is ignored by Extensions and the Miniserver, but it is always set accordingly
  LoxMsgLegacyCommandDirection_t_fromServer = 0, // commands sent out from the Miniserver
  LoxMsgLegacyCommandDirection_t_fromDevice = 1, // commands sent from an extension to the Miniserver
} LoxMsgLegacyCommandDirection_t;

/////////////////////////////////////////////////////////////////
// All messages are extended CAN messages with the identifier and 8 data bytes
// This structure is defined for little-endian CPUs only!
class LoxCanMessage {
public:
  LoxCanMessage()
    : identifier(0) {
    can_data[0] = 0;
    can_data[1] = 0;
    can_data[2] = 0;
    can_data[3] = 0;
    can_data[4] = 0;
    can_data[5] = 0;
    can_data[6] = 0;
    can_data[7] = 0;

    //        assert(sizeof(this->can_data) == 8);
    //        assert(((uint8_t*)&this->can_data - (uint8_t*)&this->value8) == 1);
    //        assert(((uint8_t*)&this->value8 - (uint8_t*)&this->deviceNAT) == 1);
    //        assert(((uint8_t*)&this->value16 - (uint8_t*)&this->deviceNAT) == 2);
    //        assert(((uint8_t*)&this->value32 - (uint8_t*)&this->deviceNAT) == 4);
    //        assert(((uint8_t*)&this->can_data - (uint8_t*)&this->data) == 1);
  };

  // 4 byte (29 bit) CAN identifier
  union __attribute__((__packed__)) {
    uint32_t identifier;                 // 29-bit CAN identifier
    struct __attribute__((__packed__)) { // NAT package format of the CAN identifier
      LoxMsgNATCommand_t commandNat : 8;
      unsigned zero : 4; // always 0
      unsigned extensionNat : 8;
      LoxCmdNATPackage_t fragmented : 1;
      LoxCmdNATDirection_t directionNat : 2;
      unsigned unused : 1; // always 0
      LoxCmdNATBus_t busType : 5;
    };
    struct __attribute__((__packed__)) {           // Legacy package format of the CAN identifier
      unsigned serial : 24;                        // serial number of the device, typically printed with the hardware type as %07x
      eDeviceType_t hardwareType : 4;              // hardware type (WARNING: only the lower 4 bits of it!)
      LoxMsgLegacyDirection_t directionLegacy : 1; // from/to server
    };
  };

  // 8 bytes data field of the CAN message
  union {
    uint8_t can_data[8];
    struct { // a NAT package has it's deviceNAT in the first byte
      uint8_t deviceNAT;
      uint8_t value8;
      uint16_t value16; // often commands have a 16-bit and a 32-bit value
      uint32_t value32;
    };
    struct { // a Legacy package has it's command in the first byte
      LoxMsgLegacyCommand_t commandLegacy : 7;
      LoxMsgLegacyCommandDirection_t commandDirection : 1;
      uint8_t data[7]; // the remaining 7 bytes are data, depended on the command
    };
  };

  bool isNATmessage(LoxCANBaseDriver &driver) const;
#if DEBUG
  void print(LoxCANBaseDriver &driver) const;

private:
  const char *const LegacyCommandString(LoxMsgLegacyCommand_t command, eDeviceType_t hardware) const;
  const char *const HardwareNameString(eDeviceType_t hardware) const;
  const char *const NATCommandString(LoxMsgNATCommand_t command) const;
#endif
};

#endif /* MMM_LoxCanMessage_hpp */