/*
 * Library for the Adafruit Si7021 breakout board.
 * Works with STM32 microcontroller
 * Author: Ege Turan
 *
 */


#ifndef INC_SI7021_H
#define INC_SI7021_H

#include "stm32l4xx_hal.h"

/* Addresses and commands from Si7021 datasheet */
#define SI7021_ADDR               (0x40 << 1)   // 7-bit address shifted left for HAL
#define SI7021_CMD_MEAS_RH_HOLD   0xE5
#define SI7021_CMD_MEAS_TEMP_HOLD 0xE3
#define SI7021_CMD_READ_PREV_TEMP 0xE0

/* Conversion constants from Si7021 datasheet */
#define SI7021_RH_CONV_CONST      (125.0f)
#define SI7021_RH_CONV_OFFSET     (6.0f)
#define SI7021_TEMP_CONV_CONST    (175.72f)
#define SI7021_TEMP_CONV_OFFSET   (46.85f)

/* Public I2C handle */
extern I2C_HandleTypeDef hi2c2;

/* Public function prototypes */
float si7021_read_humidity(void);
float si7021_read_temperature(void);

#endif /* INC_SI7021_H */
