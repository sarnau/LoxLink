#include "MMM_can.h"

#include "stm32f1xx_hal_conf.h"

#define CAN_RX_GPIO_PIN GPIO_PIN_8
#define CAN_TX_GPIO_PIN GPIO_PIN_9
#define CAN_BITRATE 125000

CAN_HandleTypeDef gCan;

void MMM_CAN_ConfigFilter_internal(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment) {
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

void MMM_CAN_LoxNATFilter(uint32_t filterBank, uint8_t loxLink_or_Tree_ID, uint8_t natAddress, uint8_t fromServerFlag, uint32_t filterFIFOAssignment) {
  MMM_CAN_ConfigFilter_internal(
    filterBank,
    ((loxLink_or_Tree_ID & 0x1F) << 24) | (fromServerFlag << 21) | (natAddress << 12),
    0x1F2FF000,
    filterFIFOAssignment);
}

void MMM_CAN_Init() {
  gCan.Instance = CAN1;
  gCan.Init.TimeTriggeredMode = DISABLE;
  gCan.Init.AutoBusOff = ENABLE;
  gCan.Init.AutoWakeUp = DISABLE;
  gCan.Init.AutoRetransmission = DISABLE;
  gCan.Init.ReceiveFifoLocked = DISABLE;
  gCan.Init.TransmitFifoPriority = ENABLE;
  gCan.Init.Mode = CAN_MODE_NORMAL;

  // CAN_SJW_1tq + CAN_BS1_10tq + CAN_BS2_5tq => 16tq => sample point = 68.75% =(1+10)/16
  gCan.Init.Prescaler = HSE_VALUE / 16 / CAN_BITRATE; // 16tq (see above)
  gCan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  gCan.Init.TimeSeg1 = CAN_BS1_10TQ;
  gCan.Init.TimeSeg2 = CAN_BS2_5TQ;
  if (HAL_CAN_Init(&gCan) != HAL_OK) {
    for (;;)
      ;
  }

  HAL_CAN_Start(&gCan);
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_RX_FIFO0_MSG_PENDING); // FIFO 0 message pending interrupt
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_RX_FIFO1_MSG_PENDING); // FIFO 1 message pending interrupt
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_TX_MAILBOX_EMPTY); // Transmit mailbox empty interrupt
  // notify on certain errors:
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_ERROR_PASSIVE); // Error passive interrupt
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_LAST_ERROR_CODE); // Last error code interrupt
  HAL_CAN_ActivateNotification(&gCan, CAN_IT_ERROR); // Error Interrupt
}

/**
  * @brief  Transmission Mailbox 0 complete callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {
  }
}

/**
  * @brief  Transmission Mailbox 1 complete callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {
  }
}

/**
  * @brief  Transmission Mailbox 2 complete callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {
  }
}

/**
  * @brief  Error CAN callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {
  }
  HAL_CAN_ResetError(hcan);
}

/**
  * @brief  Rx FIFO 0 message pending callback.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
  if (hcan->Instance == CAN1) {
  }
}

/**
  * @brief  Initializes the CAN MSP.
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {

    __HAL_RCC_CAN1_CLK_ENABLE();

    GPIO_InitTypeDef gpio;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin = CAN_RX_GPIO_PIN;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin = CAN_TX_GPIO_PIN;
    HAL_GPIO_Init(GPIOA, &gpio);

    // interrupt init for CAN
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
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
void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {
    __HAL_RCC_CAN1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, CAN_RX_GPIO_PIN | CAN_TX_GPIO_PIN);
    HAL_NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
    HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
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
void CAN1_TX_IRQHandler(void) {
  HAL_CAN_IRQHandler(&gCan);
}

/**
* @brief This function handles USB low priority or CAN RX0 interrupts.
*/
void CAN1_RX0_IRQHandler(void) {
  HAL_CAN_IRQHandler(&gCan);
}

/**
* @brief This function handles CAN RX1 interrupt.
*/
void CAN1_RX1_IRQHandler(void) {
  HAL_CAN_IRQHandler(&gCan);
}

/**
* @brief This function handles CAN SCE interrupt.
*/
void CAN1_SCE_IRQHandler(void) {
  HAL_CAN_IRQHandler(&gCan);
}