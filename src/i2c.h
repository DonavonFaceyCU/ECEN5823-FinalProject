/*
 * i2c.h
 *
 *  Created on: Feb 1, 2026
 *      Author: donav
 */

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include "sl_i2cspm.h"
#include "em_cmu.h"
#include "sl_bluetooth.h"

#include "timers.h"
#include "scheduler.h"
#include "lcd.h"

void i2cInit();
void i2c_sensor_init();

void sensor_startRead();
void sensor_finishRead(uint16_t* touch_value, uint8_t* proximity_value);

void i2cEnableSensor();
void i2cDisableSensor();

#endif /* SRC_I2C_H_ */
