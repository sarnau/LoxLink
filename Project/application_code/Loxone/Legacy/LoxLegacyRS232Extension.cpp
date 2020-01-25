//
//  LoxLegacyRS232Extension.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxLegacyRS232Extension.hpp"
#if EXTENSION_RS232
#include "global_functions.hpp"
#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_uart.h"
#include "stream_buffer.h"
#include "task.h"
#include <__cross_studio_io.h>
#include <string.h>

static UART_HandleTypeDef huart1;
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
 *  Forward a received buffer to the CAN bus to the Miniserver
 ***/
void LoxLegacyRS232Extension::forwardBuffer(const uint8_t *buffer, size_t byteCount) {
  // ACK/NAK only works if a checksum mode is active
  if (this->checksumMode != eRS232ChecksumMode_none and (this->hasAck or this->hasNak)) {
    bool checksumValid = false;
    switch (this->checksumMode) {
    case eRS232ChecksumMode_none:
      break;
    case eRS232ChecksumMode_XOR: {
      uint8_t checksumXor = 0x00;
      if (byteCount > 1) {
        for (size_t i = 0; i < byteCount - 1; ++i)
          checksumXor ^= buffer[i];
      }
      checksumValid = buffer[byteCount - 1] == checksumXor;
      break;
    }
    case eRS232ChecksumMode_Sum: {
      uint8_t checksumSum = 0x00;
      if (byteCount > 1) {
        for (size_t i = 0; i < byteCount - 1; ++i)
          checksumSum += buffer[i];
      }
      checksumValid = buffer[byteCount - 1] == checksumSum;
      break;
    }
    case eRS232ChecksumMode_CRC:
      checksumValid = buffer[byteCount - 1] == crc8_default(buffer, byteCount - 1);
      break;
    case eRS232ChecksumMode_ModbusCRC: {
      uint16_t checksum = crc16_Modus(buffer, byteCount - 2);
      checksumValid = buffer[byteCount - 2] == (checksum & 0xFF) and buffer[byteCount - 1] == (checksum >> 8);
      break;
    }
    case eRS232ChecksumMode_Fronius:
      uint8_t checksumSum = 0x00;
      if (byteCount > 4) {
        for (size_t i = 3; i < byteCount - 1; ++i)
          checksumSum += buffer[i];
      }
      checksumValid = buffer[0] == 0x80 and buffer[1] == 0x80 and buffer[2] == 0x80 and buffer[byteCount - 1] == checksumSum;
      break;
    }
    if (checksumValid) {
      if (this->hasAck)
        sendBuffer(&this->ack_byte, 1);
    } else {
      if (this->hasNak)
        sendBuffer(&this->nak_byte, 1);
    }
  }
#if DEBUG && 0
  debug_print_buffer(buffer, byteCount, "RS232 MS:");
#endif
  send_fragmented_message(FragCmd_C232_bytes_received, buffer, byteCount);
}

/***
 *  Send a buffer to the RS232
 ***/
void LoxLegacyRS232Extension::sendBuffer(const uint8_t *buffer, size_t byteCount) {
#if DEBUG && 0
  debug_print_buffer(buffer, byteCount, "RS232 TX:");
#endif
  for (int i = 0; i < byteCount; ++i) {
    xQueueSendToBack(&this->txQueue, &buffer[i], 0);
  }
}

/***
 *  RS232 RX Task
 ***/
void LoxLegacyRS232Extension::vRS232RXTask(void *pvParameters) {
  LoxLegacyRS232Extension *_this = (LoxLegacyRS232Extension *)pvParameters;
  static uint8_t buffer[RS232_RX_BUFFERSIZE];
  size_t bufferFill = 0;
  while (1) {
    size_t byteCount = xStreamBufferReceive(gUART_RX_Stream, buffer + bufferFill, RS232_RX_BUFFERSIZE - bufferFill, 10);
    bufferFill += byteCount;
#if DEBUG && 0
    debug_print_buffer(buffer, byteCount, "RS232 RX:");
#endif
    if (byteCount > 0) {
      if (not _this->hasEndCharacter or bufferFill == RS232_RX_BUFFERSIZE) {
        // if we don't wait for an end-character or the buffer is full anyway
        // just sent the data
        _this->forwardBuffer(buffer, bufferFill);
        bufferFill = 0;
      } else {
        for (size_t pos = 0; pos < bufferFill; ++pos) {
          if (buffer[pos] == _this->endCharacter) {
            size_t count = pos + 1; // number of bytes including the end character
             // WARNING Loxone RS232 extension forwards `count`, not `count - 1`,
             // which means it never works with a checksum active, because the
             // endCharacter is stored in the last byte, which is where the following
             // code expects the checksum to be. Without the checksum, ACK/NAK will
             // obviously not work. So, for Loxone it will always send NAK (and
             // sometimes, 1/256%, ACK). Feels like a Loxone bug.
            _this->forwardBuffer(buffer, count);
            // move the remaining bytes down
            bufferFill -= count;
            memmove(buffer, buffer + count, bufferFill);
            break;
          }
        }
      }
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
      HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, &byte, sizeof(byte), 50);
      if (status != HAL_OK) {
#if DEBUG
        debug_printf("### RS232 TX error %d\n", status);
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
  gUART_RX_Stream = xStreamBufferCreateStatic(RS232_RX_BUFFERSIZE, 1, gUART_RX_Buffer, &gUART_RX_Buffer_Stuct);

  static uint8_t sRS232TXBuffer[RS232_TX_BUFFERSIZE];
  xQueueCreateStatic(RS232_TX_BUFFERSIZE, 1, (uint8_t *)sRS232TXBuffer, &this->txQueue);

  static StackType_t sRS232RXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sRS232RXTask;
  xTaskCreateStatic(LoxLegacyRS232Extension::vRS232RXTask, "RS232RXTask", configMINIMAL_STACK_SIZE, this, 2, sRS232RXTaskStack, &sRS232RXTask);

  static StackType_t sRS232TXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sRS232TXTask;
  xTaskCreateStatic(LoxLegacyRS232Extension::vRS232TXTask, "RS232TXTask", configMINIMAL_STACK_SIZE, this, 2, sRS232TXTaskStack, &sRS232TXTask);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
#if DEBUG
    debug_printf("### RS232 ERROR\n");
#endif
  }

  // RXNE Interrupt Enable
  SET_BIT(huart1.Instance->CR1, USART_CR1_RXNEIE);

  HAL_UART_Receive_IT(&huart1, &gChar, 1);
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
    debug_printf("# RS232 config hardware %d%s%d, %d baud, endChar:%d:0x%02x, unknown:0x%02x\n", bits, pstr, stopBits, message.value32, this->hasEndCharacter, this->endCharacter, message.data[2]);
#endif
    if (HAL_UART_DeInit(&huart1) != HAL_OK) {
#if DEBUG
      debug_printf("### RS232 HAL_UART_DeInit ERROR\n");
#endif
    }
    huart1.Instance = USART1;
    huart1.Init.BaudRate = message.value32;
    huart1.Init.WordLength = (bits == 8) ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    huart1.Init.StopBits = stopBits == 1 ? UART_STOPBITS_1 : UART_STOPBITS_2;
    huart1.Init.Parity = parity == 0 ? UART_PARITY_NONE : ((parity == 1) ? UART_PARITY_EVEN : UART_PARITY_ODD);
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
#if DEBUG
      debug_printf("### RS232 HAL_UART_Init ERROR\n");
#endif
    }
    HAL_UART_Receive_IT(&huart1, &gChar, 1);
    break;
  }
  case RS232_send_bytes: {
    int pos = message.data[0];
    if (pos) {
      //      debug_printf("# RS232 send data #%d: Bytes:0x%02x.0x%02x.0x%02x.0x%02x.0x%02x.0x%02x\n", pos, message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6]);
      int offset = pos * 6 - 2; // pos 0 doesn't have 6 bytes, but only 4, so we need to subtract 2
      int count = sizeof(this->sendData) - offset;
      if (count > 0) { // avoid a buffer overflow
        if (count > 6)
          count = 6;
        this->sendFill += count;
        memmove(&this->sendData[offset], &message.data[1], count);
      }
    } else {
      //      debug_printf("# RS232 send header: %d bytes, CRC:0x%02x Bytes:0x%02x.0x%02x.0x%02x.0x%02x\n", message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6]);
      this->sendCount = message.data[1];
      this->sendCRC = message.data[2];
      memmove(this->sendData, &message.data[3], 4);
      this->sendFill = 4;
    }
    if (this->sendFill >= this->sendCount && this->sendCRC == crc8_default(this->sendData, this->sendCount)) {
      sendBuffer(this->sendData, this->sendCount);
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
    debug_printf("# RS232 config protocol ACK:%d:0x%02x NAK:%d:0x%02x Checksum-Mode:%s\n", this->hasAck, this->ack_byte, this->hasNak, this->nak_byte, checksumModeStr);
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
    debug_printf("RS232 RX Stream error");
#endif
  }
  HAL_UART_Receive_IT(huart, &gChar, 1);

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
  HAL_UART_IRQHandler(&huart1);
}
#endif
