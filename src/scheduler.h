/*
 * scheduler.h
 *
 *  Created on: Feb 1, 2026
 *      Author: donav
 */

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include <stdint.h>

#include "em_core.h"

//Functions related to LETIMER0 UF Event
uint32_t Scheduler_Active_UF();
void Scheduler_Set_UF();
void Scheduler_Clear_UF();

uint32_t Scheduler_Active_COMP1();
void Scheduler_Set_COMP1();
void Scheduler_Clear_COMP1();

uint32_t Scheduler_Active_TXC();
void Scheduler_Set_TXC();
void Scheduler_Clear_TXC();


#endif /* SRC_SCHEDULER_H_ */
