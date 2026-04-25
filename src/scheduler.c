/*
 * scheduler.c
 *
 *  Created on: Feb 1, 2026
 *      Author: donav
 */

#include "scheduler.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#define SOFT_TIMER_HANDLE_SENSOR   (0xD4)
#define SOFT_TIMER_PERIOD   (32768 / 5) // 5 Hz Timer Frequency
#define SOFT_TIMER_SLACK    (20)

void sensor_init(){

  i2c_sensor_init();
  sl_status_t          timer_response;
  timer_response = sl_bt_system_set_lazy_soft_timer(SOFT_TIMER_PERIOD, SOFT_TIMER_SLACK, SOFT_TIMER_HANDLE_SENSOR, false);
  if (timer_response != SL_STATUS_OK) {
    //LOG_ERROR("SOFT_TIMER_INIT_SENSOR");
  }
}

void sensor_stateMachine(sl_bt_msg_t *evt){
  //ignore all events aside from the correct periodic timer
  if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_system_soft_timer_id && evt->data.evt_system_soft_timer.handle == SOFT_TIMER_HANDLE_SENSOR){
      sensor_startRead();
  } else if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_system_external_signal_id && Scheduler_Active_TXC()){
      uint16_t touch_value = -1;
      uint8_t proximity_value = -1;

      sensor_finishRead(&touch_value, &proximity_value);

      touch_value = (touch_value >> 8) | (touch_value << 8);

      update_sensor_reading(touch_value, proximity_value);
  }
}

///////////////////////////// Event Generic Wrappers are Below ///////////////////////////////////

static uint32_t Scheduler_Active(sl_bt_msg_t *evt, uint32_t mask);
static uint32_t Scheduler_Active(sl_bt_msg_t *evt, uint32_t mask){
  return evt->data.evt_system_external_signal.extsignals & mask;
}

static void Scheduler_Set(uint32_t mask);
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

#define EVENT_PB0_pressed (0x1 << 3)
uint32_t Scheduler_Active_PB0_pressed(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_PB0_pressed);    }
void Scheduler_Set_PB0_pressed()                            { Scheduler_Set(EVENT_PB0_pressed);                   }

#define EVENT_PB0_released (0x1 << 4)
uint32_t Scheduler_Active_PB0_released(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_PB0_released);    }
void Scheduler_Set_PB0_released()                            { Scheduler_Set(EVENT_PB0_released);                   }

#define EVENT_ButtonState_ToggleIndication (0x1 << 5)
uint32_t Scheduler_Active_ButtonState_ToggleIndication(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_ButtonState_ToggleIndication);    }
void Scheduler_Set_ButtonState_ToggleIndication()                            { Scheduler_Set(EVENT_ButtonState_ToggleIndication);                   }

#define EVENT_PB0_ButtonState_Read (0x1 << 6)
uint32_t Scheduler_Active_ButtonState_Read(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_PB0_ButtonState_Read);    }
void Scheduler_Set_ButtonState_Read()                            { Scheduler_Set(EVENT_PB0_ButtonState_Read);                   }
