/*
 * timers.h
 *
 *  Created on: Jan 28, 2026
 *      Author: donav
 */

#ifndef SRC_TIMERS_H_
#define SRC_TIMERS_H_

#include "em_letimer.h"

#include "oscillators.h"
#include "irq.h"

#define LETIMER_ON_TIME_MS 800
#define LETIMER_PERIOD_MS 3000

#if LETIMER_ON_TIME_MS > LETIMER_PERIOD_MS
#error "On time must be less than period"
#endif

void timerInit(uint8_t LowestEnergyMode);

#endif /* SRC_TIMERS_H_ */
