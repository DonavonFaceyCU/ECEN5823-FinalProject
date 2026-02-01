/*
 * oscillators.c
 *
 *  Created on: Jan 28, 2026
 *      Author: donav
 */

#include "oscillators.h"

void timer_clockInit(uint8_t LowestEnergyMode){
  if(LowestEnergyMode == 3){
      CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);
      CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
  } else {
      CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
      CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
      CMU_ClockPrescSet(cmuClock_LETIMER0, 1);
  }
  CMU_ClockEnable(cmuClock_LETIMER0, true);
}

