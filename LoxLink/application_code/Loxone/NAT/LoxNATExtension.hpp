//
//  LoxNATExtension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxNATExtension_hpp
#define LoxNATExtension_hpp

#include "LoxExtension.hpp"
#include "system.hpp"

// Configuration for extensions all share the same header. The configuration is stored in FLASH and
// validated via a CRC with the Miniserver to be current. If not, the Miniserver automatically uploads
// a new configuration to the extension.
// Most Extensions have optional data after the header. This data is versioned via the `version` number.
// After the optional data there is always 4 zero bytes. The minimum size of a configuration is therefore
// 12 bytes.
// The 32-bit CRC is a STM32 CRC32 over the `(size - 1)/4` uint32_t words. For that reason the configuration
// is followed by 4 zero bytes.
class __attribute__((__packed__)) tConfigHeader {
public:
  uint8_t size;            // size of the configuration in bytes
  uint8_t version;         // version number for the configuration
  uint8_t blinkSyncOffset; // allows to sync LEDs between devices, so they flash in a specific order
  uint8_t _unused;         // 32-bit alignment
  uint32_t offlineTimeout; // time in s after which the extension/device is considered offline. Reset with each message from the server.
};
typedef uint32_t tConfigHeaderFiller; // dummy filler for the end of the configuration

typedef enum /*: uint8_t*/ {
  eUpdatePackageType_write_flash = 1,      // update bytes of the update
  eUpdatePackageType_receive_crc = 2,      // upload CRCs for the update
  eUpdatePackageType_verify = 3,           // verify the update of pageNumber pages and send reply for ok/error
  eUpdatePackageType_verify_and_reset = 4, // like verify, plus a reset
  eUpdatePackageType_reply_verify_ok = 0x80,
  eUpdatePackageType_reply_verify_error = 0x81, // pageNumber contains the wrong page, crc[0] the wrong CRC.
} eUpdatePackageType;

typedef struct __attribute__((__packed__)) {
  uint8_t size;                                     // size of this package
  uint8_t /*eUpdatePackageType*/ updatePackageType; // type of this package, see above
  eDeviceType_t device_type;                        // for which hardware is this update
  uint32_t version;                                 // what is the new version number for this update
  uint16_t pageNumber;                              // a page is 512 bytes large
  uint16_t blockNumber;                             // a block is 16 bytes large
  union {
    uint8_t data[16]; // write flash has 16 bytes of data
    uint32_t crc[16]; // verify flash has up to 16 STM32 CRC32 values, pageNumber is an offset, to allow up to 64 CRC32 entries
  };
} eUpdatePackage;

typedef enum {
  // Bit 0..3 // multiplication factor for floating point numbers
  eAnalogFormat_mul_1 = 0,
  eAnalogFormat_mul_1000 = 1,
  eAnalogFormat_mul_1000000 = 2,
  eAnalogFormat_mul_1000000000 = 3,
  eAnalogFormat_div_1000 = 5,
  eAnalogFormat_div_1000000 = 6,
  eAnalogFormat_div_1000000000 = 7,
  eAnalogFormat_div_10 = 8,
} eAnalogFormat;

typedef enum {
  // Bit 4 // sign of floating point numbers, always seems to be signed
  eAnalogFlags_unsignedValue = 0x00,
  eAnalogFlags_signedValue = 0x10,
} eAnalogFlags;

typedef enum {
  eTreeBranch_extension = 0,
  eTreeBranch_leftBranch = 1,
  eTreeBranch_rightBranch = 2,
} eTreeBranch;

class LoxNATExtension : public LoxExtension {
protected:
  // Configuration support
  const uint8_t configVersion;    // version of the configuration, typically 0.
  const uint8_t configSize;       // size of the expected configuration, at least 12 bytes
  tConfigHeader *const configPtr; // pointer to the configuration

  // fragmented command support
  LoxMsgNATCommand_t fragCommand;
  uint16_t fragSize;
  uint16_t fragOffset;
  uint32_t fragCRC;
  uint8_t fragBuffer[1024];

  // some internal state variables
  LoxCmdNATBus_t busType;                 // LoxoneLink extension or a Tree device?
  uint8_t extensionNAT;                   // NAT of the extension
  uint8_t deviceNAT;                      // NAT for the device on a Tree bus, otherwise 0
  uint8_t /*eAliveReason_t*/ aliveReason; // reason for a reset or current state
  uint32_t upTimeInMs;                    // time since boot in ms
  int32_t NATStateCounter;                //
  int32_t randomNATIndexRequestDelay;
  int32_t offlineTimeout;
  int32_t offlineCountdownInMs;

  // internal functions
  void send_message(LoxMsgNATCommand_t command, LoxCanMessage &msg);
  void send_special_message(LoxMsgNATCommand_t command);
  void lox_send_package_if_nat(LoxMsgNATCommand_t command, LoxCanMessage &msg);
  void send_fragmented_message(LoxMsgNATCommand_t command, const void *data, int dataCount, uint8_t deviceNAT);
  void send_alive_package(void);
  void send_can_status(LoxMsgNATCommand_t command, eTreeBranch branch);
  void send_info_package(LoxMsgNATCommand_t command, uint8_t /*eAliveReason_t*/ reason);
  void send_digital_value(uint8_t index, uint32_t value);
  void send_analog_value(uint8_t index, uint32_t value, uint16_t flags, eAnalogFormat format);
  void send_frequency_value(uint8_t index, uint32_t value);
  void update(const eUpdatePackage *updatePackage);
  void config_data(const tConfigHeader *config);
  uint32_t config_CRC(void);

  virtual void ConfigUpdate(void){};
  virtual void ConfigLoadDefaults(void){};
  virtual void SendValues(void){};
  virtual void SetState(eDeviceState state);
 public:
  virtual void ReceiveDirect(LoxCanMessage &message);
  virtual void ReceiveBroadcast(LoxCanMessage &message);
  virtual void ReceiveDirectFragment(LoxMsgNATCommand_t command, const uint8_t *data, uint16_t size);
  virtual void ReceiveBroadcastFragment(LoxMsgNATCommand_t command, const uint8_t *data, uint16_t size);

public:
  LoxNATExtension(LoxCANBaseDriver &driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version, uint8_t configVersion, uint8_t configSize, tConfigHeader *configPtr, eAliveReason_t alive);

  virtual void Timer10ms(void);
  virtual void ReceiveMessage(LoxCanMessage &message);
};

#endif /* LoxNATExtension_hpp */