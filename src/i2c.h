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

#include "timers.h"

void i2cInit();
void i2cReadTemperature();

#endif /* SRC_I2C_H_ */
