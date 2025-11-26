/*
 * Library for the Adafruit STEMMA Soil Sensor (I2C Capacitive Moisture)
 * Works with STM32 microcontroller
 * Author: Ege Turan
 */


#ifndef INC_SOIL_H
#define INC_SOIL_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

/* I2C address (7-bit left aligned for HAL) */
#define SOIL_ADDR               (0x36 << 1)

/* Register addresses for STEMMA Soil Sensor */
// a reading ranging from about 200 (very dry) to 2000 (very wet)
#define SOIL_REG_CAPACITANCE    0x00  // 2 bytes, big-endian
#define SOIL_REG_TEMPERATURE    0x04  // 2 bytes, big-endian

/* Public I2C handle */
extern I2C_HandleTypeDef hi2c2;

/* Public function prototypes */
uint16_t soil_read_capacitance(void);
float    soil_read_temperature(void);

#endif /* INC_SOIL_H */
