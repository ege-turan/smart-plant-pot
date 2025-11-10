#include "ssd1306.h"

// Control bytes per SSD1306 I2C protocol: 0x00=commands, 0x40=data
// (Co=0,D/C#=0) for commands; (Co=0,D/C#=1) for data stream.
#define SSD1306_CTRL_CMD   0x00
#define SSD1306_CTRL_DATA  0x40

static HAL_StatusTypeDef write_cmd(ssd1306_t *dev, uint8_t cmd) {
    uint8_t buf[2] = { SSD1306_CTRL_CMD, cmd };
    return HAL_I2C_Master_Transmit(dev->i2c, (SSD1306_I2C_ADDR << 1), buf, sizeof(buf), HAL_MAX_DELAY);
}

static HAL_StatusTypeDef write_cmds(ssd1306_t *dev, const uint8_t *cmds, size_t n) {
    // Send control byte + N commands as one I2C transaction to avoid NACK between bytes
    uint8_t tmp[1 + 16];
    while (n) {
        size_t chunk = (n > 16) ? 16 : n;
        tmp[0] = SSD1306_CTRL_CMD;
        memcpy(&tmp[1], cmds, chunk);
        HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(dev->i2c, (SSD1306_I2C_ADDR << 1), tmp, 1 + chunk, HAL_MAX_DELAY);
        if (st != HAL_OK) return st;
        cmds += chunk; n -= chunk;
    }
    return HAL_OK;
}

static HAL_StatusTypeDef write_data(ssd1306_t *dev, const uint8_t *data, size_t n) {
    // Stream data with a 0x40 control byte prefix; chunk to keep stack/use light
    uint8_t tmp[1 + 16];
    tmp[0] = SSD1306_CTRL_DATA;
    while (n) {
        size_t chunk = (n > 16) ? 16 : n;
        memcpy(&tmp[1], data, chunk);
        HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(dev->i2c, (SSD1306_I2C_ADDR << 1), tmp, 1 + chunk, HAL_MAX_DELAY);
        if (st != HAL_OK) return st;
        data += chunk; n -= chunk;
    }
    return HAL_OK;
}

static HAL_StatusTypeDef set_addressing_window(ssd1306_t *dev, uint8_t x0, uint8_t x1, uint8_t page0, uint8_t page1) {
    uint8_t cmds[] = {
        0x20, 0x00,             // Memory mode: horizontal addressing
        0x21, x0, x1,           // Column addr: start, end
        0x22, page0, page1      // Page addr: start, end
    };
    return write_cmds(dev, cmds, sizeof(cmds));
}

HAL_StatusTypeDef ssd1306_init(ssd1306_t *dev, I2C_HandleTypeDef *hi2c) {
    dev->i2c = hi2c;

#ifdef USE_SSD1306_RESET
    // If you wired RES#, give a proper reset pulse after VDD is stable
    HAL_GPIO_WritePin(SSD1306_RESET_GPIO_Port, SSD1306_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);   // >3us
    HAL_GPIO_WritePin(SSD1306_RESET_GPIO_Port, SSD1306_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
#endif

    // Recommended init sequence for 128x64 panel using internal charge pump:
    // (values per SSD1306 app note/datasheet)
    uint8_t init_seq[] = {
        0xAE,             // Display OFF
        0xD5, 0x80,       // Set display clock divide/oscillator freq
        0xA8, 0x3F,       // Set multiplex ratio (0x3F for 64)
        0xD3, 0x00,       // Display offset = 0
        0x40,             // Start line = 0
        0x8D, 0x14,       // Charge pump ON (0x14)  [0x8D, 0x10 for external VCC]
        0x20, 0x00,       // Memory mode: horizontal addressing
        0xA1,             // Segment remap (column address 127 is SEG0)
        0xC8,             // COM scan direction remapped (scan from COM[N-1] to COM0)
        0xDA, 0x12,       // COM pins hw config (alternative COM pin config, disable left/right remap)
        0x81, 0x7F,       // Contrast
        0xD9, 0xF1,       // Pre-charge period
        0xDB, 0x40,       // VCOMH deselect level
        0xA4,             // Entire display ON (resume from RAM)
        0xA6,             // Normal display (not inverted)
        0x2E,             // Deactivate scroll
        0xAF              // Display ON
    };
    HAL_StatusTypeDef st = write_cmds(dev, init_seq, sizeof(init_seq));
    if (st != HAL_OK) return st;

    ssd1306_fill(dev, 0);
    return ssd1306_update(dev);
}

HAL_StatusTypeDef ssd1306_update(ssd1306_t *dev) {
    HAL_StatusTypeDef st = set_addressing_window(dev, 0, SSD1306_WIDTH - 1, 0, (SSD1306_HEIGHT/8) - 1);
    if (st != HAL_OK) return st;
    return write_data(dev, dev->buffer, sizeof(dev->buffer));
}

void ssd1306_fill(ssd1306_t *dev, uint8_t color) {
    memset(dev->buffer, color ? 0xFF : 0x00, sizeof(dev->buffer));
}

void ssd1306_pixel(ssd1306_t *dev, uint8_t x, uint8_t y, uint8_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    uint16_t index = x + (y / 8) * SSD1306_WIDTH;
    uint8_t  mask  = 1U << (y & 7);
    if (color) dev->buffer[index] |= mask;
    else       dev->buffer[index] &= (uint8_t)~mask;
}
