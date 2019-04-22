#include "LED.hpp"
#include "Watchdog.hpp"
#include "stm32f1xx_hal.h"
#include "system.hpp"
#include <ctl_api.h>
#include <string.h>
#include <__cross_studio_io.h>

#include "LoxCANDriver_STM32.hpp"
#include "LoxBusTreeExtension.hpp"
//#include "LoxBusDIExtension.hpp"
//#include "LoxBusTreeAlarmSiren.hpp"
//#include "LoxBusTreeRoomComfortSensor.hpp"
//#include "LoxBusTreeTouch.hpp"
//#include "LoxLegacyDMXExtension.hpp"
//#include "LoxLegacyRS232Extension.hpp"
//#include "LoxLegacyModbusExtension.hpp"
//#include "LoxLegacyRelayExtension.hpp"
#include "LoxBusTreeRgbwDimmer.hpp"

/***
 *
 ***/
int main(void) {
  system_init();

  static CTL_TASK_t main_task;
  ctl_task_init(&main_task, 255, "main"); // create subsequent tasks whilst running at the highest priority.

  // Warning: be aware that two relay extension need two different serial numbers!
  uint32_t serial_base = serialnumber_24bit();

  static LoxCANDriver_STM32 gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
#if EXTENSION_RS232
//  static LoxLegacyRS232Extension gLoxLegacyRS232Extension(gLoxCANDriver, serial_base);
#endif
#if EXTENSION_MODBUS
//  static LoxLegacyModbusExtension gLoxLegacyModbusExtension(gLoxCANDriver, serial_base);
#endif
  //  static LoxBusDIExtension gDIExtension(gLoxCANDriver, serial_base, gResetReason);
  //  static LoxLegacyRelayExtension gRelayExtension(gLoxCANDriver, serial_base);
  //static LoxLegacyDMXExtension gDMXExtension(gLoxCANDriver, serial_base);
  static LoxBusTreeExtension gTreeExtension(gLoxCANDriver, serial_base, gResetReason);
  //  static LoxBusTreeRoomComfortSensor gTreeRoomComfortSensor(gTreeExtension.Driver(eTreeBranch_rightBranch), 0xb0112233, gResetReason);
  //  gTreeExtension.AddDevice(&gTreeRoomComfortSensor, eTreeBranch_rightBranch);
  //  static LoxBusTreeTouch gLoxBusTreeTouch(gTreeExtension.Driver(eTreeBranch_leftBranch), 0xb010035b, gResetReason);
  //  gTreeExtension.AddDevice(&gLoxBusTreeTouch, eTreeBranch_leftBranch);
  //  static LoxBusTreeAlarmSiren gLoxBusTreeAlarmSiren(gTreeExtension.Driver(eTreeBranch_leftBranch), 0xb010035c, gResetReason);
  //  gTreeExtension.AddDevice(&gLoxBusTreeAlarmSiren, eTreeBranch_leftBranch);
  static LoxBusTreeRgbwDimmer gLoxBusTreeRgbwDimmer(gTreeExtension.Driver(eTreeBranch_rightBranch), 0xb0200000, gResetReason);
  gTreeExtension.AddDevice(&gLoxBusTreeRgbwDimmer, eTreeBranch_rightBranch);

#if DEBUG
  MX_print_cpu_info();
#endif
  gLED.Startup();
  gLoxCANDriver.Startup();

  Start_Watchdog();
  ctl_task_set_priority(&main_task, 0); // drop to lowest priority to start created tasks running.
  while (1) {
  }
  return 0;
}