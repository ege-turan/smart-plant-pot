#include "i2c.h"

#define BH1750_ADDR (0x23 << 1)

void bh1750_init(uint32_t address);
uint16_t bh1750_read(uint32_t address);
