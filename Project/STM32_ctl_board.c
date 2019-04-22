// Copyright (c) 2011-2013 Rowley Associates Limited.
//
// This file may be distributed under the terms of the License Agreement
// provided with this software.
//
// THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

#include <ctl_api.h>

void SystemInit (void) __attribute__ ((section (".init")));

#if defined(STM32F030) || defined(STM32F031) || defined(STM32F042)|| defined(STM32F051) || defined(STM32F072)
#include "system_stm32f0xx.c"
#elif defined(STM32F10X_LD) || defined(STM32F10X_MD) || defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL) || defined(STM32F10X_LD_VL) || defined(STM32F10X_MD_VL) || defined(STM32F10X_HD_VL)
#include "system_stm32f10x.c"
#elif defined(STM32F2XX)
#include "system_stm32f2xx.c"
#elif defined(STM32F30X)
#include "system_stm32f30x.c"
#elif defined(STM32F37X)
#include "system_stm32f37x.c"
#elif defined(STM32F40_41xxx) || defined(STM32F401xx) || defined(STM32F427_437xx) || defined(STM32F429_439xx)
#include "system_stm32f4xx.c"
#elif defined(STM32L1XX_MD) || defined(STM32L1XX_MDP) || defined(STM32L1XX_HD)
#include "system_stm32l1xx.c"
#elif defined (STM32W108C8) || defined (STM32W108CB) || defined (STM32W108CC) || defined (STM32W108CZ) || defined (STM32W108HB)
#include "system_stm32w108xx.c"
#else
#error no target
#endif

void
delay(int count)
{
  volatile int i=0;
  while (i++<count);
}

void
ctl_board_init(void)
{
  //...
}

void 
ctl_board_set_leds(unsigned on)
{
  //...
}

static CTL_ISR_FN_t userButtonISR;

void 
ctl_board_on_button_pressed(CTL_ISR_FN_t isr)
{
  userButtonISR = isr;
  // ...
}
