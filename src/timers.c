/*
 * timers.c
 *
 *  Created on: Jan 28, 2026
 *      Author: donav
 */

#include "timers.h"

static int32_t period_counts;
static int32_t freq;

static uint64_t Timer_getTime();

static volatile uint64_t underflowCounter;
void Timer_incrementUnderflowCounter(){
  underflowCounter += period_counts;
}

static uint64_t Timer_getTime(){
  uint64_t time;
  CORE_ATOMIC_SECTION
  (
      time = underflowCounter + period_counts - LETIMER_CounterGet(LETIMER0);
  )

  return time;
}

void timerInit(uint8_t LOWEST_ENERGY_MODE){
  timer_clockInit(LOWEST_ENERGY_MODE);

  freq = CMU_ClockFreqGet(cmuClock_LETIMER0);

  /*
  if(LOWEST_ENERGY_MODE == 3){
        //The tolerance on this ULFRCO is 5-7%, this value is empirically determined to make it more accurate
        freq = 1017;
    }
  */

  //shifting left 10 bits to increase precision in division
  period_counts = LETIMER_PERIOD_MS * freq / 1000;
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
  LETIMER_IntEnable(LETIMER0, LETIMER_IF_UF);
  NVIC_EnableIRQ(LETIMER0_IRQn);
}

//The fundamental time unit is LETIMER0 timer ticks. At max frequency (~32kHz), the total counter still wouldn't overflow for ~17 million years. The 64 bit accesses are made atomic with a critical section.
//The underflow increment operation is handled in the ISR handler to prevent race conditions involving the timer continuing to run without the underflow counted. This does allow for a practically infinite delay time.
uint8_t timerWaitUs(uint32_t delay_usec){
  uint64_t currentTime = Timer_getTime();

  //Range Checking
  //Any value of delay_usec is valid since the function does not have any valid range. As such, returning -1 (error) never happens.
  if(delay_usec == 0){
      return 1;
  }

  uint32_t delayTime = (((uint64_t)delay_usec * freq) / 1000000) + 1;
  uint64_t targetTime = currentTime + delayTime;

  uint64_t countTime = Timer_getTime();
  while(countTime < targetTime){
      countTime = Timer_getTime();
  }

  return 1;
}
