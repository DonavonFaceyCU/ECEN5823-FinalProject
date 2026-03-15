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

#include "gatt_db.h"

#include "i2c.h"
#include "ble.h"

//Functions related to LETIMER0 UF Event
uint32_t Scheduler_Active_UF();
void Scheduler_Set_UF();

uint32_t Scheduler_Active_COMP1();
void Scheduler_Set_COMP1();

uint32_t Scheduler_Active_TXC();
void Scheduler_Set_TXC();

uint32_t Scheduler_Active_PB0_pressed();
void Scheduler_Set_PB0_pressed();

uint32_t Scheduler_Active_PB0_released();
void Scheduler_Set_PB0_released();

void temperature_stateMachine(sl_bt_msg_t *evt);
void discovery_stateMachine(sl_bt_msg_t *evt);

#endif /* SRC_SCHEDULER_H_ */
