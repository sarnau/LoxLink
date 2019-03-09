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

class LoxCANDriver;

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
  webrequest = 0x12,
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
  system_temperature = 0x53,       // send from extension
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
  LoxCmdLegacyHardware_t_Server = 0x00,               // from Miniserver, used for Broadcast messages to all extensions
  LoxCmdLegacyHardware_t_Extension = 0x01,            // Extension
  LoxCmdLegacyHardware_t_Dimmer = 0x02,               // Dimmer Extension
  LoxCmdLegacyHardware_t_EnOcean = 0x03,              // EnOcean Extension
  LoxCmdLegacyHardware_t_DMX = 0x04,                  // DMX Extension
  LoxCmdLegacyHardware_t_OneWire = 0x05,              // 1-Wire Extension
  LoxCmdLegacyHardware_t_RS232 = 0x06,                // RS232 Extension
  LoxCmdLegacyHardware_t_RS485 = 0x07,                // RS485 Extension
  LoxCmdLegacyHardware_t_IR = 0x08,                   // IR Extension
  LoxCmdLegacyHardware_t_Modbus485 = 0x09,            // Modbus 485 Extension
  LoxCmdLegacyHardware_t_Froeling = 0x0a,             // FrÃ¶ling Extension (messages on the bus)
  LoxCmdLegacyHardware_t_Relay = 0x0b,                // Relay Extension
  LoxCmdLegacyHardware_t_AirBase = 0x0c,              // Air Base Extension
  LoxCmdLegacyHardware_t_Dali = 0x0d,                 // Dali Extension
  LoxCmdLegacyHardware_t_Modbus232 = 0x0e,            // Modbus 232 Extension
  LoxCmdLegacyHardware_t_FroelingSerialnumber = 0x0f, // FrÃ¶ling Extension (serial number only), 0x0f can not be used for legacy devices, because messages from the server are not possible - 0x1f is  used for Legacy software updates.
} LoxCmdLegacyHardware_t;

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
      LoxCmdLegacyHardware_t hardwareType : 4;     // hardware type
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

  bool isNATmessage(LoxCANDriver &driver) const;
#if DEBUG
  void print(LoxCANDriver &driver) const;

private:
    const char * const NATcommandString(LoxMsgNATCommand_t command) const;
#endif
};

#endif /* MMM_LoxCanMessage_hpp */