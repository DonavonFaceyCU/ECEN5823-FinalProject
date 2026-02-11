/*
 * irq.h
 *
 *  Created on: Jan 28, 2026
 *      Author: donav
 */

#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_

#include "efr32bg13p632f512gm48.h"
#include "core_cm4.h"

#include "gpio.h"
#include "timers.h"
#include "i2c.h"
#include "scheduler.h"

void Time_Init(uint32_t timerCounts, uint32_t frequency);

uint64_t Time_getTime();
uint64_t letimerMilliseconds();
uint8_t timerWaitUs(uint32_t delay_usec);
uint8_t timerWaitUs_polled(uint32_t delay_usec);

#endif /* SRC_IRQ_H_ */
