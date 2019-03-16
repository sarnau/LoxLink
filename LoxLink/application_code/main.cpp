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
#include "LoxLegacyRelayExtension.hpp"
#include "LoxLegacyRS232Extension.hpp"
#include "SEGGER_SYSVIEW.h"

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

#if defined(USE_SYSVIEW)
  SEGGER_SYSVIEW_Conf();
  SEGGER_SYSVIEW_Start();
#endif

  static LoxCANDriver_STM32 gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
  //static LoxLegacyRS232Extension gLoxLegacyRS232Extension(gLoxCANDriver, serial_base);
  //static LoxBusDIExtension gDIExtension(gLoxCANDriver, serial_base, sResetReason);
  //static LoxLegacyRelayExtension gRelayExtension(gLoxCANDriver, serial_base);
  //static LoxBusTreeExtension gTreeExtension(gLoxCANDriver, serial_base, sResetReason);
  //static LoxBusTreeRoomComfortSensor gTreeRoomComfortSensor(gLoxCANDriver, 0xb0112233, sResetReason);
  //static LoxBusTreeTouch gLoxBusTreeTouch(gLoxCANDriver, 0xb010035b, sResetReason);
  //static LoxBusTreeAlarmSiren gLoxBusTreeAlarmSiren(gLoxCANDriver, 0xb010035c, sResetReason);

#if DEBUG && 0
  MX_print_cpu_info();
#endif
  gLED.Startup();
  gLoxCANDriver.Startup();

  Start_Watchdog();

  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}


#if defined(USE_SYSVIEW)

/*
File    : SEGGER_SYSVIEW_Config_NoOS.c
Purpose : Sample setup configuration of SystemView without an OS.
Revision: $Rev: 12706 $
*/
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_Conf.h"

// SystemcoreClock can be used in most CMSIS compatible projects.
// In non-CMSIS projects define SYSVIEW_CPU_FREQ.
extern unsigned int SystemCoreClock;

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
// The application name to be displayed in SystemViewer
#define SYSVIEW_APP_NAME        "LoxLink"

// The target device name
#define SYSVIEW_DEVICE_NAME     "Cortex-M3"

// Frequency of the timestamp. Must match SEGGER_SYSVIEW_Conf.h
#define SYSVIEW_TIMESTAMP_FREQ  (SystemCoreClock)

// System Frequency. SystemcoreClock is used in most CMSIS compatible projects.
#define SYSVIEW_CPU_FREQ        (SystemCoreClock)

// The lowest RAM address used for IDs (pointers)
#define SYSVIEW_RAM_BASE        (0x20000000)

// Define as 1 if the Cortex-M cycle counter is used as SystemView timestamp. Must match SEGGER_SYSVIEW_Conf.h
#ifndef   USE_CYCCNT_TIMESTAMP
  #define USE_CYCCNT_TIMESTAMP    1
#endif

// Define as 1 if the Cortex-M cycle counter is used and there might be no debugger attached while recording.
#ifndef   ENABLE_DWT_CYCCNT
  #define ENABLE_DWT_CYCCNT       (USE_CYCCNT_TIMESTAMP & SEGGER_SYSVIEW_POST_MORTEM_MODE)
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
#define DEMCR                     (*(volatile unsigned long*) (0xE000EDFCuL))   // Debug Exception and Monitor Control Register
#define TRACEENA_BIT              (1uL << 24)                                   // Trace enable bit
#define DWT_CTRL                  (*(volatile unsigned long*) (0xE0001000uL))   // DWT Control Register
#define NOCYCCNT_BIT              (1uL << 25)                                   // Cycle counter support bit
#define CYCCNTENA_BIT             (1uL << 0)                                    // Cycle counter enable bit

/********************************************************************* 
*
*       _cbSendSystemDesc()
*
*  Function description
*    Sends SystemView description strings.
*/
static void _cbSendSystemDesc(void) {
  SEGGER_SYSVIEW_SendSysDesc("N="SYSVIEW_APP_NAME",D="SYSVIEW_DEVICE_NAME);
  SEGGER_SYSVIEW_SendSysDesc("I#15=SysTick");
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
void SEGGER_SYSVIEW_Conf(void) {
#if USE_CYCCNT_TIMESTAMP
#if ENABLE_DWT_CYCCNT
  //
  // If no debugger is connected, the DWT must be enabled by the application
  //
  if ((DEMCR & TRACEENA_BIT) == 0) {
    DEMCR |= TRACEENA_BIT;
  }
#endif
  //
  //  The cycle counter must be activated in order
  //  to use time related functions.
  //
  if ((DWT_CTRL & NOCYCCNT_BIT) == 0) {       // Cycle counter supported?
    if ((DWT_CTRL & CYCCNTENA_BIT) == 0) {    // Cycle counter not enabled?
      DWT_CTRL |= CYCCNTENA_BIT;              // Enable Cycle counter
    }
  }
#endif
  SEGGER_SYSVIEW_Init(SYSVIEW_TIMESTAMP_FREQ, SYSVIEW_CPU_FREQ, 
                      0, _cbSendSystemDesc);
  SEGGER_SYSVIEW_SetRAMBase(SYSVIEW_RAM_BASE);
}

#endif
