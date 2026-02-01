/*
 * timers.c
 *
 *  Created on: Jan 28, 2026
 *      Author: donav
 */

#include "timers.h"

void timerInit(uint8_t LOWEST_ENERGY_MODE){
  timer_clockInit(LOWEST_ENERGY_MODE);

  uint32_t freq = CMU_ClockFreqGet(cmuClock_LETIMER0);

  /*
  if(LOWEST_ENERGY_MODE == 3){
        //The tolerance on this ULFRCO is 5-7%, this value is empirically determined to make it more accurate
        freq = 1017;
    }
  */

  //shifting left 10 bits to increase precision in division
  uint32_t period_counts = LETIMER_PERIOD_MS * freq / 1000;
  uint32_t on_counts = LETIMER_ON_TIME_MS * freq / 1000;

  //route PWM out (Does not work for some reason)
  //LETIMER0->ROUTELOC0 = LETIMER_ROUTELOC0_OUT0LOC_LOC28 | LETIMER_ROUTELOC0_OUT1LOC_LOC28;
  //LETIMER0->ROUTEPEN = LETIMER_ROUTEPEN_OUT0PEN | LETIMER_ROUTEPEN_OUT1PEN;

  const LETIMER_Init_TypeDef LETIMER0_Init = {
      .enable       = true,
      .debugRun     = true,
      .comp0Top     = true,
      .bufTop       = 0,
      .out0Pol      = 0,
      .out1Pol      = 0,
      .ufoa0        = _LETIMER_CTRL_UFOA0_NONE,
      .ufoa1        = _LETIMER_CTRL_UFOA1_NONE,
      .repMode      = letimerRepeatFree,
      .topValue     = period_counts
  };

  LETIMER_CompareSet(LETIMER0,  1, on_counts);
  LETIMER_Init(LETIMER0, &LETIMER0_Init);

  //enable interrupt
  LETIMER_IntEnable(LETIMER0, LETIMER_IF_UF | LETIMER_IF_COMP1);
  NVIC_EnableIRQ(LETIMER0_IRQn);
}

