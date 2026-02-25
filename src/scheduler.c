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
  S2_COMMAND,
  S3_MEASUREMENT,
  S4_PUBLISH,
  I2C_TRANSFER_NUM_STATES
} i2c_transfer_state_t;

void i2c_stateMachine(sl_bt_msg_t *evt){
  static i2c_transfer_state_t current_state;

  //ignore all events aside from external signals
  if(SL_BT_MSG_ID(evt->header) != sl_bt_evt_system_external_signal_id){
      return;
  }

    i2c_transfer_state_t next_state = current_state;

    switch(current_state){
      case S0_IDLE:
        //if UF event is triggered, turn on sensor, wait 82ms
        if(Scheduler_Active_UF(evt) && connection_established() && indication_allowed()){
          next_state = S1_STARTUP;

          i2cEnableSensor();
          timerWaitUs(80000);
        }
        break;
      case S1_STARTUP:
        //if COMP1 is triggered after 80ms, command a temperature reading to happen
        if(Scheduler_Active_COMP1(evt)){
          next_state = S2_COMMAND;

          i2cCommandTemperatureReading();
          NVIC_EnableIRQ(I2C0_IRQn);
        }
        break;
      case S2_COMMAND:
        //if TXC is triggered, wait 11ms for temperature to be read
        if(Scheduler_Active_TXC()){
          next_state = S3_MEASUREMENT;

          NVIC_DisableIRQ(I2C0_IRQn);
          timerWaitUs(10800);
        }
        break;
      case S3_MEASUREMENT:
        //if COMP1 is triggered, start temperature reading
        if(Scheduler_Active_COMP1(evt)){
          next_state = S4_PUBLISH;

          i2cReadTemperature_nonblocking_start();
          NVIC_EnableIRQ(I2C0_IRQn);
        }
        break;
      case S4_PUBLISH:
        //if TXC is triggered, shutdown sensor, go back to idle
        if(Scheduler_Active_TXC(evt)){
          next_state = S0_IDLE;

          NVIC_DisableIRQ(I2C0_IRQn);
          uint32_t temp_11073 = i2cReadTemperature_nonblocking_finish();

          uint8_t htmTempBuffer[5];

          uint8_t *p = htmTempBuffer;

          uint8_t flags = 0x00;
          UINT8_TO_BITSTREAM(p, flags);
          UINT32_TO_BITSTREAM(p, temp_11073);

          update_temperature_reading(5, htmTempBuffer);
          if(indication_allowed()){
              send_temperature_reading(5, htmTempBuffer);
          }

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

static uint32_t Scheduler_Active(sl_bt_msg_t *evt, uint32_t mask);
static void Scheduler_Set(uint32_t mask);

static uint32_t Scheduler_Active(sl_bt_msg_t *evt, uint32_t mask){
  return evt->data.evt_system_external_signal.extsignals & mask;
}

static void Scheduler_Set(uint32_t mask){
  CORE_ATOMIC_SECTION
  (
      sl_bt_external_signal(mask);
  )
}

///////////////////////////// Event Specific Wrappers are Below ///////////////////////////////////

#define EVENT_UF (0x1 << 0)
uint32_t Scheduler_Active_UF(sl_bt_msg_t *evt)      { return Scheduler_Active(evt, EVENT_UF);     }
void Scheduler_Set_UF()                             { Scheduler_Set(EVENT_UF);                    }

#define EVENT_COMP1 (0x1 << 1)
uint32_t Scheduler_Active_COMP1(sl_bt_msg_t *evt)   { return Scheduler_Active(evt, EVENT_COMP1);  }
void Scheduler_Set_COMP1()                          { Scheduler_Set(EVENT_COMP1);                 }

#define EVENT_TXC (0x1 << 2)
uint32_t Scheduler_Active_TXC(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_TXC);    }
void Scheduler_Set_TXC()                            { Scheduler_Set(EVENT_TXC);                   }
