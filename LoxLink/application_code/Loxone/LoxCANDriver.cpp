//
//  LoxCANDriver.cpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxCANDriver.hpp"
#include "LoxExtension.hpp"
#include "event_groups.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define CAN_GPIO_PORT GPIOB
#define CAN_RX_GPIO_PIN GPIO_PIN_8
#define CAN_TX_GPIO_PIN GPIO_PIN_9

#define CAN_RX_QUEUE_SIZE 64
#define CAN_TX_QUEUE_SIZE 64

static CAN_HandleTypeDef gCan;
static StaticQueue_t gCanReceiveQueue;
static EventGroupHandle_t gCANRXEventGroup;
static LoxCANDriver *gCANDriver;

typedef enum {
  eMainEvents_LoxCanMessageReceived = 0x1,
  eMainEvents_10msTimer = 0x2,
} eMainEvents;

LoxCANDriver::LoxCANDriver(tLoxCANDriverType type)
  : driverType(type), extensionCount(0) {
}

/**
  * @brief  SYSTICK callback.
  * @retval None
  */
extern "C" void xPortSysTickHandler(void);
extern "C" void HAL_SYSTICK_Callback(void) {
  xPortSysTickHandler();
  LoxCANDriver::CANSysTick();
}

/***
 *  SysTick callback at 1000Hz
 ***/
void LoxCANDriver::CANSysTick(void) {
  BaseType_t xResult = pdFAIL;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  static uint32_t sMsCounter;
  if (sMsCounter == 0)
    xResult = xEventGroupSetBitsFromISR(gCANRXEventGroup, eMainEvents_10msTimer, &xHigherPriorityTaskWoken);
  if (++sMsCounter > 9)
    sMsCounter = 0;
  if (xResult != pdFAIL) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/***
 *  CAN RX Task to forward messages and timers to all extensions
 ***/
void LoxCANDriver::vCANRXTask(void *pvParameters) {
  LoxCANDriver *_this = (LoxCANDriver *)pvParameters;
  while (1) {
    EventBits_t uxBits = xEventGroupWaitBits(gCANRXEventGroup, eMainEvents_LoxCanMessageReceived | eMainEvents_10msTimer, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & eMainEvents_LoxCanMessageReceived) {
      UBaseType_t rq = uxQueueMessagesWaiting(&gCanReceiveQueue);
      _this->statistics.RQ = rq;
      if (rq > _this->statistics.mRQ)
        _this->statistics.mRQ = rq;
      LoxCanMessage message;
      while (xQueueReceive(&gCanReceiveQueue, &message, 0)) {
        ++_this->statistics.Rcv;
#if DEBUG
        printf("CANR:");
        message.print(*_this);
#endif
        for (int i = 0; i < _this->extensionCount; ++i) {
          _this->extensions[i]->ReceiveMessage(message);
        }
      }
    }
    if (uxBits & eMainEvents_10msTimer) {
      for (int i = 0; i < _this->extensionCount; ++i)
        _this->extensions[i]->Timer10ms();
    }
  }
}

/***
 *  CAN TX Task to send pending messages to the CAN bus
 ***/
void LoxCANDriver::vCANTXTask(void *pvParameters) {
  LoxCANDriver *_this = (LoxCANDriver *)pvParameters;
  const TickType_t xDelay4ms = pdMS_TO_TICKS(4);
  while (1) {
    UBaseType_t tq = uxQueueMessagesWaiting(&gCanReceiveQueue);
    _this->statistics.TQ = tq;
    if (tq > _this->statistics.mTQ)
      _this->statistics.mTQ = tq;
    LoxCanMessage message;
    while (xQueueReceive(&_this->transmitQueue, &message, 0)) {
      ++_this->statistics.Sent;
#if DEBUG
      printf("CANS:");
      message.print(*_this);
#endif
      const CAN_TxHeaderTypeDef hdr = {
        .ExtId = message.identifier,
        .IDE = CAN_ID_EXT,
        .RTR = CAN_RTR_DATA,
        .DLC = 8,
        .TransmitGlobalTime = DISABLE,
      };
      uint32_t txMailbox = 0;
      /*HAL_StatusTypeDef status =*/HAL_CAN_AddTxMessage(&gCan, &hdr, message.can_data, &txMailbox);
      vTaskDelay(xDelay4ms);
    }
  }
}

/***
 *  Initialize the CAN bus and all tasks, etc
 ***/
void LoxCANDriver::Startup(void) {
  gCANDriver = this;

  static StaticEventGroup_t sEventGroup;
  gCANRXEventGroup = xEventGroupCreateStatic(&sEventGroup);

  // queue for 64 messages
  static LoxCanMessage sCanReceiveBuffer[CAN_RX_QUEUE_SIZE];
  xQueueCreateStatic(CAN_RX_QUEUE_SIZE, sizeof(sCanReceiveBuffer[0]), (uint8_t *)sCanReceiveBuffer, &gCanReceiveQueue);

  // queue for 64 messages
  static LoxCanMessage sCanTransmitBuffer[CAN_TX_QUEUE_SIZE];
  xQueueCreateStatic(CAN_TX_QUEUE_SIZE, sizeof(sCanTransmitBuffer[0]), (uint8_t *)sCanTransmitBuffer, &this->transmitQueue);

  static StackType_t sCANTXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sCANTXTask;
  xTaskCreateStatic(LoxCANDriver::vCANTXTask, "CANTXTask", configMINIMAL_STACK_SIZE, this, 2, sCANTXTaskStack, &sCANTXTask);

  static StackType_t sCANRXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sCANRXTask;
  xTaskCreateStatic(LoxCANDriver::vCANRXTask, "CANRXTask", configMINIMAL_STACK_SIZE, this, 2, sCANRXTaskStack, &sCANRXTask);

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
  gCan.Init.Prescaler = HAL_RCC_GetPCLK1Freq() / 16 / (this->driverType == tLoxCANDriverType_LoxoneLink ? 125000 : 50000); // 16tq (see above)
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
#if DEBUG
  FilterAllowAll(10);
#endif
}

/***
 *  Loxone Link or Tree Bus
 ***/
tLoxCANDriverType LoxCANDriver::GetDriverType() const {
  return this->driverType;
}

/***
 *  Add an extension to this driver
 ***/
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
#if DEBUG
  printf("Filter #%d mask:%08x value:%08x\n", filterIndex, 0x1F2FF000, msg.identifier);
#endif
}

/***
 *  CAN error reporting and statistics
 ***/
uint32_t LoxCANDriver::GetErrorCounter() const {
  return this->statistics.Err;
}

uint8_t LoxCANDriver::GetTransmitErrorCounter() const {
  return gCan.Instance->ESR >> 16; // Least significant byte of the 9-bit transmit error counter
}

uint8_t LoxCANDriver::GetReceiveErrorCounter() const {
  return gCan.Instance->ESR >> 24; // Receive error counter
}

#if DEBUG
void LoxCANDriver::StatisticsPrint() const {
  printf("Sent:%d;", this->statistics.Sent);
  printf("Rcv:%d;", this->statistics.Rcv);
  printf("Err:%d;", this->statistics.Err);
  printf("REC:%d;", this->GetReceiveErrorCounter());
  printf("TEC:%d;", this->GetTransmitErrorCounter());
  printf("HWE:%d;", this->statistics.HWE);
  printf("TQ:%d;", this->statistics.TQ);
  printf("mTQ:%d;", this->statistics.mTQ);
  printf("QOvf:%d;", this->statistics.QOvf);
  printf("RQ:%d;", this->statistics.RQ);
  printf("mRQ:%d;\n", this->statistics.mRQ);
}
#endif

void LoxCANDriver::StatisticsReset() {
  memset(&this->statistics, 0, sizeof(this->statistics));
}

/***
 *  A ms delay, implemented via RTOS
 ***/
void LoxCANDriver::Delay(int msDelay) const {
  vTaskDelay(pdMS_TO_TICKS(msDelay));
}

/***
 *  Send a message by putting it into the transmission queue
 ***/
void LoxCANDriver::SendMessage(LoxCanMessage &message) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  BaseType_t xResult = xQueueSendToBackFromISR(&this->transmitQueue, &message, &xHigherPriorityTaskWoken);
  if (xResult != pdFAIL) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
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
      gCANDriver->statistics.HWE++;
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
      xResult = xEventGroupSetBitsFromISR(gCANRXEventGroup, eMainEvents_LoxCanMessageReceived, &xHigherPriorityTaskWoken);
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