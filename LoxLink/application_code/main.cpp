#include "LED.hpp"
#include "Watchdog.hpp"
#include "stm32f1xx_hal.h"
#include "system.hpp"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "stm32f1xx_ll_cortex.h" // LL_CPUID_...()

#include "LoxBusDIExtension.hpp"
#include "LoxBusTreeAlarmSiren.hpp"
#include "LoxBusTreeExtension.hpp"
#include "LoxBusTreeRoomComfortSensor.hpp"
#include "LoxBusTreeTouch.hpp"
#include "LoxCANDriver_STM32.hpp"
#include "LoxLegacyDMXExtension.hpp"
#include "LoxLegacyRS232Extension.hpp"
#include "LoxLegacyModbusExtension.hpp"
#include "LoxLegacyRelayExtension.hpp"
#include "LoxBusTreeRgbwDimmer.hpp"
#include <string.h>

/***
 *
 ***/
int main(void) {
  HAL_Init();
  SystemClock_Config();
  static eAliveReason_t sResetReason;
  sResetReason = MX_reset_reason();

  // generate a base serial from the STM32 UID
  // Warning: be aware that two relay extension need two different serial numbers!
  uint32_t uid[3];
  HAL_GetUID(uid);
  uint32_t serial_base = (uid[0] ^ uid[1] ^ uid[2]) & 0xFFFFFF;

  static LoxCANDriver_STM32 gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
#if EXTENSION_RS232
//  static LoxLegacyRS232Extension gLoxLegacyRS232Extension(gLoxCANDriver, serial_base);
#endif
#if EXTENSION_MODBUS
//  static LoxLegacyModbusExtension gLoxLegacyModbusExtension(gLoxCANDriver, serial_base);
#endif
//  static LoxBusDIExtension gDIExtension(gLoxCANDriver, serial_base, sResetReason);
//  static LoxLegacyRelayExtension gRelayExtension(gLoxCANDriver, serial_base);
  //static LoxLegacyDMXExtension gDMXExtension(gLoxCANDriver, serial_base);
  static LoxBusTreeExtension gTreeExtension(gLoxCANDriver, serial_base, sResetReason);
//  static LoxBusTreeRoomComfortSensor gTreeRoomComfortSensor(gTreeExtension.Driver(eTreeBranch_rightBranch), 0xb0112233, sResetReason);
//  gTreeExtension.AddDevice(&gTreeRoomComfortSensor, eTreeBranch_rightBranch);
//  static LoxBusTreeTouch gLoxBusTreeTouch(gTreeExtension.Driver(eTreeBranch_leftBranch), 0xb010035b, sResetReason);
//  gTreeExtension.AddDevice(&gLoxBusTreeTouch, eTreeBranch_leftBranch);
//  static LoxBusTreeAlarmSiren gLoxBusTreeAlarmSiren(gTreeExtension.Driver(eTreeBranch_leftBranch), 0xb010035c, sResetReason);
//  gTreeExtension.AddDevice(&gLoxBusTreeAlarmSiren, eTreeBranch_leftBranch);
  static LoxBusTreeRgbwDimmer gLoxBusTreeRgbwDimmer(gTreeExtension.Driver(eTreeBranch_rightBranch), 0xb0200000, sResetReason);
  gTreeExtension.AddDevice(&gLoxBusTreeRgbwDimmer, eTreeBranch_rightBranch);

#if DEBUG
  MX_print_cpu_info();
#endif
  gLED.Startup();
  gLoxCANDriver.Startup();

  Start_Watchdog();

  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}
