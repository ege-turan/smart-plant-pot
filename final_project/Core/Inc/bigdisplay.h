#ifndef BIGDISPLAY_H
#define BIGDISPLAY_H

// #include "main.h"
#include "spi.h"

#define TFT_WIDTH   480
#define TFT_HEIGHT  320

#define TFT_SPI_HANDLE      hspi1

#define TFT_CS_GPIO_Port    GPIOD
#define TFT_CS_Pin          GPIO_PIN_0

#define TFT_DC_GPIO_Port    GPIOD
#define TFT_DC_Pin          GPIO_PIN_1

#define TFT_RST_GPIO_Port   GPIOF
#define TFT_RST_Pin         GPIO_PIN_2

#define TFT_LED_GPIO_Port   GPIOB
#define TFT_LED_Pin         GPIO_PIN_6


#define RGB565(r, g, b) \
    (uint16_t)((((r) & 0x1F) << 11) | (((g) & 0x3F) << 5) | ((b) & 0x1F))

#define COLOR_BLACK     0x0000
#define COLOR_BLUE      0x001F
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_YELLOW    0xFFE0
#define COLOR_WHITE     0xFFFF
#define COLOR_ORANGE    0xFD20
#define COLOR_GRAY      0x8410
#define COLOR_NAVY      0x000F

void TFT_Init(void);
void TFT_FillScreen(uint16_t color);
void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

#endif // BIGDISPLAY_H