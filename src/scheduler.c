/*
 * scheduler.c
 *
 *  Created on: Feb 1, 2026
 *      Author: donav
 */

#include "scheduler.h"

///////////////////////////// State Machines are Below ///////////////////////////////////

typedef enum i2c_transfer_state_t {
  S0_IDLE = 0,
  S1_STARTUP,
  S2_MEASUREMENT,
  I2C_TRANSFER_NUM_STATES
} i2c_transfer_state_t;

void i2c_stateMachine(){
  static i2c_transfer_state_t current_state;

    i2c_transfer_state_t next_state = current_state;

    switch(current_state){
      case S0_IDLE:
        //if UF event is triggered, turn on sensor, wait 80ms
        if(Scheduler_Active_UF()){
          Scheduler_Clear_UF();
          next_state = S1_STARTUP;

          i2cEnableSensor();
          timerWaitUs(82000);
        }
        break;
      case S1_STARTUP:
        //if COMP1 is triggered after 80ms, command a temperature reading to happen. Sleep until clock stretched transaction is done
        if(Scheduler_Active_COMP1()){
          Scheduler_Clear_COMP1();
          next_state = S2_MEASUREMENT;

          i2cReadTemperature_nonblocking_start();
          NVIC_EnableIRQ(I2C0_IRQn);
        }
        break;
      case S2_MEASUREMENT:
        //if I2C_TransferComplete is triggered after 10.8ms, read the temperature. Then shut down sensor.
        if(Scheduler_Active_TXC()){
          Scheduler_Clear_TXC();
          next_state = S0_IDLE;
          NVIC_DisableIRQ(I2C0_IRQn);
          i2cReadTemperature_nonblocking_finish();
          i2cDisableSensor();
        }
        break;
      default:
        next_state = S0_IDLE;
        break;
    }

    current_state = next_state;
}

///////////////////////////// Event Generic Wrappers are Below ///////////////////////////////////

static uint32_t ScheduledEvents;

static uint32_t Scheduler_Active(uint32_t mask);
static void Scheduler_Set(uint32_t mask);
static void Scheduler_Clear(uint32_t mask);

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
