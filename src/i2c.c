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

#define MPR121_ADDR                         0x5A
#define MPR121_TOUCH_STATUS_0_7             0x00
#define MPR121_TOUCH_STATUS_8_11_PROX       0x01
#define MPR121_MHD_RISING                   0x2B
#define MPR121_NHD_RISING                   0x2C
#define MPR121_NCL_RISING                   0x2D
#define MPR121_FDL_RISING                   0x2E
#define MPR121_MHD_FALLING                  0x2F
#define MPR121_NHD_FALLING                  0x30
#define MPR121_NCL_FALLING                  0x31
#define MPR121_FDL_FALLING                  0x32
#define MPR121_NHD_TOUCHED                  0x33
#define MPR121_NCL_TOUCHED                  0x34
#define MPR121_FDL_TOUCHED                  0x35
#define MPR121_MHD_PROX_RISING              0x36
#define MPR121_NHD_PROX_RISING              0x37
#define MPR121_NCL_PROX_RISING              0x38
#define MPR121_FDL_PROX_RISING              0x39
#define MPR121_MHD_PROX_FALLING             0x3A
#define MPR121_NHD_PROX_FALLING             0x3B
#define MPR121_NCL_PROX_FALLING             0x3C
#define MPR121_FDL_PROX_FALLING             0x3D
#define MPR121_NHD_PROX_TOUCHED             0x3E
#define MPR121_NCL_PROX_TOUCHED             0x3F
#define MPR121_FDL_PROX_TOUCHED             0x40
#define MPR121_ELE0_TOUCH_THRESHOLD         0x41
#define MPR121_ELE0_RELEASE_THRESHOLD       0x42
#define MPR121_ELEPROX_TOUCH_THRESHOLD      0x59
#define MPR121_ELEPROX_RELEASE_THRESHOLD    0x5A
#define MPR121_DEBOUNCE                     0x5B
#define MPR121_CDC_CONFIG                   0x5C
#define MPR121_CDT_CONFIG                   0x5D
#define MPR121_ELECTRODE_CONFIG             0x5E
#define MPR121_AUTOCONFIG_CR_0              0x7B
#define MPR121_AUTOCONFIG_CR_1              0x7C
#define MPR121_AUTOCONFIG_USL               0x7D
#define MPR121_AUTOCONFIG_LSL               0x7E
#define MPR121_AUTOCONFIG_TARGET            0x7F
#define MPR121_SOFT_RESET                   0x80

static I2C_TransferReturn_TypeDef MPR121_write_blocking(uint8_t reg, uint8_t val){
    uint8_t tx[2] = { reg, val };

    I2C_TransferSeq_TypeDef seq = {
        .addr  = MPR121_ADDR << 1,
        .flags = I2C_FLAG_WRITE,
        .buf[0] = { .data = tx, .len = sizeof(tx) },
    };

    I2C_TransferReturn_TypeDef ret = I2C_TransferInit(I2C0, &seq);
    while (ret == i2cTransferInProgress) {
        ret = I2C_Transfer(I2C0);          /* poll until done */
    }
    return ret;
}

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

void i2c_sensor_init(){
    MPR121_write_blocking(MPR121_SOFT_RESET, 0x63);
    timerWaitUs_polled(1000);
    MPR121_write_blocking(MPR121_ELECTRODE_CONFIG, 0x00);

    //Default electrode values taken from AN3891
    MPR121_write_blocking(MPR121_MHD_RISING,   0x01);
    MPR121_write_blocking(MPR121_NHD_RISING,   0x01);
    MPR121_write_blocking(MPR121_NCL_RISING,   0x00);
    MPR121_write_blocking(MPR121_FDL_RISING,   0x00);
    MPR121_write_blocking(MPR121_MHD_FALLING,  0x01);
    MPR121_write_blocking(MPR121_NHD_FALLING,  0x01);
    MPR121_write_blocking(MPR121_NCL_FALLING,  0xFF);
    MPR121_write_blocking(MPR121_FDL_FALLING,  0x02);
    MPR121_write_blocking(MPR121_NHD_TOUCHED,  0x00);
    MPR121_write_blocking(MPR121_NCL_TOUCHED,  0x00);
    MPR121_write_blocking(MPR121_FDL_TOUCHED,  0x00);

    //Default proximity values taken from AN3893
    MPR121_write_blocking(MPR121_MHD_PROX_RISING,   0xFF);
    MPR121_write_blocking(MPR121_NHD_PROX_RISING,   0xFF);
    MPR121_write_blocking(MPR121_NCL_PROX_RISING,   0x00);
    MPR121_write_blocking(MPR121_FDL_PROX_RISING,   0x00);
    MPR121_write_blocking(MPR121_MHD_PROX_FALLING,  0x01);
    MPR121_write_blocking(MPR121_NHD_PROX_FALLING,  0x01);
    MPR121_write_blocking(MPR121_NCL_PROX_FALLING,  0xFF);
    MPR121_write_blocking(MPR121_FDL_PROX_FALLING,  0xFF);
    MPR121_write_blocking(MPR121_NHD_PROX_TOUCHED,  0x00);
    MPR121_write_blocking(MPR121_NCL_PROX_TOUCHED,  0x00);
    MPR121_write_blocking(MPR121_FDL_PROX_TOUCHED,  0x00);

    for (uint8_t i = 0; i < 12; i++) {
        MPR121_write_blocking(MPR121_ELE0_TOUCH_THRESHOLD   + (i * 2), 0x0F);
        MPR121_write_blocking(MPR121_ELE0_RELEASE_THRESHOLD + (i * 2), 0x0A);
    }

    MPR121_write_blocking(MPR121_ELEPROX_TOUCH_THRESHOLD,   0x02);
    MPR121_write_blocking(MPR121_ELEPROX_RELEASE_THRESHOLD, 0x01);
    MPR121_write_blocking(MPR121_CDC_CONFIG, 0x10);
    MPR121_write_blocking(MPR121_CDT_CONFIG, 0x20);
    MPR121_write_blocking(MPR121_DEBOUNCE,   0x00);
    MPR121_write_blocking(MPR121_AUTOCONFIG_CR_0,    0x0B);
    MPR121_write_blocking(MPR121_AUTOCONFIG_CR_1,    0x80);
    MPR121_write_blocking(MPR121_AUTOCONFIG_USL,     201);
    MPR121_write_blocking(MPR121_AUTOCONFIG_LSL,     130);
    MPR121_write_blocking(MPR121_AUTOCONFIG_TARGET,  181);
    MPR121_write_blocking(MPR121_ELECTRODE_CONFIG, 0xAC);
}

#define TOUCH_STATUS_LENGTH      2
//static uint8_t touchStatusRegAddr = MPR121_CDC_CONFIG;
static uint8_t touchStatusRegAddr = MPR121_TOUCH_STATUS_0_7;
static uint8_t touchStatusBuffer[TOUCH_STATUS_LENGTH];

I2C_TransferSeq_TypeDef ReadingTransferSequence = {
    .addr  = MPR121_ADDR << 1,
    .flags = I2C_FLAG_WRITE_READ,
    .buf[0] = { .data = &touchStatusRegAddr, .len = 1                   },
    .buf[1] = { .data = touchStatusBuffer,   .len = TOUCH_STATUS_LENGTH },
};

void sensor_startRead(){
  I2C_TransferInit(I2C0, &ReadingTransferSequence);
  NVIC_EnableIRQ(I2C0_IRQn);
}

void sensor_finishRead(uint16_t* touch_value, uint8_t* proximity_value){
  NVIC_DisableIRQ(I2C0_IRQn);

  uint16_t compound_register = (touchStatusBuffer[0] | touchStatusBuffer[1] << 8);

  *touch_value = compound_register & 0x0FFF;             /* ELE0..ELE11 only */
  *proximity_value = (compound_register & 0x1000) >> 12; /* bit 12 = ELEPROX */
}

void i2cEnableSensor(){
  //power on sensor enable
  GPIO_PinOutSet(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);

  //enable GPIO pins
  GPIO_PinModeSet(I2C0_SCL_PORT, I2C0_SCL_PIN, gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(I2C0_SDA_PORT, I2C0_SDA_PIN, gpioModeWiredAndPullUp, 1);
}
