//
//  LoxCANDriver_STM32.cpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxCANDriver_STM32.hpp"
#include "LoxExtension.hpp"
#include "stm32f1xx_hal_conf.h"
#include "system.hpp"
#include <__cross_studio_io.h>
#include <string.h>

#define CAN_GPIO_PORT GPIOB
#define CAN_RX_GPIO_PIN GPIO_PIN_8
#define CAN_TX_GPIO_PIN GPIO_PIN_9

static CAN_HandleTypeDef gCan;
static LoxCANDriver_STM32 *gCANDriver;

LoxCANDriver_STM32::LoxCANDriver_STM32(tLoxCANDriverType type) : LoxCANBaseDriver(type) {
}

/***
 *  CAN RX Task to forward messages and timers to all extensions
 ***/
void LoxCANDriver_STM32::vCANRXTask(void *pvParameters) {
  LoxCANDriver_STM32 *_this = (LoxCANDriver_STM32 *)pvParameters;
  while (1) {
    unsigned events = ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR, &gMainEvent, eMainEvents_CanMessaged | eMainEvents_10ms, CTL_TIMEOUT_DELAY, 5u);
    if (events & eMainEvents_CanMessaged) {
      unsigned rq = ctl_fifo_num_used(&_this->receiveFifo);
      _this->statistics.RQ = rq;
      if (rq > _this->statistics.mRQ)
        _this->statistics.mRQ = rq;
      LoxCanMessage *message;
      while ((message = (LoxCanMessage *)ctl_fifo_peek(&_this->receiveFifo)) != NULL) {
        _this->ReceiveMessage(*message);
        ctl_fifo_remove(&_this->receiveFifo);
      }
    }
    if (events & eMainEvents_10ms) {
      _this->Heartbeat();
    }
  }
}

/***
 *  CAN TX Task to send pending messages to the CAN bus
 ***/
void LoxCANDriver_STM32::vCANTXTask(void *pvParameters) {
  LoxCANDriver_STM32 *_this = (LoxCANDriver_STM32 *)pvParameters;
  while (1) {
    unsigned events = ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR, &_this->transmitEvent, eMainEvents_CanMessaged, CTL_TIMEOUT_DELAY, 5u);
    if (events & eMainEvents_CanMessaged) {
      LoxCanMessage *message;
      while ((message = (LoxCanMessage *)ctl_fifo_peek(&_this->transmitFifo)) != NULL) {
#if DEBUG
        debug_printf("CANS:");
        message->print(*_this);
#endif
        const CAN_TxHeaderTypeDef hdr = {
            .ExtId = message->identifier,
            .IDE = CAN_ID_EXT,
            .RTR = CAN_RTR_DATA,
            .DLC = 8,
            .TransmitGlobalTime = DISABLE,
        };
        uint32_t txMailbox = 0;
        HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&gCan, &hdr, message->can_data, &txMailbox);
        if (status != HAL_OK)
          break;
        ctl_fifo_remove(&_this->transmitFifo);
        unsigned tq = ctl_fifo_num_used(&_this->transmitFifo);
        _this->statistics.TQ = tq;
        if (tq > _this->statistics.mTQ)
          _this->statistics.mTQ = tq;
        ++_this->statistics.Sent;
        ctl_timeout_wait(ctl_get_current_time() + 4); // wait a little bit till looking for another message
      }
    }
  }
}

/***
 *  Initialize the CAN bus and all tasks, etc
 ***/
void LoxCANDriver_STM32::Startup(void) {
  gCANDriver = this;

  ctl_fifo_init(&transmitFifo, transmitBuffer, sizeof(transmitBuffer[0]), sizeof(transmitBuffer) / sizeof(transmitBuffer[0]), &transmitEvent, eMainEvents_CanMessaged);
  ctl_fifo_init(&receiveFifo, receiveBuffer, sizeof(receiveBuffer[0]), sizeof(receiveBuffer) / sizeof(receiveBuffer[0]), &gMainEvent, eMainEvents_CanMessaged);

#define RX_STACKSIZE 256
  static unsigned sCANTXTaskStack[1 + RX_STACKSIZE + 1];
  static CTL_TASK_t sCANTXTask;
  sCANTXTaskStack[0] = sCANTXTaskStack[1 + RX_STACKSIZE] = 0xfacefeed; // put marker values at the words before/after the stack
  ctl_task_run(&sCANTXTask, 0x20, LoxCANDriver_STM32::vCANTXTask, this, "CAN_TX", RX_STACKSIZE, sCANTXTaskStack + 1, 0);

#define TX_STACKSIZE 256
  static unsigned sCANRXTaskStack[1 + TX_STACKSIZE + 1];
  sCANRXTaskStack[0] = sCANRXTaskStack[1 + TX_STACKSIZE] = 0xfacefeed; // put marker values at the words before/after the stack
  static CTL_TASK_t sCANRXTask;
  ctl_task_run(&sCANRXTask, 0x10, LoxCANDriver_STM32::vCANRXTask, this, "CAN_RX", TX_STACKSIZE, sCANRXTaskStack + 1, 0);

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
  gCan.Init.Prescaler = HAL_RCC_GetPCLK1Freq() / 16 / (this->isLoxoneLinkBusDriver() ? 125000 : 50000); // 16tq (see above)
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

  // do not filter anything, because we might implement more than one extension/device
  FilterAllowAll(0);

  // FYI: At least one filter is required to be able to receive any data.
  LoxCANBaseDriver::Startup();
}

/***
 *  Setup CAN filters. While an AllowAll filter is working (we can filter in software), it is probably
 *  desirable to filter in "hardware" everything we do not care about.
 ***/
void LoxCANDriver_STM32::FilterSetup(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment) {
#if 0
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
#endif
  // an all filter is active anyway (see above). This is necessary for legacy extensions and support for multiple extensions on the same system.
}

/***
 *  This Filter is a default filter, which allows to listen to all extended messages on the CAN bus
 ***/
void LoxCANDriver_STM32::FilterAllowAll(uint32_t filterBank) {
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
 *  CAN error reporting and statistics
 ***/
uint32_t LoxCANDriver_STM32::GetErrorCounter() const {
  return this->statistics.Err;
}

uint8_t LoxCANDriver_STM32::GetTransmitErrorCounter() const {
  return gCan.Instance->ESR >> 16; // Least significant byte of the 9-bit transmit error counter
}

uint8_t LoxCANDriver_STM32::GetReceiveErrorCounter() const {
  return gCan.Instance->ESR >> 24; // Receive error counter
}

/***
 *  Send a message by putting it into the transmission queue
 ***/
void LoxCANDriver_STM32::SendMessage(LoxCanMessage &message) {
  ctl_fifo_add(&this->transmitFifo, &message);
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
      LoxCanMessage message;
      message.identifier = rx_header.ExtId;
      memmove(message.can_data, rx_data, 8);
      ctl_fifo_add(&gCANDriver->receiveFifo, &message);
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
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 0, 0);
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