/*
 * Library for the Adafruit STEMMA Soil Sensor (I2C Capacitive Moisture)
 * Works with STM32 microcontroller
 * Author: Ege Turan
 *
 */

#include "soil.h"

#include "stm32l4xx_hal.h"
#include <stdint.h>

/* Seesaw register addresses */
#define SEESAW_STATUS_BASE      0x00
#define SEESAW_STATUS_SWRST     0x7F
#define SEESAW_TOUCH_BASE       0x0F
#define SEESAW_TOUCH_CHANNEL_OFFSET  0x10
#define SEESAW_STATUS_TEMP      0x00

/* Private buffer to store read values and to keep track of initialization */
static uint8_t rxbuf[4];
static uint8_t initialized = 0;

/* Private function to initialize the sensor */
static void soil_init(void)
{
    if (initialized) return;

    // Software reset
    uint8_t reset_cmd[2] = {SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST};
    HAL_I2C_Master_Transmit(&hi2c2, SOIL_ADDR, reset_cmd, 2, HAL_MAX_DELAY);

    HAL_Delay(10);  	// Wait for reset

    initialized = 1;	// set once to 1 after first use/initilization
}

/* Function to read capacitance (moisture level) */
uint16_t soil_read_capacitance(void)
{
    soil_init();

    // Read touch/capacitance: base 0x0F, function 0x10
    uint8_t cmd[2] = {SEESAW_TOUCH_BASE, SEESAW_TOUCH_CHANNEL_OFFSET};

    if (HAL_OK != HAL_I2C_Master_Transmit(&hi2c2, SOIL_ADDR, cmd, 2, HAL_MAX_DELAY))
        return 0;

    HAL_Delay(3);  // Sensor needs time to measure

    if (HAL_OK != HAL_I2C_Master_Receive(&hi2c2, SOIL_ADDR, rxbuf, 2, HAL_MAX_DELAY))
        return 0;

    uint16_t capacitance = (rxbuf[0] << 8) | rxbuf[1];
    return capacitance; // a reading ranging from about 200 (very dry) to 2000 (very wet)
}

/* Function to read temperature */
float soil_read_temperature(void)
{
    soil_init();

    // Read temperature: base 0x00, function 0x04
    uint8_t cmd[2] = {SEESAW_STATUS_BASE, 0x04};

    if (HAL_OK != HAL_I2C_Master_Transmit(&hi2c2, SOIL_ADDR, cmd, 2, HAL_MAX_DELAY))
        return 0.0f;

    HAL_Delay(3);  // Sensor needs time to measure

    if (HAL_OK != HAL_I2C_Master_Receive(&hi2c2, SOIL_ADDR, rxbuf, 4, HAL_MAX_DELAY))
        return 0.0f;

    int32_t raw = ((int32_t)rxbuf[0] << 24) | ((int32_t)rxbuf[1] << 16) | ((int32_t)rxbuf[2] << 8) | rxbuf[3];

    // Convert: (1.0 / (1UL << 16)) * raw
    return raw / 65536.0f;
}
