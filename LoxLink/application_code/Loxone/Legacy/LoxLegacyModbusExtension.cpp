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

#define RS485_TX_PIN_Pin GPIO_PIN_10
#define RS485_TX_PIN_GPIO_Port GPIOB
#define RS485_RX_PIN_Pin GPIO_PIN_11
#define RS485_RX_PIN_GPIO_Port GPIOB
#define RS485_RX_ENABLE_Pin GPIO_PIN_4
#define RS485_RX_ENABLE_GPIO_Port GPIOC
#define RS485_TX_ENABLE_Pin GPIO_PIN_5
#define RS485_TX_ENABLE_GPIO_Port GPIOC

static UART_HandleTypeDef huart3;
static uint8_t gChar;
static uint8_t gUART_RX_Buffer[Modbus_RX_BUFFERSIZE];
static StreamBufferHandle_t gUART_RX_Stream;

/***
 *  Constructor
 ***/
LoxLegacyModbusExtension::LoxLegacyModbusExtension(LoxCANBaseDriver &driver, uint32_t serial)
  : LoxLegacyExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_ModbusExtension << 24), eDeviceType_t_ModbusExtension, 0, 10020326, &config, sizeof(config)) {
  assert(sizeof(sModbusConfig) == 0x810);
}

/***
 *  RS485 requires to switch between TX/RX mode. Default is RX.
 ***/
void LoxLegacyModbusExtension::set_tx_mode(bool txMode) {
  // The board strangely has independed pins to control RX/TX enable, but they are mutally exclusive
  // Also: RX is negated in the MAX3485, so both pins always have to have the same state, which leads
  // to the valid question: why does the board have two pins for it anyway?
  GPIO_PinState pinState = txMode ? GPIO_PIN_SET : GPIO_PIN_RESET;
  HAL_GPIO_WritePin(RS485_RX_ENABLE_GPIO_Port, RS485_RX_ENABLE_Pin, pinState);
  HAL_GPIO_WritePin(RS485_TX_ENABLE_GPIO_Port, RS485_TX_ENABLE_Pin, pinState);
}

/***
 *  New configuration was loaded
 ***/
void LoxLegacyModbusExtension::config_load(void) {
  printf("config RS485 baudrate : %ld baud\n", this->config.baudrate);
  printf("config RS485 word length : %ld bits\n", this->config.wordLength);
  printf("config RS485 parity : %ld\n", this->config.parity);
  printf("config RS485 twoStopBits : %ld\n", this->config.twoStopBits);
  printf("config protocol : %ld\n", this->config.protocol);
  if (this->config.manualTimingFlag) {
    printf("config timingPause : %ld\n", this->config.timingPause);
    printf("config timingTimeout : %ld\n", this->config.timingTimeout);
  }
  for (int i = 0; i < this->config.entryCount; ++i) {
    const sModbusDeviceConfig *d = &this->config.devices[i];
    printf("config device #%d: ", i);
    switch (d->functionCode) {
    case 1:
      printf("Read coil status");
      break;
    case 2:
      printf("Read input status");
      break;
    case 3:
      printf("Read holding register");
      break;
    case 4:
      printf("Read input register");
      break;
      // these are allowed for actors, but not used in the config
      //        case 5: printf("Write single coil"); break;
      //        case 6: printf("Write single register"); break;
      //        case 15: printf("Force multiple coils"); break;
      //        case 16: printf("Preset multiple registers"); break;
    default: // should not happen with Loxone Config
      printf("functionCode(%d)", d->functionCode);
      break;
    }
    printf(" addr:0x%02x ", d->address);
    printf(" reg:0x%02x options:", d->regNumber);
    uint16_t flags = d->pollingCycle >> 16;
    if (flags & 0x4000)
      printf("reg order HL,");
    else
      printf("reg order LH,");
    if (flags & 0x8000)
      printf("Little Endian,");
    else
      printf("Big Endian,");
    if (flags & 0x2000)
      printf("2 regs for 32-bit");
    uint32_t cycle = d->pollingCycle & 0xFFFF;
    if (cycle && flags & 0x1000) { // in seconds
      cycle = (cycle & 0xFFF) * 1000;
    } else { // in ms
      cycle = cycle * 100;
    }
    printf(" %.1fms\n", cycle * 0.001);
  }
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
      HAL_StatusTypeDef status = HAL_UART_Transmit(&huart3, &byte, sizeof(byte), 50);
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

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK) {
#if DEBUG
    printf("### Modbus ERROR\n");
#endif
  }

  // RXNE Interrupt Enable
  SET_BIT(huart3.Instance->CR1, USART_CR1_RXNEIE);

  HAL_UART_Receive_IT(&huart3, &gChar, 1);
}

void LoxLegacyModbusExtension::PacketToExtension(LoxCanMessage &message) {
  switch (message.commandLegacy) {
  case Modbus_485_WriteSingleCoil:
  case Modbus_485_WriteSingleRegister:
  case Modbus_485_WriteMultipleRegisters:
  case Modbus_485_WriteMultipleRegisters2:
  case Modbus_485_WriteSingleRegister4:
  case Modbus_485_WriteMultipleRegisters4: {
    static uint8_t modbusBuffer[16];
    int byteCount = 2;
    int offset = 2;
    int regCount = 1;
    modbusBuffer[offset++] = message.data[0]; // Modbus address
    switch (message.commandLegacy) {
    case Modbus_485_WriteSingleCoil:
      modbusBuffer[offset++] = tModbusCode_WriteSingleCoil;
      break;
    case Modbus_485_WriteSingleRegister:
      modbusBuffer[offset++] = tModbusCode_WriteSingleRegister;
      break;
    case Modbus_485_WriteMultipleRegisters:
      modbusBuffer[offset++] = tModbusCode_WriteMultipleRegisters;
      regCount = 2;
      break;
    case Modbus_485_WriteMultipleRegisters2:
      modbusBuffer[offset++] = tModbusCode_WriteMultipleRegisters;
      byteCount = 4;
      break;
    case Modbus_485_WriteSingleRegister4:
      modbusBuffer[offset++] = tModbusCode_WriteSingleRegister;
      byteCount = 4;
      break;
    case Modbus_485_WriteMultipleRegisters4:
      modbusBuffer[offset++] = tModbusCode_WriteMultipleRegisters;
      break;
    default: // should never happen
      break;
    }
    uint16_t reg = *(uint16_t *)&message.data[1]; // Modbus IO-address
    modbusBuffer[offset++] = reg >> 8;
    modbusBuffer[offset++] = reg & 0xFF;
    switch (message.commandLegacy) {
    case Modbus_485_WriteMultipleRegisters:
    case Modbus_485_WriteMultipleRegisters2:
    case Modbus_485_WriteMultipleRegisters4:
      modbusBuffer[offset++] = regCount >> 8; // number of registers
      modbusBuffer[offset++] = regCount & 0xFF;
      modbusBuffer[offset++] = byteCount; // number of transferred bytes
      break;
    default:
      break;
    }
    memcpy(modbusBuffer + offset, &message.data[3], byteCount);
    offset += byteCount;
    uint16_t crc = crc16_Modus(modbusBuffer, offset);
    modbusBuffer[offset++] = crc & 0xFF;
    modbusBuffer[offset++] = crc >> 8;
    debug_print_buffer(modbusBuffer, offset, "Modbus write:");
    break;
  }
  default:
    LoxLegacyExtension::PacketToExtension(message);
    break;
  }
}

void LoxLegacyModbusExtension::FragmentedPacketToExtension(LoxMsgLegacyFragmentedCommand_t fragCommand, const void *fragData, int size) {
  switch (fragCommand) {
  case FragCmd_Modbus_config: {
    const sModbusConfig *config = (const sModbusConfig *)fragData;
    if (size <= sizeof(sModbusConfig) and config->version == 1) { // valid config?
      config_load();
    }
    break;
  }
  default:
    break;
  }
}

/***
 *  After a start request some extensions have to send additional messages
 ***/
void LoxLegacyModbusExtension::StartRequest() {
  sendCommandWithValues(config_check_CRC, 0, 1 /* config version */, 0); // required, otherwise it is considered offline
}

/**
* @brief UART MSP Initialization
* This function configures the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (huart->Instance == USART3) {
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RS485_RX_ENABLE_GPIO_Port, RS485_RX_ENABLE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RS485_TX_ENABLE_GPIO_Port, RS485_TX_ENABLE_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : PCPin PCPin */
    GPIO_InitStruct.Pin = RS485_RX_ENABLE_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RS485_RX_ENABLE_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = RS485_TX_ENABLE_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RS485_TX_ENABLE_GPIO_Port, &GPIO_InitStruct);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    GPIO_InitStruct.Pin = RS485_TX_PIN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_TX_PIN_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RS485_RX_PIN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(RS485_RX_PIN_GPIO_Port, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  }
}

/**
* @brief UART MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
extern "C" void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) {
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    HAL_GPIO_DeInit(RS485_TX_PIN_GPIO_Port, RS485_TX_PIN_Pin);
    HAL_GPIO_DeInit(RS485_RX_PIN_GPIO_Port, RS485_RX_PIN_Pin);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
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

extern "C" void USART3_IRQHandler(void) {
  HAL_UART_IRQHandler(&huart3);
}

#endif