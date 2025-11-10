#pragma once
#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <string.h>

#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH   128
#endif
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT  64
#endif

// I2C 7-bit address (commonly 0x3C). If your SA0 is tied low, use 0x3D instead.
#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR 0x3D
#endif

// If you wire the reset pin, define the port/pin here; otherwise undef USE_SSD1306_RESET
#define USE_SSD1306_RESET 1
#define SSD1306_RESET_GPIO_Port  GPIOC
#define SSD1306_RESET_Pin        GPIO_PIN_7

typedef struct {
    I2C_HandleTypeDef *i2c;
    uint8_t buffer[(SSD1306_WIDTH * SSD1306_HEIGHT) / 8];
} ssd1306_t;

HAL_StatusTypeDef ssd1306_init(ssd1306_t *dev, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef ssd1306_update(ssd1306_t *dev);
void ssd1306_fill(ssd1306_t *dev, uint8_t color);  // 0 or 1
void ssd1306_pixel(ssd1306_t *dev, uint8_t x, uint8_t y, uint8_t color);
