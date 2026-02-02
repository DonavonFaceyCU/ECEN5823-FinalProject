/*
 * scheduler.c
 *
 *  Created on: Feb 1, 2026
 *      Author: donav
 */

#include "scheduler.h"

//Private functions for scheduling
static uint32_t Scheduler_Active(uint32_t mask);
static void Scheduler_Set(uint32_t mask);
static void Scheduler_Clear(uint32_t mask);

static uint32_t ScheduledEvents;

static uint32_t Scheduler_Active(uint32_t mask){
  return ScheduledEvents & mask;
}

static void Scheduler_Set(uint32_t mask){
  CORE_ATOMIC_SECTION
  (
      ScheduledEvents |= mask;
  )
}

static void Scheduler_Clear(uint32_t mask){
  CORE_ATOMIC_SECTION
  (
      ScheduledEvents &= ~mask;
  )
}


///////////////////////////// Event Specific Wrappers are Below ///////////////////////////////////

#define EVENT_UF (0x1 << 0)
uint32_t Scheduler_Active_UF()  { return Scheduler_Active(EVENT_UF);  }
void Scheduler_Set_UF()         { Scheduler_Set(EVENT_UF);            }
void Scheduler_Clear_UF()       { Scheduler_Clear(EVENT_UF);          }
