//
//  LoxLegacyRS232Extension.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxLegacyRS232Extension.hpp"
#include "global_functions.hpp"
#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_uart.h"
#include "stream_buffer.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef gUART1;
static uint8_t gChar;
static uint8_t gUART_RX_Buffer[RS232_RX_BUFFERSIZE];
static StreamBufferHandle_t gUART_RX_Stream;

/***
 *  Constructor
 ***/
LoxLegacyRS232Extension::LoxLegacyRS232Extension(LoxCANBaseDriver &driver, uint32_t serial)
  : LoxLegacyExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_RS232Extension << 24), eDeviceType_t_RS232Extension, 0, 9000822) {
}

/***
 *  RS232 RX Task
 ***/
void LoxLegacyRS232Extension::vRS232RXTask(void *pvParameters) {
  LoxLegacyRS232Extension *_this = (LoxLegacyRS232Extension *)pvParameters;
  while (1) {
    static uint8_t buffer[RS232_RX_BUFFERSIZE];
    size_t byteCount = xStreamBufferReceive(gUART_RX_Stream, buffer, sizeof(buffer), 10);
    if (byteCount > 0) {
#if DEBUG
      debug_print_buffer(buffer, byteCount, "RS232 RX:");
#endif
      _this->send_fragmented_data(FragCmd_C232_bytes_received, &buffer, byteCount);
    }
  }
}

/***
 *  RS232 TX Task
 ***/
void LoxLegacyRS232Extension::vRS232TXTask(void *pvParameters) {
  LoxLegacyRS232Extension *_this = (LoxLegacyRS232Extension *)pvParameters;
  while (1) {
    uint8_t byte;
    while (xQueueReceive(&_this->txQueue, &byte, 0)) {
      HAL_StatusTypeDef status = HAL_UART_Transmit(&gUART1, &byte, sizeof(byte), 0xFFFF);
      if (status != HAL_OK) {
#if DEBUG
        printf("### RS232 TX error %d\n", status);
#endif
      }
    }
  }
}

/***
 *  Setup GPIOs
 ***/
void LoxLegacyRS232Extension::Startup(void) {
  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  static StaticStreamBuffer_t gUART_RX_Buffer_Stuct;
  gUART_RX_Stream = xStreamBufferCreateStatic(sizeof(gUART_RX_Buffer), 1, gUART_RX_Buffer, &gUART_RX_Buffer_Stuct);

  static uint8_t sRS232TXBuffer[RS232_TX_BUFFERSIZE];
  xQueueCreateStatic(sizeof(sRS232TXBuffer) / sizeof(sRS232TXBuffer[0]), sizeof(sRS232TXBuffer[0]), (uint8_t *)sRS232TXBuffer, &this->txQueue);

  static StackType_t sRS232RXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sRS232RXTask;
  xTaskCreateStatic(LoxLegacyRS232Extension::vRS232RXTask, "RS232RXTask", configMINIMAL_STACK_SIZE, this, 2, sRS232RXTaskStack, &sRS232RXTask);

  static StackType_t sRS232TXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sRS232TXTask;
  xTaskCreateStatic(LoxLegacyRS232Extension::vRS232TXTask, "RS232TXTask", configMINIMAL_STACK_SIZE, this, 2, sRS232TXTaskStack, &sRS232TXTask);

  gUART1.Instance = USART1;
  gUART1.Init.BaudRate = 9600;
  gUART1.Init.WordLength = UART_WORDLENGTH_8B;
  gUART1.Init.StopBits = UART_STOPBITS_1;
  gUART1.Init.Parity = UART_PARITY_NONE;
  gUART1.Init.Mode = UART_MODE_TX_RX;
  gUART1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  gUART1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&gUART1) != HAL_OK) {
#if DEBUG
    printf("### RS232 ERROR\n");
#endif
  }

  // RXNE Interrupt Enable
  SET_BIT(gUART1.Instance->CR1, USART_CR1_RXNEIE);

  HAL_UART_Receive_IT(&gUART1, &gChar, 1);
}

void LoxLegacyRS232Extension::PacketToExtension(LoxCanMessage &message) {
  switch (message.commandLegacy) {
  case RS232_config_hardware: {
    int bits = (message.data[0] & 3) + 5; // 5..8
    int parity = (message.data[0] >> 2) & 0xf;
    int stopBits = (message.data[0] & 0x40) ? 2 : 1;
    this->hasEndCharacter = (message.data[0] & 0x80) == 0x80;
    this->endCharacter = message.data[1];
#if DEBUG
    const char *pstr = "?";
    switch (parity) {
    case 0:
      pstr = "N";
      break;
    case 1:
      pstr = "E";
      break;
    case 2:
      pstr = "O";
      break;
    case 3:
      pstr = "0";
      break;
    case 4: // seems to be broken in Loxone Config 10, because it always sends 3, instead of 4.
      pstr = "1";
      break;
    }
    printf("# RS232 config hardware %d%s%d, %d baud, endChar:%d:0x%02x, unknown:0x%02x\n", bits, pstr, stopBits, message.value32, this->hasEndCharacter, this->endCharacter, message.data[2]);
#endif
    if (HAL_UART_DeInit(&gUART1) != HAL_OK) {
#if DEBUG
      printf("### RS232 HAL_UART_DeInit ERROR\n");
#endif
    }
    gUART1.Instance = USART1;
    gUART1.Init.BaudRate = message.value32;
    gUART1.Init.WordLength = (bits == 8) ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    gUART1.Init.StopBits = stopBits == 1 ? UART_STOPBITS_1 : UART_STOPBITS_2;
    gUART1.Init.Parity = parity == 0 ? UART_PARITY_NONE : ((parity == 1) ? UART_PARITY_EVEN : UART_PARITY_ODD);
    gUART1.Init.Mode = UART_MODE_TX_RX;
    gUART1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    gUART1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&gUART1) != HAL_OK) {
#if DEBUG
      printf("### RS232 HAL_UART_Init ERROR\n");
#endif
    }
    HAL_UART_Receive_IT(&gUART1, &gChar, 1);
    break;
  }
  case RS232_send_bytes: {
    int pos = message.data[0];
    if (pos) {
      //      printf("# RS232 send data #%d: Bytes:0x%02x.0x%02x.0x%02x.0x%02x.0x%02x.0x%02x\n", pos, message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6]);
      int offset = pos * 6 - 2; // pos 0 doesn't have 6 bytes, but only 4, so we need to subtract 2
      int count = sizeof(this->sendData) - offset;
      if (count > 0) { // avoid a buffer overflow
        if (count > 6)
          count = 6;
        this->sendFill += count;
        memcpy(&this->sendData[offset], &message.data[1], count);
      }
    } else {
      //      printf("# RS232 send header: %d bytes, CRC:0x%02x Bytes:0x%02x.0x%02x.0x%02x.0x%02x\n", message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6]);
      this->sendCount = message.data[1];
      this->sendCRC = message.data[2];
      memcpy(this->sendData, &message.data[3], 4);
      this->sendFill = 4;
    }
    if (this->sendFill >= this->sendCount && this->sendCRC == crc8_default(this->sendData, this->sendCount)) {
#if DEBUG
      debug_print_buffer(this->sendData, this->sendCount, "RS232 TX:");
#endif
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      BaseType_t xResult = pdFAIL;
      for (int i = 0; i < this->sendCount; ++i) {
        xResult = xQueueSendToBackFromISR(&this->txQueue, &this->sendData[i], &xHigherPriorityTaskWoken);
        if (xResult == pdFAIL)
          break;
      }
      if (xResult != pdFAIL) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
    }
    break;
  }
  case RS232_config_protocol: {
    this->hasAck = message.data[0];
    this->ack_byte = message.data[2];
    this->hasNak = message.data[1];
    this->nak_byte = message.data[3];
    this->checksumMode = eRS232ChecksumMode(message.data[4]);
#if DEBUG
    const char *checksumModeStr = "?";
    switch (this->checksumMode) {
    case eRS232ChecksumMode_none:
      checksumModeStr = "None";
      break;
    case eRS232ChecksumMode_XOR:
      checksumModeStr = "XOR";
      break;
    case eRS232ChecksumMode_Sum:
      checksumModeStr = "Sum";
      break;
    case eRS232ChecksumMode_CRC:
      checksumModeStr = "CRC";
      break;
    case eRS232ChecksumMode_ModbusCRC:
      checksumModeStr = "Modbus CRC";
      break;
    case eRS232ChecksumMode_Fronius:
      checksumModeStr = "Fronius";
      break;
    }
    printf("# RS232 config protocol ACK:%d:0x%02x NAK:%d:0x%02x Checksum-Mode:%s\n", this->hasAck, this->ack_byte, this->hasNak, this->nak_byte, checksumModeStr);
#endif
    break;
  }
  default:
    LoxLegacyExtension::PacketToExtension(message);
    break;
  }
}

/**
* @brief UART MSP Initialization
* This function configures the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (huart->Instance == USART1) {
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
}

/**
* @brief UART MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
extern "C" void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

    /* USART1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  }
}

/**
  * @brief  Rx Transfer completed callbacks.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  // Attempt to send the string to the stream buffer.
  BaseType_t xHigherPriorityTaskWoken = pdFALSE; // Initialised to pdFALSE.
  size_t xBytesSent = xStreamBufferSendFromISR(gUART_RX_Stream, &gChar, 1, &xHigherPriorityTaskWoken);
  if (xBytesSent != 1) {
#if DEBUG
    printf("RS232 RX Stream error");
#endif
  }
  HAL_UART_Receive_IT(&gUART1, &gChar, 1);

  // If xHigherPriorityTaskWoken was set to pdTRUE inside
  // xStreamBufferSendFromISR() then a task that has a priority above the
  // priority of the currently executing task was unblocked and a context
  // switch should be performed to ensure the ISR returns to the unblocked
  // task.  In most FreeRTOS ports this is done by simply passing
  // xHigherPriorityTaskWoken into taskYIELD_FROM_ISR(), which will test the
  // variables value, and perform the context switch if necessary.  Check the
  // documentation for the port in use for port specific instructions.
  if (xHigherPriorityTaskWoken) {
    taskYIELD();
  }
}

extern "C" void USART1_IRQHandler(void) {
  HAL_UART_IRQHandler(&gUART1);
}