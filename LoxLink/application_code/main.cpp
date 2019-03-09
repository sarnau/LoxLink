#include "LED.hpp"
#include "LoxBusDIExtension.hpp"
#include "stm32f1xx_hal.h"
#include "system.hpp"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

static LoxCANDriver gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
static LoxBusDIExtension gDIExtension(gLoxCANDriver, 0x123456);

int main(void) {
  HAL_Init();
  SystemClock_Config();

  //  MX_print_cpu_info();
  gLED.Startup();
  gLoxCANDriver.Startup();
  gDIExtension.Startup();

  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}