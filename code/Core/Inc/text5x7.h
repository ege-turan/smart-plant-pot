#pragma once
#include "ssd1306.h"
#include <stdarg.h>
#include <stdio.h>

// Draw modes
typedef enum { TXT_OFF = 0, TXT_ON = 1, TXT_INV = 2 } txt_color_t;

typedef struct {
    uint8_t x, y;        // cursor in pixels (top-left of next char)
    uint8_t scale;       // 1..N (each pixel becomes scale x scale)
    uint8_t wrap;        // 0 = no wrap, 1 = wrap at right edge
    txt_color_t color;   // TXT_ON / TXT_OFF / TXT_INV
} txt_cfg_t;

void txt_init(txt_cfg_t *t);                              // defaults: x=y=0, scale=1, wrap=1, color=TXT_ON
void txt_set_cursor(txt_cfg_t *t, uint8_t x, uint8_t y);
void txt_set_scale(txt_cfg_t *t, uint8_t s);
void txt_set_wrap(txt_cfg_t *t, uint8_t w);
void txt_set_color(txt_cfg_t *t, txt_color_t c);

// Render one character (ASCII 32..126). Returns char width in pixels advanced (incl spacing).
uint8_t ssd1306_drawChar(ssd1306_t *dev, txt_cfg_t *t, char c);

// Render a C string. Returns total pixels advanced on X.
uint16_t ssd1306_drawText(ssd1306_t *dev, txt_cfg_t *t, const char *s);

// printf-style (writes to a small stack buffer then draws). Returns chars printed.
int ssd1306_printf(ssd1306_t *dev, txt_cfg_t *t, const char *fmt, ...);
