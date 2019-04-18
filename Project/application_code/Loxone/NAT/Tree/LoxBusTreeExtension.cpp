//
//  LoxBusTreeExtension.cpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeExtension.hpp"
#include "global_functions.hpp"
#include <stdio.h>
#include <string.h>

LoxBusTreeExtension::LoxBusTreeExtension(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive)
  : LoxNATExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_TreeBaseExtension << 24), eDeviceType_t_TreeBaseExtension, 0, 9030305, 0, sizeof(config), &config, alive), treeDevicesLeftCount(0), treeDevicesRightCount(0), leftDriver(this, eTreeBranch_leftBranch), rightDriver(this, eTreeBranch_rightBranch) {
}

/***
 *  Adding a device to the Tree Bus Extension
 ***/
void LoxBusTreeExtension::AddDevice(LoxBusTreeDevice *device, eTreeBranch branch) {
  if (branch == eTreeBranch_leftBranch) {
    assert(this->treeDevicesLeftCount != MAX_TREE_DEVICECOUNT);
    this->treeDevicesLeft[this->treeDevicesLeftCount++] = device;
  } else if (branch == eTreeBranch_rightBranch) {
    assert(this->treeDevicesRightCount != MAX_TREE_DEVICECOUNT);
    this->treeDevicesRight[this->treeDevicesRightCount++] = device;
  }
}

LoxBusTreeExtensionCANDriver &LoxBusTreeExtension::Driver(eTreeBranch branch) {
  if (branch == eTreeBranch_leftBranch)
    return this->leftDriver;
  return this->rightDriver;
}

/***
 *  Send values after the start
 ***/
void LoxBusTreeExtension::SendValues(void) {
  send_can_status(CAN_Error_Reply, eTreeBranch_leftBranch);
  send_can_status(CAN_Error_Reply, eTreeBranch_rightBranch);
}

/***
 *
 ***/
void LoxBusTreeExtension::from_treebus_to_loxonelink(eTreeBranch treeBranch, LoxCanMessage &message) {
  message.busType = LoxCmdNATBus_t_LoxoneLink;
  message.extensionNat = this->extensionNAT;
  if (treeBranch == eTreeBranch_leftBranch and (message.commandNat == Search_Reply or message.commandNat == NAT_Index_Request))
    message.data[0] |= 0x40;
  this->driver.SendMessage(message);
}

/***
 *  A direct message received
 ***/
void LoxBusTreeExtension::ReceiveDirect(LoxCanMessage &message) {
  if (message.deviceNAT != 0) { // forward to a tree device?
    message.busType = LoxCmdNATBus_t_TreeBus;
    uint8_t nat = message.deviceNAT;
    message.extensionNat = nat;
    // messages to parked devices is sent to both branches for parked devices, except for a NAT offer
    if ((nat & 0x80) == 0x80 and message.commandNat != NAT_Offer) {
      for (int i = 0; i < this->treeDevicesLeftCount; ++i)
        this->treeDevicesLeft[i]->ReceiveMessage(message);
      for (int i = 0; i < this->treeDevicesRightCount; ++i)
        this->treeDevicesRight[i]->ReceiveMessage(message);
    } else {
      if (message.commandNat == NAT_Offer)
        nat = message.data[0]; // for NAT offset use the new NAT
      if (nat & 0x40) {        // left branch?
        for (int i = 0; i < this->treeDevicesLeftCount; ++i)
          this->treeDevicesLeft[i]->ReceiveMessage(message);
      } else { // right branch
        for (int i = 0; i < this->treeDevicesRightCount; ++i)
          this->treeDevicesRight[i]->ReceiveMessage(message);
      }
    }
  } else {
    LoxCanMessage msg;
    switch (message.commandNat) {
    case CAN_Diagnosis_Request:
      if (message.value16 != 0) { // for this device (!=0 => for a Tree bus)
        send_can_status(CAN_Diagnosis_Reply, eTreeBranch(message.value16));
      }
      break;
    case CAN_Error_Request:
      if (message.value16 != 0) { // for this device (!=0 => for a Tree bus)
        send_can_status(CAN_Error_Reply, eTreeBranch(message.value16));
      }
      break;
    default:
      break;
    }
    LoxNATExtension::ReceiveDirect(message);
  }
}

// Forward other messages to the tree devices
void LoxBusTreeExtension::ReceiveBroadcast(LoxCanMessage &message) {
  LoxNATExtension::ReceiveBroadcast(message);

  message.busType = LoxCmdNATBus_t_TreeBus;
  uint8_t nat = message.deviceNAT;
  message.extensionNat = nat;
  for (int i = 0; i < this->treeDevicesLeftCount; ++i)
    this->treeDevicesLeft[i]->ReceiveMessage(message);
  for (int i = 0; i < this->treeDevicesRightCount; ++i)
    this->treeDevicesRight[i]->ReceiveMessage(message);
}

void LoxBusTreeExtension::ReceiveDirectFragment(LoxMsgNATCommand_t command, uint8_t extensionNAT, uint8_t deviceNAT, const uint8_t *data, uint16_t size) {
  if (deviceNAT == 0x00 || driver.isTreeBusDriver()) { // to this device?
    LoxNATExtension::ReceiveDirectFragment(command, extensionNAT, deviceNAT, data, size);
  } else {
    if (deviceNAT & 0x40) { // left tree?
      for (int i = 0; i < this->treeDevicesLeftCount; ++i)
        this->treeDevicesLeft[i]->ReceiveDirectFragment(command, deviceNAT, deviceNAT, data, size);
    } else {
      for (int i = 0; i < this->treeDevicesRightCount; ++i)
        this->treeDevicesRight[i]->ReceiveDirectFragment(command, deviceNAT, deviceNAT, data, size);
    }
  }
}

void LoxBusTreeExtension::ReceiveBroadcastFragment(LoxMsgNATCommand_t command, uint8_t extensionNAT, uint8_t deviceNAT, const uint8_t *data, uint16_t size) {
  if (deviceNAT == 0x00 || driver.isTreeBusDriver()) { // to this device?
    LoxNATExtension::ReceiveBroadcastFragment(command, extensionNAT, deviceNAT, data, size);
  } else {
    for (int i = 0; i < this->treeDevicesLeftCount; ++i)
      this->treeDevicesLeft[i]->ReceiveBroadcastFragment(command, deviceNAT, deviceNAT, data, size);
    for (int i = 0; i < this->treeDevicesRightCount; ++i)
      this->treeDevicesRight[i]->ReceiveBroadcastFragment(command, deviceNAT, deviceNAT, data, size);
  }
}

/***
 *  10ms Timer to be called 100x per second
 ***/
void LoxBusTreeExtension::Timer10ms() {
  LoxNATExtension::Timer10ms();
  for (int i = 0; i < this->treeDevicesLeftCount; ++i)
    this->treeDevicesLeft[i]->Timer10ms();
  for (int i = 0; i < this->treeDevicesRightCount; ++i)
    this->treeDevicesRight[i]->Timer10ms();
}