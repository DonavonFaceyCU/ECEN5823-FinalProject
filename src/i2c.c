/*
 * i2c.c
 *
 *  Created on: Feb 1, 2026
 *      Author: donav
 */

#include "i2c.h"

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "log.h"

#define I2C0_SCL_PORT gpioPortC
#define I2C0_SCL_PIN  10
#define I2C0_SDA_PORT gpioPortC
#define I2C0_SDA_PIN  11

#define SENSOR_ENABLE_PORT gpioPortD
#define SENSOR_ENABLE_PIN  15

#define SI7021_ADDR   0x40
#define SI7021_NO_HOLD_TEMP_READ 0xE0

void i2cInit(){
  CMU_ClockEnable(cmuClock_I2C0, true);

  GPIO_PinModeSet(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN, gpioModePushPull, 1);

  I2C0->ROUTEPEN  = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
  I2C0->ROUTELOC0 = I2C_ROUTELOC0_SDALOC_LOC16 | I2C_ROUTELOC0_SCLLOC_LOC14;

  I2CSPM_Init_TypeDef I2C0_Init = {
    .port               = I2C0,
    .sclPort            = I2C0_SCL_PORT,
    .sclPin             = I2C0_SCL_PIN,
    .sdaPort            = I2C0_SDA_PORT,
    .sdaPin             = I2C0_SDA_PIN,
    .portLocationScl    = 14,
    .portLocationSda    = 16,
    .i2cRefFreq         = 0,
    .i2cMaxFreq         = I2C_FREQ_FAST_MAX,
    .i2cClhr            = i2cClockHLRFast
  };

  I2CSPM_Init(&I2C0_Init);

  I2C_IntEnable(I2C0, I2C_IEN_TXC);
}

void i2cEnableSensor(){
  //power on sensor enable
  GPIO_PinOutSet(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);

  //enable GPIO pins
  GPIO_PinModeSet(I2C0_SCL_PORT, I2C0_SCL_PIN, gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(I2C0_SDA_PORT, I2C0_SDA_PIN, gpioModeWiredAndPullUp, 1);
}

void i2cDisableSensor(){
  //disable GPIO pins
  GPIO_PinModeSet(I2C0_SCL_PORT, I2C0_SCL_PIN, gpioModeDisabled, 1);
  GPIO_PinModeSet(I2C0_SDA_PORT, I2C0_SDA_PIN, gpioModeDisabled, 1);

  //turn off sensor enable
  GPIO_PinOutClear(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);
}

#define BLOCKING_RECEIVE_LENGTH 2
#define BLOCKING_TRANSMIT_LENGTH 1
static uint8_t blockingReceiveBuffer[BLOCKING_RECEIVE_LENGTH] = {0,0};
static uint8_t blockingTransmitBuffer[BLOCKING_TRANSMIT_LENGTH] = {0xE3}; //Hold Master Mode Read Temperature Command
I2C_TransferSeq_TypeDef BlockingTransferSequence = {
    .addr = SI7021_ADDR << 1,
    .flags = I2C_FLAG_WRITE,
    .buf[0] = {.data = blockingTransmitBuffer, .len = BLOCKING_TRANSMIT_LENGTH},
    .buf[1] = {.data = blockingReceiveBuffer, .len = BLOCKING_RECEIVE_LENGTH}
};

void i2cReadTemperature_blocking(){
  //power on sensor enable
  GPIO_PinOutSet(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);

  //enable GPIO pins
  GPIO_PinModeSet(I2C0_SCL_PORT, I2C0_SCL_PIN, gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(I2C0_SDA_PORT, I2C0_SDA_PIN, gpioModeWiredAndPullUp, 1);

  //wait for power to come up
  timerWaitUs_polled(80000);

  //send temperature command
  uint8_t transferReturn = I2CSPM_Transfer(I2C0, &BlockingTransferSequence);
  if(transferReturn != i2cTransferDone){
      LOG_ERROR("I2C Transfer Failed with code:%u", transferReturn);
  }

  //wait for temperature to be read
  //timerWaitUs_polled(10800);

  //read temperature
  //Done in above transfer

  //disable GPIO pins
  GPIO_PinModeSet(I2C0_SCL_PORT, I2C0_SCL_PIN, gpioModeDisabled, 1);
  GPIO_PinModeSet(I2C0_SDA_PORT, I2C0_SDA_PIN, gpioModeDisabled, 1);

  //turn off sensor enable
  GPIO_PinOutClear(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);

  uint16_t temp_code = blockingReceiveBuffer[1] | (blockingReceiveBuffer[0] << 8);
  float temperature = 175.72f * (temp_code >> 16) - 46.85;
  LOG_INFO("Logged Measurement: %.0f Degrees Celsius", temperature);
}

#define COMMAND_TRANSMIT_LENGTH 1
static uint8_t commandTransmitBuffer[BLOCKING_TRANSMIT_LENGTH] = {0xF3}; //No Hold Master Mode Read Temperature Command
I2C_TransferSeq_TypeDef CommandTransferSequence = {
    .addr = SI7021_ADDR << 1,
    .flags = I2C_FLAG_WRITE,
    .buf[0] = {.data = commandTransmitBuffer, .len = COMMAND_TRANSMIT_LENGTH},
};

void i2cCommandTemperatureReading(){
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
  I2C_TransferReturn_TypeDef transferStatus;
  transferStatus = I2C_TransferInit (I2C0, &CommandTransferSequence);
  if (transferStatus < 0) {
    LOG_ERROR("I2C_TransferInit() Write error = %d", transferStatus);
  }
}

#define READING_RECEIVE_LENGTH 2
static uint8_t readingReceiveBuffer[READING_RECEIVE_LENGTH] = {0xAB, 0xCD}; //No Hold Master Mode Read Temperature Command
I2C_TransferSeq_TypeDef ReadingTransferSequence = {
    .addr = SI7021_ADDR << 1,
    .flags = I2C_FLAG_READ,
    .buf[0] = {.data = readingReceiveBuffer, .len = READING_RECEIVE_LENGTH},
};

void i2cReadTemperature_nonblocking_start(){
  I2C_TransferReturn_TypeDef transferStatus;
  transferStatus = I2C_TransferInit (I2C0, &ReadingTransferSequence);
  if (transferStatus < 0) {
    LOG_ERROR("I2C_TransferInit() Read error = %d", transferStatus);
  }
}

uint32_t i2cReadTemperature_nonblocking_finish(){
  uint16_t temp_code = readingReceiveBuffer[1] | (readingReceiveBuffer[0] << 8);
  //uint32_t temperature = 175720 * temp_code / 65536 - 46850;
  //uint32_t temperature_11073 = ((uint32_t)(uint8_t)(-3) << 24) | ((temperature & 0x00FFFFFF) << 0);

  uint32_t temperature = 1760 * temp_code / 65536 - 470;
  uint32_t temperature_11073 = ((uint32_t)(uint8_t)(-1) << 24) | ((temperature & 0x00FFFFFF) << 0);
  //LOG_INFO("Logged Measurement: %u Degrees Celsius", temperature);

  sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
  return temperature_11073;
}


