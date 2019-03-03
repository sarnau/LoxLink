#include "stm32f1xx_hal.h" // HAL_GetUID()
#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_utils.h" // LL_GetFlashSize()
#include <stdio.h>
#include <stdlib.h>
#include <stm32f1xx.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

#include "MMM_can.h"
#include "main.h"
#include "rtos_code.h"
#include "system.h"

EventGroupHandle_t gEventGroup;

static void vMainTask(void *pvParameters) {
  xFreeRTOSActive = pdTRUE; // if the boot takes too long, the SysTick ISR will trigger a crash in FreeRTOS, if it is not already initialized.
  while (1) {
    EventBits_t uxBits = xEventGroupWaitBits(gEventGroup, (eMainEvents_buttonLeft | eMainEvents_buttonRight | eMainEvents_anyButtonPressed) | eMainEvents_LoxCanMessageReceived | eMainEvents_1sTimer, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & eMainEvents_anyButtonPressed) {
      uint8_t keyBitmask = uxBits & (eMainEvents_buttonLeft | eMainEvents_buttonRight);
      if (keyBitmask == 3) {
        const LoxCanMessage msg = {
          .identifier = 0x106ff0fd,
          .data = {0xff, 0x01, 0x00, 0x00, 0x6c, 0x10, 0x10, 0x13},
        };
        MMM_CAN_Send(&msg);
      }
    }
    if (uxBits & eMainEvents_LoxCanMessageReceived) {
      LoxCanMessage msg;
      while (xQueueReceive(&gCanReceiveQueue, &msg, 0)) {
        printf("%08x %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n", msg.identifier, msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
      }
    }
    if (uxBits & eMainEvents_1sTimer) {
      printf("Ticks %d\n", HAL_GetTick());
    }
  }
}

static void vLEDTask(void *pvParameters) {
  while (1) {
    EventBits_t uxBits = xEventGroupWaitBits(gEventGroup, (eMainEvents_buttonLeft | eMainEvents_buttonRight | eMainEvents_anyButtonPressed) | eMainEvents_1sTimer, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & eMainEvents_anyButtonPressed) {
      uint8_t keyBitmask = uxBits & (eMainEvents_buttonLeft | eMainEvents_buttonRight);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, !((keyBitmask & 1) == 1)); // 0=LED on, 1=LED off
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, !((keyBitmask & 2) == 2)); // 0=LED on, 1=LED off
    }
    if (uxBits & eMainEvents_1sTimer) {
      //printf("Ticks %d (LED)\n", HAL_GetTick());
    }
  }
}

static void vKeyInputTask(void *pvParameters) {
  const TickType_t xDelay50ms = pdMS_TO_TICKS(50);
  eMainEvents lastMainEventMask = 0;
  while (1) {
    eMainEvents eventMask = eMainEvents_anyButtonPressed;
    if (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1)) // 0=Button pressed, 1=Button released
      eventMask |= eMainEvents_buttonLeft;
    if (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15))
      eventMask |= eMainEvents_buttonRight;
    if (eventMask != lastMainEventMask) {
      lastMainEventMask = eventMask;
      xEventGroupSetBits(gEventGroup, eventMask);
    }
    vTaskDelay(xDelay50ms);
  }
}

/**
* @brief ADC MSP Initialization
* This function configures the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    /* Peripheral clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();
  }
}

/**
* @brief ADC MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static float MX_read_temperature(void) {
  static ADC_HandleTypeDef sADC1;

  /** Common config */
  sADC1.Instance = ADC1;
  sADC1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  sADC1.Init.ContinuousConvMode = DISABLE;
  sADC1.Init.DiscontinuousConvMode = DISABLE;
  sADC1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  sADC1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  sADC1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&sADC1) != HAL_OK) {
    /* Loop forever */
    for (;;)
      ;
  }
  /** Configure Regular Channel */
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&sADC1, &sConfig) != HAL_OK) {
    /* Loop forever */
    for (;;)
      ;
  }

  while (HAL_ADCEx_Calibration_Start(&sADC1) != HAL_OK)
    ;
  HAL_ADC_Start(&sADC1);
  while (HAL_ADC_PollForConversion(&sADC1, 0) != HAL_OK)
    ;
  uint16_t adcValue = HAL_ADC_GetValue(&sADC1);
  const float AVG_SLOPE = 4.3E-03;
  const float V25 = 1.43;
  const float ADC_TO_VOLT = 3.3 / 4096;
  float temp = (V25 - adcValue * ADC_TO_VOLT) / AVG_SLOPE + 25.0f;
  HAL_ADC_Stop(&sADC1);
  return temp;
}

int main(void) {
  HAL_Init();
  SystemClock_Config();

  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("SysClock:%dMHz HCLK:%dMHz PCLK1:%dMHz PCLK2:%dMHz FLASH:%dkb, Unique device ID:%08x.%08x.%08x\n", HAL_RCC_GetSysClockFreq() / 1000000, HAL_RCC_GetHCLKFreq() / 1000000, HAL_RCC_GetPCLK1Freq() / 1000000, HAL_RCC_GetPCLK2Freq() / 1000000, LL_GetFlashSize(), uid[0], uid[1], uid[2]);
  printf("ADC_TEMPSENSOR: %.2fC\n", MX_read_temperature());

  MMM_CAN_Init();
  //MMM_CAN_FilterLoxNAT(0, 0x10, 0xFF, 1, CAN_FILTER_FIFO0);
  MMM_CAN_FilterAllowAll(0);

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_Init;
  // 2 LEDs as outputs
  GPIO_Init.Pin = GPIO_PIN_13 | GPIO_PIN_14;
  GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_Init.Pull = GPIO_NOPULL;
  GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_Init);

  // 2 buttons as inputs
  GPIO_Init.Pin = GPIO_PIN_1 | GPIO_PIN_15;
  GPIO_Init.Mode = GPIO_MODE_INPUT;
  GPIO_Init.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_Init);

  static StaticEventGroup_t sEventGroup;
  gEventGroup = xEventGroupCreateStatic(&sEventGroup);

  static StackType_t sMainTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sMainTask;
  xTaskCreateStatic(vMainTask, "MainTask", configMINIMAL_STACK_SIZE, NULL, 2, sMainTaskStack, &sMainTask);

  static StackType_t sLEDTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sLEDTask;
  xTaskCreateStatic(vLEDTask, "LEDTask", configMINIMAL_STACK_SIZE, NULL, 1, sLEDTaskStack, &sLEDTask);

  static StackType_t sKeyInputTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sKeyInputTaskTask;
  xTaskCreateStatic(vKeyInputTask, "KeyInputTask", configMINIMAL_STACK_SIZE, NULL, 1, sKeyInputTaskStack, &sKeyInputTaskTask);

  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}