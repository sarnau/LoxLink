#include "system.hpp"
#include "stm32f1xx_hal.h" // HAL_IncTick
#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_ll_cortex.h" // LL_CPUID_...()
#include "stm32f1xx_ll_utils.h"  // LL_GetFlashSize()

/***
 *
 ***/
#if DEBUG
void MX_print_cpu_info(void) {
  switch (LL_CPUID_GetImplementer()) {
  case 0x41:
    switch (LL_CPUID_GetParNo()) {
    case 0x0C05:
      printf("ARM Cortex-A5");
      break;
    case 0x0C07:
      printf("ARM Cortex-A7");
      break;
    case 0x0C08:
      printf("ARM Cortex-A8");
      break;
    case 0x0C09:
      printf("ARM Cortex-A9");
      break;
    case 0x0C0D:
      printf("ARM Cortex-A12");
      break;
    case 0x0C0F:
      printf("ARM Cortex-A15");
      break;
    case 0x0C14:
      printf("ARM Cortex-R4");
      break;
    case 0x0C15:
      printf("ARM Cortex-R5");
      break;
    case 0x0C17:
      printf("ARM Cortex-R7");
      break;
    case 0x0C18:
      printf("ARM Cortex-R8");
      break;
    case 0x0C20:
      printf("ARM Cortex-M0");
      break;
    case 0x0C21:
      printf("ARM Cortex-M1");
      break;
    case 0x0C23:
      printf("ARM Cortex-M3");
      break;
    case 0x0C24:
      printf("ARM Cortex-M4");
      break;
    case 0x0C27:
      printf("ARM Cortex-M7");
      break;
    case 0x0D01:
      printf("ARM Cortex-A32");
      break;
    case 0x0D03:
      printf("ARM Cortex-A53");
      break;
    case 0x0D04:
      printf("ARM Cortex-A35");
      break;
    case 0x0D05:
      printf("ARM Cortex-A55");
      break;
    case 0x0D07:
      printf("ARM Cortex-A57");
      break;
    case 0x0D08:
      printf("ARM Cortex-A72");
      break;
    case 0x0D09:
      printf("ARM Cortex-A73");
      break;
    case 0x0D0A:
      printf("ARM Cortex-A75");
      break;
    case 0x0D0B:
      printf("ARM Cortex-A76");
      break;
    case 0x0D13:
      printf("ARM Cortex-R52");
      break;
    case 0x0D20:
      printf("ARM Cortex-M23");
      break;
    case 0x0D21:
      printf("ARM Cortex-M33");
      break;
    }
    break;
  default:
    printf("Implementor:%c:0x%04x.%04x", LL_CPUID_GetImplementer(), LL_CPUID_GetConstant(), LL_CPUID_GetParNo());
    break;
  }
  printf(" r:%d p:%d\n", LL_CPUID_GetVariant(), LL_CPUID_GetRevision());
  printf("FLASH:%dkb\n", LL_GetFlashSize());
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Unique device ID:%08x.%08x.%08x\n", uid[0], uid[1], uid[2]);
  printf("SysClock:%dMHz HCLK:%dMHz PCLK1:%dMHz PCLK2:%dMHz FLASH:%dkb\n", HAL_RCC_GetSysClockFreq() / 1000000, HAL_RCC_GetHCLKFreq() / 1000000, HAL_RCC_GetPCLK1Freq() / 1000000, HAL_RCC_GetPCLK2Freq() / 1000000);
  printf("ADC_TEMPSENSOR: %.2fC\n", MX_read_temperature());
}
#endif

/**
  * Initializes the Global MSP.
  */
extern "C" void HAL_MspInit(void) {
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

/**
* @brief This function handles System tick timer.
*/
extern "C" void SysTick_Handler(void) {
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    for (;;) {
    }
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    for (;;) {
    }
  }
}

void assert_failed(uint8_t *file, uint32_t line) {
  printf("assert_failed(\"%s\", %d)\n", file, line);
  for (;;) {
  }
}

/**
* @brief ADC MSP Initialization
* This function configures the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
extern "C" void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
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
extern "C" void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc) {
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
float MX_read_temperature(void) {
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