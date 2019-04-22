#include "Watchdog.hpp"
#include "stm32f1xx_hal_iwdg.h"
#include <ctl_api.h>
#include <string.h>

IWDG_HandleTypeDef gIWDG;

/***
 *  Trigger the watchdog every second
 ***/
void vWatchdogTask(void *pvParameters) {
  while (1) {
    HAL_IWDG_Refresh(&gIWDG);
    ctl_timeout_wait(ctl_get_current_time() + 1000);
  }
}

/***
 *  Setup the watchdog and run a task to keep it alive
 ***/
void Start_Watchdog(void) {
  /* 
        Watchdog freq. is 40 kHz
        Prescaler: Min_Value = 4 and Max_Value = 256
        Reload: Min_Data = 0 and Max_Data = 0x0FFF
        TimeOut in seconds = (Reload * Prescaler) / Freq.
        MinTimeOut = (4 * 1) / 40000 = 0.0001 seconds (100 microseconds)
        MaxTimeOut = (256 * 4096) / 40000 = 26.2 seconds
    */
  gIWDG.Instance = IWDG;
  gIWDG.Init.Prescaler = IWDG_PRESCALER_256;
  gIWDG.Init.Reload = 1562; // about 10s
  HAL_IWDG_Init(&gIWDG);

  // Run this task at almost the lowest priority (1)
  #define STACKSIZE 64          
  static unsigned stack[1+STACKSIZE+1];
  memset(stack, 0xcd, sizeof(stack));  // write known values into the stack
  stack[0]=stack[1+STACKSIZE]=0xfacefeed; // put marker values at the words before/after the stack
  static CTL_TASK_t watchdog;
  ctl_task_run(&watchdog, 1, vWatchdogTask, 0, "Watchdog", STACKSIZE, stack+1, 0);
  
  __HAL_IWDG_START(&gIWDG);
}