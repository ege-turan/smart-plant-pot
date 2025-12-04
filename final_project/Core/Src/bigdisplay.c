#include "bigdisplay.h"
#include "spi.h"
#include "stm32l4xx_hal.h"
#include <stdarg.h> // for TFT_TextPrintf
#include <stdio.h>  // for printf

extern SPI_HandleTypeDef TFT_SPI_HANDLE;

static void BigDisplay_GPIO_Init(void);

static void TFT_Select(void) {
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);
}

static void TFT_Unselect(void) {
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);
}

static void TFT_DC_Command(void) {
  HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);
}

static void TFT_DC_Data(void) {
  HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET);
}

static void TFT_Reset(void) {
  HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(150);
}

static void tft_writeCommand(uint8_t cmd) {
  TFT_DC_Command();
  HAL_StatusTypeDef status =
      HAL_SPI_Transmit(&TFT_SPI_HANDLE, &cmd, 1, HAL_MAX_DELAY);
  if (status != HAL_OK) {
    printf("[TFT][ERR] SPI transmit for CMD 0x%02X failed (status=%d)\r\n", cmd,
           status);
  }
}

static void tft_writeData(const uint8_t *data, uint16_t size) {
  TFT_DC_Data();
  HAL_StatusTypeDef status =
      HAL_SPI_Transmit(&TFT_SPI_HANDLE, (uint8_t *)data, size, HAL_MAX_DELAY);
  if (status != HAL_OK) {
    printf("[TFT][ERR] SPI transmit for DATA block failed (status=%d)\r\n",
           status);
  }
}

static void tft_writeData8(uint8_t data) {
  TFT_DC_Data();
  // printf("[TFT] DATA 0x%02X\r\n", data);
  HAL_StatusTypeDef status =
      HAL_SPI_Transmit(&TFT_SPI_HANDLE, &data, 1, HAL_MAX_DELAY);
  if (status != HAL_OK) {
    printf("[TFT][ERR] SPI transmit for DATA 0x%02X failed (status=%d)\r\n",
           data, status);
  }
}

// Set drawing window (inclusive)
static void TFT_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1,
                                 uint16_t y1) {
  uint8_t buf[4];
  TFT_Select();

  // Column address set
  tft_writeCommand(0x2A);
  buf[0] = x0 >> 8;
  buf[1] = x0 & 0xFF;
  buf[2] = x1 >> 8;
  buf[3] = x1 & 0xFF;
  tft_writeData(buf, 4);

  // Page address set
  tft_writeCommand(0x2B);
  buf[0] = y0 >> 8;
  buf[1] = y0 & 0xFF;
  buf[2] = y1 >> 8;
  buf[3] = y1 & 0xFF;
  tft_writeData(buf, 4);

  TFT_Unselect();
}

static void BigDisplay_GPIO_Init(void) {

  GPIO_InitTypeDef gi = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  gi.Mode = GPIO_MODE_OUTPUT_PP;
  gi.Pull = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  // CS
  gi.Pin = TFT_CS_Pin;
  HAL_GPIO_Init(TFT_CS_GPIO_Port, &gi);
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);

  // DC (RS)
  gi.Pin = TFT_DC_Pin;
  HAL_GPIO_Init(TFT_DC_GPIO_Port, &gi);
  HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);

  // RST
  gi.Pin = TFT_RST_Pin;
  HAL_GPIO_Init(TFT_RST_GPIO_Port, &gi);
  HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);

  // LED (backlight)
  gi.Pin = TFT_LED_Pin;
  HAL_GPIO_Init(TFT_LED_GPIO_Port, &gi);
  HAL_GPIO_WritePin(TFT_LED_GPIO_Port, TFT_LED_Pin, GPIO_PIN_RESET);
}

void TFT_Init(void) {

  BigDisplay_GPIO_Init();

  // Turn on backlight
  HAL_GPIO_WritePin(TFT_LED_GPIO_Port, TFT_LED_Pin, GPIO_PIN_SET);

  // Reset
  __HAL_SPI_DISABLE(&TFT_SPI_HANDLE);
  TFT_Reset();
  __HAL_SPI_ENABLE(&TFT_SPI_HANDLE);

  // Select
  TFT_Select();

  // Send SLEEP OUT
  tft_writeCommand(0x11);
  HAL_Delay(120);

  // Set pixel format
  tft_writeCommand(0x3A);
  tft_writeData8(0x55); // 16-bit color

  // Set MADCTL
  tft_writeCommand(0x36);
  // 0x28 = MADCTL_MV | MADCTL_BGR : landscape, connector on the left
  tft_writeData8(0x28);

  // Display on
  tft_writeCommand(0x29);
  HAL_Delay(20);

  // Unselect
  TFT_Unselect();

  // Clear screen
  // TFT_FillScreen(COLOR_BLACK);
}

void TFT_FillScreen(uint16_t color) {
  // Use logical dimensions so this stays correct for any orientation
  TFT_FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
  if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
    return;
  }

  uint8_t data[2];
  data[0] = color >> 8;
  data[1] = color & 0xFF;

  TFT_Select();

  // Window is 1x1
  // Column
  tft_writeCommand(0x2A);
  uint8_t buf[4];
  buf[0] = x >> 8;
  buf[1] = x & 0xFF;
  buf[2] = x >> 8;
  buf[3] = x & 0xFF;
  tft_writeData(buf, 4);

  // Page
  tft_writeCommand(0x2B);
  buf[0] = y >> 8;
  buf[1] = y & 0xFF;
  buf[2] = y >> 8;
  buf[3] = y & 0xFF;
  tft_writeData(buf, 4);

  // Memory write
  tft_writeCommand(0x2C);
  tft_writeData(data, 2);

  TFT_Unselect();
}

void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  uint16_t color) {

  if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
    return;
  }
  if ((x + w) > TFT_WIDTH) {
    uint16_t old_w = w;
    w = TFT_WIDTH - x;
  }
  if (y + h > TFT_HEIGHT) {
    uint16_t old_h = h;
    h = TFT_HEIGHT - y;
  }

  uint32_t numPixels = (uint32_t)w * (uint32_t)h;

  (void)numPixels;

  static uint8_t lineBuf[480 * 2];
  if (w > 480) {
    w = 480;
  }

  for (uint16_t i = 0; i < w; i++) {
    lineBuf[2 * i + 0] = color >> 8;
    lineBuf[2 * i + 1] = color & 0xFF;
  }

  TFT_Select();

  // Set window
  uint8_t buf[4];

  tft_writeCommand(0x2A); // Column
  buf[0] = x >> 8;
  buf[1] = x & 0xFF;
  buf[2] = (x + w - 1) >> 8;
  buf[3] = (x + w - 1) & 0xFF;
  tft_writeData(buf, 4);

  tft_writeCommand(0x2B); // Page
  buf[0] = y >> 8;
  buf[1] = y & 0xFF;
  buf[2] = (y + h - 1) >> 8;
  buf[3] = (y + h - 1) & 0xFF;

  tft_writeData(buf, 4);

  // Memory write
  tft_writeCommand(0x2C);

  // Send line by line
  for (uint16_t row = 0; row < h; row++) {
    tft_writeData(lineBuf, w * 2);
  }

  TFT_Unselect();
}

/*=====================================================================
 * 5x7 TEXT RENDERING (PORTED FROM SSD1306 VERSION FOR TFT)
 *====================================================================*/

// 5x7 font, each char = 5 bytes, LSB is top pixel. One blank column we add as
// spacing in code. Range: ASCII 0x20..0x7E (95 chars)
static const uint8_t font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00},
    {0x00, 0x07, 0x00, 0x07, 0x00}, {0x14, 0x7F, 0x14, 0x7F, 0x14},
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1C, 0x22, 0x41, 0x00}, {0x00, 0x41, 0x22, 0x1C, 0x00},
    {0x14, 0x08, 0x3E, 0x08, 0x14}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x60, 0x60, 0x00, 0x00}, {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x00, 0x36, 0x36, 0x00, 0x00}, {0x00, 0x56, 0x36, 0x00, 0x00},
    {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3E}, {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x09, 0x01}, {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40}, {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F},
    {0x63, 0x14, 0x08, 0x14, 0x63}, {0x07, 0x08, 0x70, 0x08, 0x07},
    {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7F, 0x00},
    {0x04, 0x02, 0x01, 0x02, 0x04}, {0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20},
    {0x38, 0x44, 0x44, 0x48, 0x7F}, {0x38, 0x54, 0x54, 0x54, 0x18},
    {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x0C, 0x52, 0x52, 0x52, 0x3E},
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00},
    {0x20, 0x40, 0x44, 0x3D, 0x00}, {0x7F, 0x10, 0x28, 0x44, 0x00},
    {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38},
    {0x7C, 0x14, 0x14, 0x14, 0x08}, {0x08, 0x14, 0x14, 0x14, 0x7C},
    {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C},
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, {0x3C, 0x40, 0x30, 0x40, 0x3C},
    {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C},
    {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00},
    {0x00, 0x00, 0x7F, 0x00, 0x00}, {0x00, 0x41, 0x36, 0x08, 0x00},
    {0x10, 0x08, 0x08, 0x10, 0x08},
};

typedef struct {
  uint16_t x;
  uint16_t y;
  uint8_t scale;
  uint8_t wrap;
  uint16_t fg;
  uint16_t bg;
  uint8_t use_bg;
} TFT_TextCfg;

void TFT_TextInit(TFT_TextCfg *t) {
  if (!t)
    return;
  t->x = 0;
  t->y = 0;
  t->scale = 1;
  t->wrap = 1;
  t->fg = COLOR_WHITE;
  t->bg = COLOR_BLACK;
  t->use_bg = 0;
}

void TFT_TextSetCursor(TFT_TextCfg *t, uint16_t x, uint16_t y) {
  if (!t)
    return;
  t->x = x;
  t->y = y;
}

void TFT_TextSetScale(TFT_TextCfg *t, uint8_t s) {
  if (!t)
    return;
  if (s == 0)
    s = 1;
  t->scale = s;
}

void TFT_TextSetWrap(TFT_TextCfg *t, uint8_t w) {
  if (!t)
    return;
  t->wrap = w ? 1u : 0u;
}

void TFT_TextSetColors(TFT_TextCfg *t, uint16_t fg, uint16_t bg,
                       uint8_t use_bg) {
  if (!t)
    return;
  t->fg = fg;
  t->bg = bg;
  t->use_bg = use_bg ? 1u : 0u;
}

static void TFT_DrawPixelScaled(uint16_t x, uint16_t y, uint8_t s,
                                uint16_t color) {
  for (uint8_t dy = 0; dy < s; ++dy) {
    for (uint8_t dx = 0; dx < s; ++dx) {
      TFT_DrawPixel((uint16_t)(x + dx), (uint16_t)(y + dy), color);
    }
  }
}

uint8_t TFT_TextDrawChar(TFT_TextCfg *t, char c) {
  if (!t)
    return 0;

  if (c == '\r')
    return 0;
  if (c == '\n') {
    t->x = 0;
    t->y = (uint16_t)(t->y + (8u * t->scale));
    return 0;
  }

  if ((uint8_t)c < 32u || (uint8_t)c > 126u) {
    c = '?';
  }

  const uint8_t *glyph = font5x7[(uint8_t)c - 32u];

  uint8_t char_w = (uint8_t)(6u * t->scale);
  uint8_t char_h = (uint8_t)(8u * t->scale);

  if (t->wrap && (t->x + char_w) > TFT_WIDTH) {
    t->x = 0;
    t->y = (uint16_t)(t->y + char_h);
  }

  if (t->x >= TFT_WIDTH || t->y >= TFT_HEIGHT) {
    return 0;
  }

  for (uint8_t col = 0; col < 5u; ++col) {
    uint8_t bits = glyph[col];
    for (uint8_t row = 0; row < 7u; ++row) {
      uint16_t px = (uint16_t)(t->x + col * t->scale);
      uint16_t py = (uint16_t)(t->y + row * t->scale);

      if (bits & (1u << row)) {
        TFT_DrawPixelScaled(px, py, t->scale, t->fg);
      } else if (t->use_bg) {
        TFT_DrawPixelScaled(px, py, t->scale, t->bg);
      }
    }
  }

  if (t->use_bg) {
    for (uint8_t row = 0; row < 7u; ++row) {
      uint16_t px = (uint16_t)(t->x + 5u * t->scale);
      uint16_t py = (uint16_t)(t->y + row * t->scale);
      TFT_DrawPixelScaled(px, py, t->scale, t->bg);
    }
  }

  t->x = (uint16_t)(t->x + char_w);
  return char_w;
}

uint16_t TFT_TextDrawString(TFT_TextCfg *t, const char *s) {
  if (!t || !s)
    return 0;
  uint16_t px = 0;
  while (*s) {
    px += TFT_TextDrawChar(t, *s++);
  }
  return px;
}

int TFT_TextPrintf(TFT_TextCfg *t, const char *fmt, ...) {
  if (!t || !fmt)
    return 0;

  char buf[128];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (n < 0) {
    return n;
  }

  TFT_TextDrawString(t, buf);
  return n;
}

uint16_t TFT_DrawStringAt(uint16_t x, uint16_t y, const char *s, uint16_t color,
                          uint8_t scale) {
  TFT_TextCfg cfg;
  TFT_TextInit(&cfg);
  TFT_TextSetCursor(&cfg, x, y);
  TFT_TextSetScale(&cfg, scale);
  TFT_TextSetWrap(&cfg, 0);
  TFT_TextSetColors(&cfg, color, COLOR_BLACK, 0);
  return TFT_TextDrawString(&cfg, s);
}

int TFT_PrintfAt(uint16_t x, uint16_t y, uint16_t color, uint8_t scale,
                 const char *fmt, ...) {
  TFT_TextCfg cfg;
  TFT_TextInit(&cfg);
  TFT_TextSetCursor(&cfg, x, y);
  TFT_TextSetScale(&cfg, scale);
  TFT_TextSetWrap(&cfg, 0);
  TFT_TextSetColors(&cfg, color, COLOR_BLACK, 0);

  char buf[128];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (n < 0) {
    return n;
  }

  TFT_TextDrawString(&cfg, buf);
  return n;
}

void TFT_DrawRGB888Buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                          const uint8_t *buffer) {
  if (!buffer)
    return;

  // Clip if starting outside display
  if (x >= TFT_WIDTH || y >= TFT_HEIGHT)
    return;

  // Clip width/height if extending beyond screen
  if (x + w > TFT_WIDTH)
    w = TFT_WIDTH - x;
  if (y + h > TFT_HEIGHT)
    h = TFT_HEIGHT - y;

  // Prepare TFT for pixel write
  TFT_Select();

  // Set drawing window
  uint8_t buf[4];

  // Column
  tft_writeCommand(0x2A);
  buf[0] = x >> 8;
  buf[1] = x & 0xFF;
  buf[2] = (x + w - 1) >> 8;
  buf[3] = (x + w - 1) & 0xFF;
  tft_writeData(buf, 4);

  // Row
  tft_writeCommand(0x2B);
  buf[0] = y >> 8;
  buf[1] = y & 0xFF;
  buf[2] = (y + h - 1) >> 8;
  buf[3] = (y + h - 1) & 0xFF;
  tft_writeData(buf, 4);

  // Begin memory write
  tft_writeCommand(0x2C);

  // Convert and send row by row: RGB888 → RGB565
  for (uint16_t row = 0; row < h; row++) {
    const uint8_t *src = buffer + (row * w * 3);

    // Send each pixel
    for (uint16_t col = 0; col < w; col++) {
      uint8_t r = *src++;
      uint8_t g = *src++;
      uint8_t b = *src++;

      uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

      uint8_t out[2] = {rgb565 >> 8, rgb565 & 0xFF};
      tft_writeData(out, 2);
    }
  }

  TFT_Unselect();
}

void TFT_DrawSmiley(uint16_t x, uint16_t y, uint16_t radius, uint16_t fg,
                    uint16_t bg, uint8_t happy) {
  // --- Draw face fill (background color) ---
  for (int16_t dy = -(int16_t)radius; dy <= (int16_t)radius; dy++) {
    for (int16_t dx = -(int16_t)radius; dx <= (int16_t)radius; dx++) {
      if (dx * dx + dy * dy <= radius * radius) {
        TFT_DrawPixel(x + dx, y + dy, bg);
      }
    }
  }

  // --- Eyes (foreground color) ---
  uint16_t eyeOffsetX = radius / 2;
  uint16_t eyeOffsetY = radius / 2;

  for (int16_t dy = -2; dy <= 2; dy++) {
    for (int16_t dx = -2; dx <= 2; dx++) {
      TFT_DrawPixel(x - eyeOffsetX + dx, y - eyeOffsetY + dy, fg);
      TFT_DrawPixel(x + eyeOffsetX + dx, y - eyeOffsetY + dy, fg);
    }
  }

  // --- Mouth (arc) in foreground color ---
  int16_t mouthRadius = radius * 2 / 3;
  int16_t mouthYOffset = radius / 4;

  for (int16_t dx = -mouthRadius; dx <= mouthRadius; dx++) {
    float t = (float)dx / (float)mouthRadius;
    float arc = sqrtf(1.0f - t * t);

    int16_t dy = (int16_t)(arc * (mouthRadius / 2));
    if (!happy)
      dy = -dy; // invert for frown

    TFT_DrawPixel(x + dx, y + mouthYOffset + dy, fg);
    TFT_DrawPixel(x + dx, y + mouthYOffset + dy + 1, fg);
  }

  // --- Outer Circle (foreground outline) ---
  int16_t r = radius;
  for (int16_t dy = -r; dy <= r; dy++) {
    for (int16_t dx = -r; dx <= r; dx++) {
      int32_t d = dx * dx + dy * dy;
      // Perimeter band for outline thickness of ~1 pixel
      if (d >= (r * r - r) && d <= (r * r + r)) {
        TFT_DrawPixel(x + dx, y + dy, fg);
      }
    }
  }
}
