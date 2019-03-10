#include "LED.hpp"
#include "LoxBusDIExtension.hpp"
#include "Watchdog.hpp"
#include "stm32f1xx_hal.h"
#include "system.hpp"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "stm32f1xx_ll_cortex.h" // LL_CPUID_...()

#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_hal_pwr.h"

static eAliveReason_t GetResetReason() {
  eAliveReason_t reason = eAliveReason_t_unknown;
  if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB))
    reason = eAliveReason_t_standby_reset;
  else if (LL_RCC_IsActiveFlag_IWDGRST())
    reason = eAliveReason_t_watchdog_reset;
  else if (LL_RCC_IsActiveFlag_LPWRRST())
    reason = eAliveReason_t_low_power_reset;
  else if (LL_RCC_IsActiveFlag_PINRST())
    reason = eAliveReason_t_pin_reset;
  else if (LL_RCC_IsActiveFlag_PORRST())
    reason = eAliveReason_t_power_on_reset;
  else if (LL_RCC_IsActiveFlag_SFTRST())
    reason = eAliveReason_t_software_reset;
  else if (LL_RCC_IsActiveFlag_WWDGRST())
    reason = eAliveReason_t_window_watchdog_reset;
  LL_RCC_ClearResetFlags();
  return reason;
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  static eAliveReason_t sResetReason;
  sResetReason = GetResetReason();

  uint32_t uid[3];
  HAL_GetUID(uid);

  static LoxCANDriver gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
  static LoxBusDIExtension gDIExtension(gLoxCANDriver, (uid[0] ^ uid[1] ^ uid[2]) & 0xFFFFFF, sResetReason);

#if DEBUG && 0
  MX_print_cpu_info();
#endif
  gLED.Startup();
  gLoxCANDriver.Startup();
  gDIExtension.Startup();

  Start_Watchdog();
  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}