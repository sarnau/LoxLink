#include "LED.hpp"
#include "LoxBusDIExtension.hpp"
#include "stm32f1xx_hal.h"
#include "system.hpp"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "stm32f1xx_ll_cortex.h" // LL_CPUID_...()

int main(void) {
  HAL_Init();
  SystemClock_Config();

  uint32_t uid[3];
  HAL_GetUID(uid);

  static LoxCANDriver gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
  static LoxBusDIExtension gDIExtension(gLoxCANDriver, (uid[0] ^ uid[1] ^ uid[2]) & 0xFFFFFF);

  //  MX_print_cpu_info();
  gLED.Startup();
  gLoxCANDriver.Startup();
  gDIExtension.Startup();

  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}