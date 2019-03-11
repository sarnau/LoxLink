#include "LED.hpp"
#include "LoxBusDIExtension.hpp"
#include "LoxLegacyRelayExtension.hpp"
#include "Watchdog.hpp"
#include "stm32f1xx_hal.h"
#include "system.hpp"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "stm32f1xx_ll_cortex.h" // LL_CPUID_...()

int main(void) {
  HAL_Init();
  SystemClock_Config();
  static eAliveReason_t sResetReason;
  sResetReason = MX_reset_reason();

  uint32_t uid[3];
  HAL_GetUID(uid);

  static LoxCANDriver gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
  static LoxBusDIExtension gDIExtension(gLoxCANDriver, (uid[0] ^ uid[1] ^ uid[2]) & 0xFFFFFF, sResetReason);
  static LoxLegacyRelayExtension gRelayExtension(gLoxCANDriver, (uid[0] ^ uid[1] ^ uid[2]) & 0xFFFFFF);

#if DEBUG && 0
  MX_print_cpu_info();
#endif
  gLED.Startup();
  gLoxCANDriver.Startup();
  gRelayExtension.Startup();
  gDIExtension.Startup();

  Start_Watchdog();
  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}