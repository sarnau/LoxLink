//
//  LoxExtension.hpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright © 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxExtension_hpp
#define LoxExtension_hpp

#include "LoxCanMessage.hpp"
#include "LoxCANDriver.hpp"

// The different state, in which the extension can be
typedef enum {
    eDeviceState_offline = 0, // no Miniserver communication
    eDeviceState_parked, // detected by the Miniserver, but Extension is not part of the current configuration
    eDeviceState_online, // fully active Extension
} eDeviceState;

// All Loxone extension device types
typedef enum /*: uint16_t*/ {
    eDeviceType_t_Miniserver = 0x0000, // broadcasts sent from the Miniserver
    eDeviceType_t_Extension = 0x0001, // <https://shop.loxone.com/enuk/extension.html>
    eDeviceType_t_DimmerExtension = 0x0002, // <https://shop.loxone.com/enuk/extension.html>
    eDeviceType_t_EnOceanExtension = 0x0003, // <https://shop.loxone.com/enuk/enocean-extension.html>
    eDeviceType_t_DMXExtension = 0x0004, // <https://shop.loxone.com/enuk/dmx-extension.html>
    eDeviceType_t_OneWireExtension = 0x0005, // <https://shop.loxone.com/enuk/1-wire-extension.html>
    eDeviceType_t_RS232Extension = 0x0006, // <https://shop.loxone.com/enuk/rs232-extension.html>
    eDeviceType_t_RS485Extension = 0x0007, // <https://shop.loxone.com/enuk/rs485-extension.html>
    eDeviceType_t_IRExtension = 0x0008, // <http://www.loxone.com/dede/service/dokumentation/extensions/ir.html> - discontinued
    eDeviceType_t_ModbusExtension = 0x0009, // <https://shop.loxone.com/enuk/modbus-extension.html>
    eDeviceType_t_FroelingExtension = 0x000a, // <https://shop.loxone.com/enuk/froeling-extension.html>
    eDeviceType_t_RelayExtension = 0x000b, // <https://shop.loxone.com/enuk/relay-extension.html>
    eDeviceType_t_AirBaseExtension = 0x000c, // <https://shop.loxone.com/enuk/air-base-extension.html>
    eDeviceType_t_DaliExtension = 0x000d, // <https://shop.loxone.com/enuk/dali-extension.html>
    eDeviceType_t_Modbus232Extension = 0x000e, // unreleased
    eDeviceType_t_FroelingExtensionSerial = 0x000f, // The Fröhling Extension can't receive at this type, because 0x1F is used for the legacy software update. For Loxone Link the extension uses eDeviceType_t_FroelingExtension.
    eDeviceType_t_NATProtocol = 0x0010, // LoxCmdNATBus_t_LoxoneLink
    eDeviceType_t_TreeBusProtocol = 0x0011, // LoxCmdNATBus_t_TreeBus
    // the following extensions are all NAT based 
    eDeviceType_t_InternormExtension = 0x0012, // <https://shop.loxone.com/enuk/internorm-extension.html>
    eDeviceType_t_TreeBaseExtension = 0x0013, // <https://shop.loxone.com/enuk/tree-extension.html>
    eDeviceType_t_DIExtension = 0x0014, // <https://shop.loxone.com/enuk/di-extension.html>
    eDeviceType_t_KNXExtension = 0x0015, // unreleased
    eDeviceType_t_AIExtension = 0x0016, // <https://shop.loxone.com/enuk/ai-extension.html>
    eDeviceType_t_AOExtension = 0x0017, // <https://shop.loxone.com/enuk/ao-extension.html>
    eDeviceType_t_LegacySoftwareUpdate = 0x001F, // only used for legacy software updates

    // These are Tree devices on the Tree bus behind a Tree Base Extension.
    eDeviceType_t_ValveActuatorTree = 0x8001, // <https://shop.loxone.com/enuk/valve-actuator.html>
    eDeviceType_t_MotionSensorTree = 0x8002, // <https://shop.loxone.com/enuk/motion-sensor.html>
    eDeviceType_t_TouchTree = 0x8003, // <https://shop.loxone.com/enuk/loxone-touch.html>
    eDeviceType_t_UniversalTree = 0x8004, // unreleased
    eDeviceType_t_TouchPureTree = 0x8005, // <https://shop.loxone.com/enuk/loxone-touch-pure.html>
    eDeviceType_t_LEDCeilingLightTree = 0x8006, // <https://shop.loxone.com/enuk/led-ceiling-light-rgbw.html>
    eDeviceType_t_LEDSurfaceMountSpotRGBWTree = 0x8007, // not sure
    eDeviceType_t_LEDSpotRGBWTreeGen1 = 0x8008, // <https://shop.loxone.com/enuk/led-spot-rgbw-global.html>
    eDeviceType_t_NFCCodeTouchTree = 0x8009, // <https://shop.loxone.com/enuk/nfc-code-touch.html>
    eDeviceType_t_WeatherStationTree = 0x800a, // <https://shop.loxone.com/enuk/weather-station.html>
    eDeviceType_t_NanoDITree = 0x800b, // <https://shop.loxone.com/enuk/nano-di-tree.html>
    eDeviceType_t_RGBW24VDimmerTree = 0x800c, // <https://shop.loxone.com/enuk/rgbw-24v-dimmer.html>, <https://shop.loxone.com/enuk/rgbw-24v-compact-dimmer.html>
    eDeviceType_t_TouchSurfaceTree = 0x800d, // <https://shop.loxone.com/enuk/touch-surface.html>
    eDeviceType_t_LEDSurfaceMountSpotWWTree = 0x800e, // not sure
    eDeviceType_t_LEDSpotWWTreeGen1 = 0x800f, // <https://shop.loxone.com/enuk/led-spots-ww-global.html>
    eDeviceType_t_RoomComfortSensorTree = 0x8010, // <https://shop.loxone.com/enuk/room-comfort-sensor.html>
    eDeviceType_t_LEDPendulumSlimRGBWTree = 0x8011, // <https://shop.loxone.com/enuk/led-pendulum-slim-global.html>
    eDeviceType_t_AlarmSirenTree = 0x8012, // <https://shop.loxone.com/enuk/alarm-siren.html>
    eDeviceType_t_DamperTree = 0x8013, // <https://shop.loxone.com/enus/damper-tree.html>
    eDeviceType_t_LeafTree = 0x8014, // <https://shop.loxone.com/dede/leaf-1.html>
    eDeviceType_t_IntegratedWindowContactTree = 0x8015, // unreleased
    eDeviceType_t_LEDSpotRGBWTree = 0x8016, // <https://shop.loxone.com/enuk/led-ceiling-spots-rgbw-global.html>
    eDeviceType_t_LEDSpotWWTree = 0x8017, // <https://shop.loxone.com/enuk/led-ceiling-spots-ww-global.html>
} eDeviceType_t;

/***
 *  Virtual baseclass for legacy and NAT extensions and devices
 ***/
class LoxExtension {
public:
    const uint32_t serial; // 24 bit serial number of the device.
    const uint16_t/*eDeviceType_t*/ device_type; // what kind of extension is this device
    const uint8_t hardware_version; // some extensions have more than one hardware revision, but typically this is 0.
    const uint32_t version; // version number of the software in the extension. Automatically updated by the Miniserver.
protected:
    LoxCANDriver& driver;
    eDeviceState state;
    struct { // CAN bus statistics
        uint32_t Rcv; // number of received CAN bus packages
        uint32_t Sent; // number of sent CAN messages
        uint32_t RQ; // number of entries in the receive queue
        uint32_t mRQ; // maximum number of entries in the receive queue
        uint32_t TQ; // number of entries in the transmit queue
        uint32_t mTQ; // maximum number of entries in the transmit queue
        uint32_t QOvf; // number of dropped packages, because the transmit queue was full
        uint32_t Err; // incremented, whenever the CAN Last error code was != 0
        uint32_t HWE; // Hardware error: incremented, whenever the Error Passive limit has been reached (Receive Error Counter or Transmit Error Counter>127).
    } statistics;

    virtual void SetState(eDeviceState state);
    virtual void ReceiveDirect(LoxCanMessage& message){};
    virtual void ReceiveBroadcast(LoxCanMessage& message){};
    virtual void ReceiveDirectFragment(LoxMsgNATCommand_t command, const uint8_t* data, uint16_t size){};
    virtual void ReceiveBroadcastFragment(LoxMsgNATCommand_t command, const uint8_t* data, uint16_t size){};

public:
    LoxExtension(LoxCANDriver& driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version);

    // CAN bus statistics, just for debugging
    void StatisticsPrint() const;
    void StatisticsReset();

    // Need to be called by the main
    virtual void Timer10ms(void){};
    virtual void ReceiveMessage(LoxCanMessage& message){};
};

#endif /* LoxExtension_hpp */
