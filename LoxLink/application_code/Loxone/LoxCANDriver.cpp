//
//  LoxCANDriver.cpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxCANDriver.hpp"
#include "LoxExtension.hpp"
#include "main.hpp"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define CAN_GPIO_PORT GPIOB
#define CAN_RX_GPIO_PIN GPIO_PIN_8
#define CAN_TX_GPIO_PIN GPIO_PIN_9
#define CAN_BITRATE 125000

StaticQueue_t gCanReceiveQueue;
static StaticQueue_t gCanTransmitQueue;
static CAN_HandleTypeDef gCan;


LoxCANDriver::LoxCANDriver(tLoxCANDriverType type)
  : driverType(type), extensionCount(0) {
}

static void vCANTXTask(void *pvParameters) {
  const TickType_t xDelay4ms = pdMS_TO_TICKS(4);
  while (1) {
    LoxCanMessage msg;
    while (xQueueReceive(&gCanTransmitQueue, &msg, 0)) {
      const CAN_TxHeaderTypeDef hdr = {
        .ExtId = msg.identifier,
        .IDE = CAN_ID_EXT,
        .RTR = CAN_RTR_DATA,
        .DLC = 8,
        .TransmitGlobalTime = DISABLE,
      };
      uint32_t txMailbox = 0;
      /*HAL_StatusTypeDef status =*/HAL_CAN_AddTxMessage(&gCan, &hdr, msg.can_data, &txMailbox);
      vTaskDelay(xDelay4ms);
    }
  }
}

void LoxCANDriver::Startup(void) {
  // queue for 64 messages
  static LoxCanMessage sCanReceiveBuffer[64];
  xQueueCreateStatic(sizeof(sCanReceiveBuffer) / sizeof(sCanReceiveBuffer[0]), sizeof(sCanReceiveBuffer[0]), (uint8_t *)sCanReceiveBuffer, &gCanReceiveQueue);

  // queue for 64 messages
  static LoxCanMessage sCanTransmitBuffer[64];
  xQueueCreateStatic(sizeof(sCanTransmitBuffer) / sizeof(sCanTransmitBuffer[0]), sizeof(sCanTransmitBuffer[0]), (uint8_t *)sCanTransmitBuffer, &gCanTransmitQueue);

  static StackType_t sCANTXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sCANTXTask;
  xTaskCreateStatic(vCANTXTask, "CANTXTask", configMINIMAL_STACK_SIZE, NULL, 2, sCANTXTaskStack, &sCANTXTask);

  gCan.Instance = CAN1;
  gCan.Init.TimeTriggeredMode = DISABLE;
  gCan.Init.AutoBusOff = ENABLE;
  gCan.Init.AutoWakeUp = DISABLE;
  gCan.Init.AutoRetransmission = DISABLE;
  gCan.Init.ReceiveFifoLocked = DISABLE;
  gCan.Init.TransmitFifoPriority = ENABLE;
  gCan.Init.Mode = CAN_MODE_NORMAL;

  // CAN_SJW_1tq + CAN_BS1_10tq + CAN_BS2_5tq => 16tq => sample point = 68.75% =(1+10)/16
  gCan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  gCan.Init.TimeSeg1 = CAN_BS1_10TQ;
  gCan.Init.TimeSeg2 = CAN_BS2_5TQ;
  gCan.Init.Prescaler = HAL_RCC_GetPCLK1Freq() / 16 / CAN_BITRATE; // 16tq (see above)
  if (HAL_CAN_Init(&gCan) != HAL_OK) {
    for (;;)
      ;
  }

  HAL_CAN_ActivateNotification(&gCan, CAN_IT_RX_FIFO0_MSG_PENDING); // FIFO 0 message pending interrupt
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_RX_FIFO1_MSG_PENDING); // FIFO 1 message pending interrupt
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_TX_MAILBOX_EMPTY);     // Transmit mailbox empty interrupt
  // notify on certain errors:
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_ERROR_PASSIVE);   // Error passive interrupt
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_LAST_ERROR_CODE); // Last error code interrupt
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_ERROR);           // Error Interrupt
  HAL_CAN_Start(&gCan);

  // FYI: At least one filter is required to be able to receive any data.
//  FilterAllowAll(0);
}

tLoxCANDriverType LoxCANDriver::GetDriverType() const {
  return this->driverType;
}

void LoxCANDriver::AddExtension(LoxExtension *extension) {
  if (this->extensionCount == sizeof(this->extensions) / sizeof(this->extensions[0]))
    return;
  this->extensions[this->extensionCount++] = extension;
}

/***
 *  Setup CAN filters. While an AllowAll filter is working (we can filter in software), it is probably
 *  desirable to filter in "hardware" everything we do not care about.
 ***/
void LoxCANDriver::FilterSetup(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment) {
  filterId = (filterId << 3) | CAN_ID_EXT;
  filterMaskId = (filterMaskId << 3) | CAN_ID_EXT;
  CAN_FilterTypeDef filterInit = {
    .FilterIdHigh = filterId >> 16,
    .FilterIdLow = filterId & 0xFFFF,
    .FilterMaskIdHigh = filterMaskId >> 16,
    .FilterMaskIdLow = filterMaskId & 0xFFFF,
    .FilterFIFOAssignment = filterFIFOAssignment,
    .FilterBank = filterBank,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_32BIT,
    .FilterActivation = CAN_FILTER_ENABLE,
    .SlaveStartFilterBank = 0,
  };
  HAL_CAN_ConfigFilter(&gCan, &filterInit);
}

/***
 *  This Filter is a default filter, which allows to listen to all extended messages on the CAN bus
 ***/
void LoxCANDriver::FilterAllowAll(uint32_t filterBank) {
  CAN_FilterTypeDef filterInit = {
    .FilterIdHigh = 0x0000,
    .FilterIdLow = 0x0000,
    .FilterMaskIdHigh = 0x0000,
    .FilterMaskIdLow = 0x0000,
    .FilterFIFOAssignment = CAN_FILTER_FIFO0,
    .FilterBank = filterBank,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_32BIT,
    .FilterActivation = CAN_FILTER_ENABLE,
    .SlaveStartFilterBank = 0,
  };
  HAL_CAN_ConfigFilter(&gCan, &filterInit);
}

/***
 *  Setup a NAT filter
 ***/
void LoxCANDriver::FilterSetupNAT(int filterIndex, LoxCmdNATBus_t busType, uint8_t extensionNAT) {
  LoxCanMessage msg;
  msg.busType = busType;
  msg.directionNat = LoxCmdNATDirection_t_fromServer;
  msg.extensionNat = extensionNAT;
  FilterSetup(filterIndex, msg.identifier, 0x1F2FF000, CAN_FILTER_FIFO0);
  printf("Filter #%d mask:%08x value:%08x\n", filterIndex, 0x1F2FF000, msg.identifier);
}

uint8_t LoxCANDriver::GetTransmitErrorCounter() const {
  return 0;
}

uint8_t LoxCANDriver::GetReceiveErrorCounter() const {
  return 0;
}

void LoxCANDriver::Delay(int msDelay) const {
  vTaskDelay(pdMS_TO_TICKS(msDelay));
}

void LoxCANDriver::SendMessage(LoxCanMessage &message) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  BaseType_t xResult = xQueueSendToBackFromISR(&gCanTransmitQueue, &message, &xHigherPriorityTaskWoken);
  if (xResult != pdFAIL) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void LoxCANDriver::ReceiveMessage(LoxCanMessage &message) {
  for (int i = 0; i < this->extensionCount; ++i)
    this->extensions[i]->ReceiveMessage(message);
}

/**
  * @brief  Error CAN callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
extern "C" void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {
    int ErrorStatus = hcan->ErrorCode;
    if (ErrorStatus != HAL_CAN_ERROR_NONE) {
      if (ErrorStatus & HAL_CAN_ERROR_EWG)
        printf("HAL_CAN_ERROR_EWG,");
      if (ErrorStatus & HAL_CAN_ERROR_EPV)
        printf("HAL_CAN_ERROR_EPV,");
      if (ErrorStatus & HAL_CAN_ERROR_BOF)
        printf("HAL_CAN_ERROR_BOF,");
      if (ErrorStatus & HAL_CAN_ERROR_STF)
        printf("HAL_CAN_ERROR_STF,");
      if (ErrorStatus & HAL_CAN_ERROR_FOR)
        printf("HAL_CAN_ERROR_FOR,");
      if (ErrorStatus & HAL_CAN_ERROR_ACK)
        printf("HAL_CAN_ERROR_ACK,");
      if (ErrorStatus & HAL_CAN_ERROR_BR)
        printf("HAL_CAN_ERROR_BR,");
      if (ErrorStatus & HAL_CAN_ERROR_BD)
        printf("HAL_CAN_ERROR_BD,");
      if (ErrorStatus & HAL_CAN_ERROR_CRC)
        printf("HAL_CAN_ERROR_CRC,");
      if (ErrorStatus & HAL_CAN_ERROR_RX_FOV0)
        printf("HAL_CAN_ERROR_RX_FOV0,");
      if (ErrorStatus & HAL_CAN_ERROR_RX_FOV1)
        printf("HAL_CAN_ERROR_RX_FOV1,");
      if (ErrorStatus & HAL_CAN_ERROR_TX_ALST0)
        printf("HAL_CAN_ERROR_TX_ALST0,");
      if (ErrorStatus & HAL_CAN_ERROR_TX_TERR0)
        printf("HAL_CAN_ERROR_TX_TERR0,");
      if (ErrorStatus & HAL_CAN_ERROR_TX_ALST1)
        printf("HAL_CAN_ERROR_TX_ALST1,");
      if (ErrorStatus & HAL_CAN_ERROR_TX_TERR1)
        printf("HAL_CAN_ERROR_TX_TERR1,");
      if (ErrorStatus & HAL_CAN_ERROR_TX_ALST0)
        printf("HAL_CAN_ERROR_TX_ALST0,");
      if (ErrorStatus & HAL_CAN_ERROR_TX_ALST2)
        printf("HAL_CAN_ERROR_TX_ALST2,");
      if (ErrorStatus & HAL_CAN_ERROR_TX_TERR2)
        printf("HAL_CAN_ERROR_TX_TERR2,");
      if (ErrorStatus & HAL_CAN_ERROR_TIMEOUT)
        printf("HAL_CAN_ERROR_TIMEOUT,");
      if (ErrorStatus & HAL_CAN_ERROR_NOT_INITIALIZED)
        printf("HAL_CAN_ERROR_NOT_INITIALIZED,");
      if (ErrorStatus & HAL_CAN_ERROR_NOT_READY)
        printf("HAL_CAN_ERROR_NOT_READY,");
      if (ErrorStatus & HAL_CAN_ERROR_NOT_STARTED)
        printf("HAL_CAN_ERROR_NOT_STARTED,");
      if (ErrorStatus & HAL_CAN_ERROR_PARAM)
        printf("HAL_CAN_ERROR_PARAM,");
      if (ErrorStatus & HAL_CAN_ERROR_INTERNAL)
        printf("HAL_CAN_ERROR_INTERNAL,");
      printf("\n");
    }
  }
  HAL_CAN_ResetError(hcan);
}

/**
  * @brief  Rx FIFO 0 message pending callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
  if (hcan->Instance == CAN1) {
    if (rx_header.IDE == CAN_ID_EXT && rx_header.RTR == CAN_RTR_DATA && rx_header.DLC == 8) { // only accept standard Loxone packages
      LoxCanMessage msg;
      msg.identifier = rx_header.ExtId;
      memcpy(msg.can_data, rx_data, 8);
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      BaseType_t xResult = xQueueSendToBackFromISR(&gCanReceiveQueue, &msg, &xHigherPriorityTaskWoken);
      assert_param(xResult != pdFAIL);
      xResult = xEventGroupSetBitsFromISR(gEventGroup, eMainEvents_LoxCanMessageReceived, &xHigherPriorityTaskWoken);
      if (xResult != pdFAIL) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
    }
  }
}

/**
  * @brief  Initializes the CAN MSP.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
extern "C" void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {

    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpioRX = {
      .Pin = CAN_RX_GPIO_PIN,
      .Mode = GPIO_MODE_INPUT,
      .Pull = GPIO_NOPULL,
    };
    HAL_GPIO_Init(CAN_GPIO_PORT, &gpioRX);
    GPIO_InitTypeDef gpioTX = {
      .Pin = CAN_TX_GPIO_PIN,
      .Mode = GPIO_MODE_AF_PP,
      .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    HAL_GPIO_Init(CAN_GPIO_PORT, &gpioTX);

    __HAL_AFIO_REMAP_CAN1_2();

    // interrupt init for CAN
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
  }
}

/**
  * @brief  DeInitializes the CAN MSP.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
extern "C" void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {
    __HAL_RCC_CAN1_CLK_DISABLE();
    HAL_GPIO_DeInit(CAN_GPIO_PORT, CAN_RX_GPIO_PIN | CAN_TX_GPIO_PIN);
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
  }
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
* @brief This function handles USB high priority or CAN TX interrupts.
*/
extern "C" void CAN1_TX_IRQHandler(void) {
  HAL_CAN_IRQHandler(&gCan);
}

/**
* @brief This function handles USB low priority or CAN RX0 interrupts.
*/
extern "C" void CAN1_RX0_IRQHandler(void) {
  HAL_CAN_IRQHandler(&gCan);
}

/**
* @brief This function handles CAN RX1 interrupt.
*/
extern "C" void CAN1_RX1_IRQHandler(void) {
  HAL_CAN_IRQHandler(&gCan);
}

/**
* @brief This function handles CAN SCE interrupt.
*/
extern "C" void CAN1_SCE_IRQHandler(void) {
  HAL_CAN_IRQHandler(&gCan);
}