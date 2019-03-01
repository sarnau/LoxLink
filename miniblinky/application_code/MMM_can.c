#include "MMM_can.h"

#include "stm32f1xx_hal_conf.h"

#define CAN_GPIO_PORT GPIOB
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
  gCan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  gCan.Init.TimeSeg1 = CAN_BS1_10TQ;
  gCan.Init.TimeSeg2 = CAN_BS2_5TQ;
  gCan.Init.Prescaler = 18; //SystemCoreClock/2 / 16 / CAN_BITRATE; // 16tq (see above)
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

  //  MMM_CAN_ConfigFilter_internal(0, 0, 0, CAN_FILTER_FIFO0);
  CAN_FilterTypeDef filterInit = {
    .FilterIdHigh = 0x0000,
    .FilterIdLow = 0x0000,
    .FilterMaskIdHigh = 0x0000,
    .FilterMaskIdLow = 0x0000,
    .FilterFIFOAssignment = CAN_FILTER_FIFO0,
    .FilterBank = 0,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_32BIT,
    .FilterActivation = CAN_FILTER_ENABLE,
    .SlaveStartFilterBank = 0,
  };
  HAL_CAN_ConfigFilter(&gCan, &filterInit);

  //  printf("freeLevel %d\n", HAL_CAN_GetTxMailboxesFreeLevel(&gCan));
  //  CAN_TxHeaderTypeDef hdr = {
  //    .ExtId = 0x106ff0fd,
  //    .IDE = CAN_ID_EXT,
  //    .RTR = CAN_RTR_DATA,
  //    .DLC = 8,
  //    .TransmitGlobalTime = DISABLE,
  //  };
  //  uint8_t data[8] = {0xff, 0x01, 0x00, 0x00, 0x6c, 0x10, 0x10, 0x13};
  //  uint32_t txMailbox = 0;
  //  HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&gCan, &hdr, data, &txMailbox);
  //  printf("status %d\n", status);
  //  printf("txMailbox 0x%x\n", txMailbox);
  //  printf("freeLevel %d\n", HAL_CAN_GetTxMailboxesFreeLevel(&gCan));
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
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
  if (hcan->Instance == CAN1) {
    printf("EXTID:%x IDE:%d RTR:%d DLC:%d DATA:%x.%x.%x.%x.%x.%x.%x.%x\n", rx_header.ExtId, rx_header.IDE, rx_header.RTR, rx_header.DLC, rx_data[0], rx_data[1], rx_data[2], rx_data[3], rx_data[4], rx_data[5], rx_data[6], rx_data[7]);
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
void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan) {
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