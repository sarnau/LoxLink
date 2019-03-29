//
//  LoxLegacyModbusExtension.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxLegacyModbusExtension.hpp"
#if EXTENSION_MODBUS
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
static uint8_t gUART_RX_Buffer[Modbus_RX_BUFFERSIZE];
static StreamBufferHandle_t gUART_RX_Stream;

/***
 *  Constructor
 ***/
LoxLegacyModbusExtension::LoxLegacyModbusExtension(LoxCANBaseDriver &driver, uint32_t serial)
  : LoxLegacyExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_ModbusExtension << 24), eDeviceType_t_ModbusExtension, 0, 9000822) {
  assert(sizeof(sModbusConfig) == 0x810);
}

/***
 *  New configuration was loaded
 ***/
void LoxLegacyModbusExtension::config_load(void)
{
}

/***
 *  Forward a received buffer to the CAN bus to the Miniserver
 ***/
void LoxLegacyModbusExtension::forwardBuffer(const uint8_t *buffer, size_t byteCount) {
  // ACK/NAK only works if a checksum mode is active
//  if (this->checksumMode != eModbusChecksumMode_none and (this->hasAck or this->hasNak)) {
//    bool checksumValid = false;
//    uint16_t checksum = crc16_Modus(buffer, byteCount - 2);
//    checksumValid = buffer[byteCount - 2] == (checksum & 0xFF) and buffer[byteCount - 1] == (checksum >> 8);
//    if (checksumValid) {
//      if (this->hasAck)
//        sendBuffer(&this->ack_byte, 1);
//    } else {
//      if (this->hasNak)
//        sendBuffer(&this->nak_byte, 1);
//    }
//  }
#if DEBUG && 1
  debug_print_buffer(buffer, byteCount, "Modbus MS:");
#endif
  send_fragmented_data(FragCmd_C232_bytes_received, buffer, byteCount);
}

/***
 *  Send a buffer to the Modbus
 ***/
void LoxLegacyModbusExtension::sendBuffer(const uint8_t *buffer, size_t byteCount) {
#if DEBUG && 1
  debug_print_buffer(buffer, byteCount, "Modbus TX:");
#endif
  for (int i = 0; i < byteCount; ++i) {
    xQueueSendToBack(&this->txQueue, &buffer[i], 0);
  }
}

/***
 *  Modbus RX Task
 ***/
void LoxLegacyModbusExtension::vModbusRXTask(void *pvParameters) {
  LoxLegacyModbusExtension *_this = (LoxLegacyModbusExtension *)pvParameters;
  static uint8_t buffer[Modbus_RX_BUFFERSIZE];
  size_t bufferFill = 0;
  while (1) {
    size_t byteCount = xStreamBufferReceive(gUART_RX_Stream, buffer + bufferFill, Modbus_RX_BUFFERSIZE - bufferFill, 10);
    bufferFill += byteCount;
#if DEBUG && 1
    debug_print_buffer(buffer, byteCount, "Modbus RX:");
#endif
    if (byteCount > 0) {
      //      if (not _this->hasEndCharacter or bufferFill == Modbus_RX_BUFFERSIZE) {
      //        // if we don't wait for an end-character or the buffer is full anyway
      //        // just sent the data
      //        _this->forwardBuffer(buffer, bufferFill);
      //        bufferFill = 0;
      //      } else {
      //        for (size_t pos = 0; pos < bufferFill; ++pos) {
      //          if (buffer[pos] == _this->endCharacter) {
      //            size_t count = pos + 1; // number of bytes including the end character
      //                                    // WARNING Loxone Modbus extension forwards `count`, not `count - 1`,
      //                                    // which means it never works with a checksum active, because the
      //                                    // endCharacter is stored in the last byte, which is where the following
      //                                    // code expects the checksum to be. Without the checksum, ACK/NAK will
      //                                    // obviously not work. So, for Loxone it will always send NAK (and
      //                                    // sometimes, 1/256%, ACK). Feels like a Loxone bug.
      //            _this->forwardBuffer(buffer, count);
      //            // move the remaining bytes down
      //            bufferFill -= count;
      //            memmove(buffer, buffer + count, bufferFill);
      //            break;
      //          }
      //        }
      //      }
    }
  }
}

/***
 *  Modbus TX Task
 ***/
void LoxLegacyModbusExtension::vModbusTXTask(void *pvParameters) {
  LoxLegacyModbusExtension *_this = (LoxLegacyModbusExtension *)pvParameters;
  while (1) {
    uint8_t byte;
    while (xQueueReceive(&_this->txQueue, &byte, 0)) {
      HAL_StatusTypeDef status = HAL_UART_Transmit(&gUART1, &byte, sizeof(byte), 50);
      if (status != HAL_OK) {
#if DEBUG
        printf("### Modbus TX error %d\n", status);
#endif
      }
    }
  }
}

/***
 *  Setup GPIOs
 ***/
void LoxLegacyModbusExtension::Startup(void) {
  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  static StaticStreamBuffer_t gUART_RX_Buffer_Stuct;
  gUART_RX_Stream = xStreamBufferCreateStatic(Modbus_RX_BUFFERSIZE, 1, gUART_RX_Buffer, &gUART_RX_Buffer_Stuct);

  static uint8_t sModbusTXBuffer[Modbus_TX_BUFFERSIZE];
  xQueueCreateStatic(Modbus_TX_BUFFERSIZE, 1, (uint8_t *)sModbusTXBuffer, &this->txQueue);

  static StackType_t sModbusRXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sModbusRXTask;
  xTaskCreateStatic(LoxLegacyModbusExtension::vModbusRXTask, "ModbusRXTask", configMINIMAL_STACK_SIZE, this, 2, sModbusRXTaskStack, &sModbusRXTask);

  static StackType_t sModbusTXTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sModbusTXTask;
  xTaskCreateStatic(LoxLegacyModbusExtension::vModbusTXTask, "ModbusTXTask", configMINIMAL_STACK_SIZE, this, 2, sModbusTXTaskStack, &sModbusTXTask);

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
    printf("### Modbus ERROR\n");
#endif
  }

  // RXNE Interrupt Enable
  SET_BIT(gUART1.Instance->CR1, USART_CR1_RXNEIE);

  HAL_UART_Receive_IT(&gUART1, &gChar, 1);
}

void LoxLegacyModbusExtension::PacketToExtension(LoxCanMessage &message) {
  switch (message.commandLegacy) {
#if 0
  case Modbus_config_hardware: {
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
    printf("# Modbus config hardware %d%s%d, %d baud, endChar:%d:0x%02x, unknown:0x%02x\n", bits, pstr, stopBits, message.value32, this->hasEndCharacter, this->endCharacter, message.data[2]);
#endif
    if (HAL_UART_DeInit(&gUART1) != HAL_OK) {
#if DEBUG
      printf("### Modbus HAL_UART_DeInit ERROR\n");
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
      printf("### Modbus HAL_UART_Init ERROR\n");
#endif
    }
    HAL_UART_Receive_IT(&gUART1, &gChar, 1);
    break;
  }
#endif
  case Modbus_485_WriteSingleCoil:
  case Modbus_485_WriteSingleRegister:
  case Modbus_485_WriteMultipleRegisters:
  case Modbus_485_WriteMultipleRegisters2:
  case Modbus_485_WriteSingleRegister4:
  case Modbus_485_WriteMultipleRegisters4:
    printf("Modbus write reg cmd:%02x", message.commandLegacy);
    break;
  default:
    LoxLegacyExtension::PacketToExtension(message);
    break;
  }
}

void LoxLegacyModbusExtension::FragmentedPacketToExtension(LoxMsgLegacyFragmentedCommand_t fragCommand, const void *fragData, int size) {
  switch (fragCommand) {
  case FragCmd_Modbus_config: {
    const sModbusConfig *config = (const sModbusConfig *)fragData;
    if(size == sizeof(sModbusConfig) and config->version == 1) { // valid config?
        memmove(&this->config, config, sizeof(sModbusConfig));
        config_load();
    }
    break;
  }
  default:
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
    printf("Modbus RX Stream error");
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

#endif