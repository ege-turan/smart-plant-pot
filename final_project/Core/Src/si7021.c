/*
 * Library for the Adafruit Si7021 breakout board.
 * Works with STM32 microcontroller
 * Author: Ege Turan
 *
 */

#include "si7021.h"

#include "stm32l4xx_hal.h"
#include <stdint.h>   // for uint8_t


/* Private buffer to store read values */
static uint8_t rxbuf[2];

/* Function to read humidity */
float si7021_read_humidity(void)
{
    uint8_t cmd = SI7021_CMD_MEAS_RH_HOLD;

    if (HAL_OK != HAL_I2C_Master_Transmit(&hi2c2, SI7021_ADDR, &cmd, 1, HAL_MAX_DELAY))
        return 0.0f;

    if (HAL_OK != HAL_I2C_Master_Receive(&hi2c2, SI7021_ADDR, rxbuf, 2, HAL_MAX_DELAY))
        return 0.0f;

    uint16_t raw = (rxbuf[0] << 8) | rxbuf[1];
    return ((SI7021_RH_CONV_CONST * raw) / 65536.0f) - SI7021_RH_CONV_OFFSET;
}

/* Function to read temperature */
float si7021_read_temperature(void)
{
    uint8_t cmd = SI7021_CMD_MEAS_TEMP_HOLD;

    if (HAL_OK != HAL_I2C_Master_Transmit(&hi2c2, SI7021_ADDR, &cmd, 1, HAL_MAX_DELAY))
        return 0.0f;

    if (HAL_OK != HAL_I2C_Master_Receive(&hi2c2, SI7021_ADDR, rxbuf, 2, HAL_MAX_DELAY))
        return 0.0f;

    uint16_t raw = (rxbuf[0] << 8) | rxbuf[1];
    return ((SI7021_TEMP_CONV_CONST * raw) / 65536.0f) - SI7021_TEMP_CONV_OFFSET;
}
