#include "Watchdog.hpp"
#include "FreeRTOS.h"
#include "stm32f1xx_hal_iwdg.h"
#include "task.h"

IWDG_HandleTypeDef gIWDG;

/***
 *  Trigger the watchdog every second
 ***/
void vWatchdogTask(void *pvParameters) {
  while (1) {
    HAL_IWDG_Refresh(&gIWDG);
    vTaskDelay(pdMS_TO_TICKS(1000)); // 1s delay
  }
}

/***
 *
 ***/
void Start_Watchdog(void) {
  /* 
        Watchdog freq. is 32 kHz
        Prescaler: Min_Value = 4 and Max_Value = 256
        Reload: Min_Data = 0 and Max_Data = 0x0FFF
        TimeOut in seconds = (Reload * Prescaler) / Freq.
        MinTimeOut = (4 * 1) / 32000 = 0.000125 seconds (125 microseconds)
        MaxTimeOut = (256 * 4096) / 32000 = 32.768 seconds
    */
  gIWDG.Instance = IWDG;
  gIWDG.Init.Prescaler = IWDG_PRESCALER_256;
  gIWDG.Init.Reload = 1500; // about 10s
  HAL_IWDG_Init(&gIWDG);

  static StackType_t sWatchdogTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sWatchdogTask;
  xTaskCreateStatic(vWatchdogTask, "WatchdogTask", configMINIMAL_STACK_SIZE, NULL, 2, sWatchdogTaskStack, &sWatchdogTask);

  __HAL_IWDG_START(&gIWDG);
}