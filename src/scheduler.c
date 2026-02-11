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

#define EVENT_COMP1 (0x1 << 1)
uint32_t Scheduler_Active_COMP1()  { return Scheduler_Active(EVENT_COMP1);  }
void Scheduler_Set_COMP1()         { Scheduler_Set(EVENT_COMP1);            }
void Scheduler_Clear_COMP1()       { Scheduler_Clear(EVENT_COMP1);          }

#define EVENT_TXC (0x1 << 2)
uint32_t Scheduler_Active_TXC()  { return Scheduler_Active(EVENT_TXC);  }
void Scheduler_Set_TXC()         { Scheduler_Set(EVENT_TXC);            }
void Scheduler_Clear_TXC()       { Scheduler_Clear(EVENT_TXC);          }
