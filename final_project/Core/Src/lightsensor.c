/*
 * lightsensor.c
 *
 *  Created on: Nov 30, 2025
 *      Author: shuanlinchen
 */


#include "lightsensor.h"

#include "stdint.h"

void bh1750_init(uint32_t address) {
    I2C2_write(address, 0x01); // Power on command
    // Set the sensor to continuous high-resolution mode
    I2C2_write(address, 0x10); // Continuous H-Resolution Mode
}

uint16_t bh1750_read(uint32_t address) {
    uint8_t data[2] = {0};

    return I2C2_read(address);
}
