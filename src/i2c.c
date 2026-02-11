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

static void i2cEnableSensor();
static void i2cDisableSensor();

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

static void i2cEnableSensor(){
  //power on sensor enable
  GPIO_PinOutSet(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);

  //enable GPIO pins
  GPIO_PinModeSet(I2C0_SCL_PORT, I2C0_SCL_PIN, gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(I2C0_SDA_PORT, I2C0_SDA_PIN, gpioModeWiredAndPullUp, 1);
}

static void i2cDisableSensor(){
  //disable GPIO pins
  GPIO_PinModeSet(I2C0_SCL_PORT, I2C0_SCL_PIN, gpioModeDisabled, 1);
  GPIO_PinModeSet(I2C0_SDA_PORT, I2C0_SDA_PIN, gpioModeDisabled, 1);

  //turn off sensor enable
  GPIO_PinOutClear(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);
}

#define RECEIVE_LENGTH 2
#define TRANSMIT_LENGTH 1
static uint8_t receiveBuffer[RECEIVE_LENGTH] = {0,0};
static uint8_t transmitBuffer[TRANSMIT_LENGTH] = {0xE3}; //Hold Master Mode Read Temperature Command
I2C_TransferSeq_TypeDef TransferSequence = {
    .addr = SI7021_ADDR << 1,
    .flags = I2C_FLAG_WRITE_READ,
    .buf[0] = {.data = transmitBuffer, .len = TRANSMIT_LENGTH},
    .buf[1] = {.data = receiveBuffer, .len = RECEIVE_LENGTH}
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
  uint8_t transferReturn = I2CSPM_Transfer(I2C0, &TransferSequence);
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

  uint16_t temp_code = receiveBuffer[1] | (receiveBuffer[0] << 8);
  float temperature = 175.72f * temp_code / 65536 - 46.85;
  LOG_INFO("Logged Measurement: %.0f Degrees Celsius", temperature);
}

typedef enum i2c_transfer_state_t {
  S0_IDLE = 0,
  S1_STARTUP,
  S2_MEASUREMENT,
  I2C_TRANSFER_NUM_STATES
} i2c_transfer_state_t;

void i2cReadTemperature_nonblocking(){
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

        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
        I2C_TransferReturn_TypeDef transferStatus;
        transferStatus = I2C_TransferInit (I2C0, &TransferSequence);
        if (transferStatus < 0) {
          LOG_ERROR("I2C_TransferInit() Write error = %d", transferStatus);
        }
        NVIC_EnableIRQ(I2C0_IRQn);
      }
      break;
    case S2_MEASUREMENT:
      //if I2C_TransferComplete is triggered after 10.8ms, read the temperature. Then shut down sensor.
      if(Scheduler_Active_TXC()){
        Scheduler_Clear_TXC();
        next_state = S0_IDLE;

        NVIC_DisableIRQ(I2C0_IRQn);
        uint16_t temp_code = receiveBuffer[1] | (receiveBuffer[0] << 8);
        float temperature = 175.72f * temp_code / 65536 - 46.85;
        LOG_INFO("Logged Measurement: %.0f Degrees Celsius", temperature);
        receiveBuffer[1] = 0xAB;
        receiveBuffer[0] = 0xCD;
        i2cDisableSensor();
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      }
      break;
    default:
      next_state = S0_IDLE;
      break;
  }

  current_state = next_state;
}
