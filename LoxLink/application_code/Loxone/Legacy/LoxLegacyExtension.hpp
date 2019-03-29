//
//  LoxLegacyExtension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxLegacyExtension_hpp
#define LoxLegacyExtension_hpp

#include "LoxExtension.hpp"

#define EXTENSION_RS232 0
#define EXTENSION_MODBUS 1

/////////////////////////////////////////////////////////////////
// Legacy protocol
typedef enum { // some of these commands have different meanings, depending on the extension
  FragCmd_air_container = 0x00,
  FragCmd_air_MAC_container = 0x01,
  FragCmd_C485_package_received = 0x02,
  FragCmd_page_CRC_external = 0x04,
  FragCmd_retry_page_external = 0x05,
  FragCmd_Modbus_config = 0x06,
  FragCmd_dali_search_data = 0x07,
  FragCmd_dali_search_data_lastpackage = 0x08,
  FragCmd_WebServiceRequest = 0x09,
  FragCmd_C232_bytes_received = 0x0a, // RS232 bytes received
  FragCmd_device_webservice = 0x0b,
  FragCmd_air_nat_update = 0x0c,
  FragCmd_DMX_actor = 0x0d,
  FragCmd_DMX_dimming = 0x0e,
  FragCmd_DMX_init_rdm_device = 0x0f,
  FragCmd_config_data = 0x11,
  FragCmd_air_debug_container = 0x12,
  FragCmd_DMX_composite_actor = 0x13,
  FragCmd_dali_log = 0x28,
} LoxMsgLegacyFragmentedCommand_t;

// Definition of the fragmented header
typedef struct {
    uint8_t command;
    uint8_t packageIndex;
    uint8_t fragCommand; // LoxMsgLegacyFragmentedCommand_t
    uint8_t unused;
    uint16_t size; // number of bytes in the fragment
    uint16_t checksum; // byte checksum over the fragment
} LoxFragHeader;

class LoxLegacyExtension : public LoxExtension {
protected:
  bool isMuted;
  bool forceStartMessage;
  int32_t aliveCountdown;

  // firmware update
  bool firmwareUpdateActive;
  uint32_t firmwareNewVersion;
  uint32_t firmwareUpdateCRCs[64];
  LoxFragHeader fragHeader;
  int fragLargeIndex;
  void *fragPtr;
  uint16_t fragMaxSize;

  void sendCommandWithValues(LoxMsgLegacyCommand_t command, uint8_t val8, uint16_t val16, uint32_t val32);
  void sendCommandWithVersion(LoxMsgLegacyCommand_t command);
  void send_fragmented_data(LoxMsgLegacyFragmentedCommand_t command, const void *buffer, uint32_t byteCount);

  virtual void PacketMulticastAll(LoxCanMessage &message);
  virtual void PacketMulticastExtension(LoxCanMessage &message);
  virtual void PacketToExtension(LoxCanMessage &message);
  virtual void PacketFromExtension(LoxCanMessage &message);
  virtual void PacketFirmwareUpdate(LoxCanMessage &message);
  virtual void FragmentedPacketToExtension(LoxMsgLegacyFragmentedCommand_t fragCommand, const void *fragData, int size) {};

public:
  LoxLegacyExtension(LoxCANBaseDriver &driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version, void *fragPtr = 0, uint16_t fragMaxSize = 0);

  virtual void Timer10ms(void);
  virtual void ReceiveMessage(LoxCanMessage &message);
};

#endif /* LoxLegacyExtension_hpp */