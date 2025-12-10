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

// TFT Command Registers
#define TFT_CMD_SLEEP_OUT           0x11
#define TFT_CMD_PIXEL_FORMAT_SET    0x3A
#define TFT_CMD_MADCTL              0x36
#define TFT_CMD_DISPLAY_ON          0x29
#define TFT_CMD_COLUMN_ADDR_SET     0x2A
#define TFT_CMD_PAGE_ADDR_SET       0x2B
#define TFT_CMD_MEMORY_WRITE        0x2C

// TFT Register Values
#define TFT_PIXEL_FORMAT_16BIT      0x55
#define TFT_MADCTL_MV_BGR           0x28  // MADCTL_MV | MADCTL_BGR : landscape, connector on the left

// Timing Delays (ms)
#define TFT_RESET_DELAY             20
#define TFT_RESET_RELEASE_DELAY     150
#define TFT_SLEEP_OUT_DELAY         120
#define TFT_DISPLAY_ON_DELAY        20

// Buffer and Size Constants
#define TFT_MAX_LINE_BUFFER_WIDTH   480
#define TFT_BYTES_PER_PIXEL_16BIT   2
#define TFT_ADDRESS_WINDOW_BUF_SIZE 4
#define TFT_PRINTF_BUFFER_SIZE      128

// Font Constants
#define TFT_FONT_TABLE_SIZE         95
#define TFT_FONT_GLYPH_WIDTH        5
#define TFT_FONT_GLYPH_HEIGHT       7
#define TFT_FONT_ASCII_OFFSET       32
#define TFT_FONT_ASCII_MAX          126
#define TFT_CHAR_WIDTH_PIXELS       6
#define TFT_CHAR_HEIGHT_PIXELS      8

// RGB888 Constants
#define TFT_RGB888_BYTES_PER_PIXEL  3

// RGB565 Conversion Masks
#define TFT_RGB565_R_MASK           0xF8
#define TFT_RGB565_G_MASK           0xFC
#define TFT_RGB565_B_SHIFT          3
#define TFT_RGB565_BYTE_MASK        0xFF

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

void TFT_DrawRGB888Buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *buffer, uint8_t scale);

#endif // BIGDISPLAY_H