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

void i2cInit();
void i2cReadTemperature_blocking();

void i2cReadTemperature_nonblocking_start();
uint32_t i2cReadTemperature_nonblocking_finish();

void i2cEnableSensor();
void i2cDisableSensor();

void i2cCommandTemperatureReading();

#endif /* SRC_I2C_H_ */
