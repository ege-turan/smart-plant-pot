/*
 * camera.h
 *
 *  Created on: Dec 7, 2025
 *      Author: Archer Date
 *      Resources:
 *      https://github.com/ArduCAM/Arduino/tree/master/ArduCAM
 *      https://github.com/ArduCAM/STM32/tree/master
 *      https://www.uctronics.com/download/Image_Sensor/OV5642_camera_module_software_application_notes.pdf
 *      https://web.archive.org/web/20180421030430/http://www.equasys.de/colorconversion.html
 */

#ifndef INC_CAMERA_H_
#define INC_CAMERA_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"

#ifndef _SENSOR_
#define _SENSOR_
struct sensor_reg { // sensor struct that known code uses
	uint16_t reg;
	uint16_t val;
};
#endif

/********/
// I2C BUSINESS
/*******/
#define OV5642_I2C_ADDR 0x3C << 1
#define OV5642_CHIPID_HIGH 0x300a
#define OV5642_CHIPID_LOW  0x300b

#define ARDUCHIP_TEST1       	0x00  //TEST register
#define ARDUCHIP_TIM       		0x03  //Timming control
#define VSYNC_LEVEL_MASK   		0x02  //0 = High active , 		1 = Low active

#define ARDUCHIP_TRIG      		0x41  //Trigger source
#define CAP_DONE_MASK      		0x08

#define ARDUCHIP_FIFO      		0x04  //FIFO and I2C control
#define FIFO_CLEAR_MASK    		0x01
#define FIFO_START_MASK    		0x02

#define BURST_FIFO_READ			0x3C  //Burst FIFO read operation
#define SINGLE_FIFO_READ		0x3D  //Single FIFO read operation

/********/
// SPI BUSINESS
/*******/
#define  CS_PORT	GPIOA
#define  CS_PIN     GPIO_PIN_4

/********/
// BUFFA
/*******/
#define yuyv_data_length       153600 // 320 x 240 x 2 for yuyv data type // i don't actually use this, just for reference
#define rgb888_data_length     230400 // 320 x 240 x 2 for yuyv data type

/********/
// FUNCTIONS + buffer + One Register Table
/*******/

//buffer
extern uint8_t camera_buf[rgb888_data_length];

// register table
extern const struct sensor_reg OV5642_QVGA_Preview[];

void CS_HIGH(void);
void CS_LOW(void);

uint8_t bus_read(uint8_t addr); // spi functions
void bus_write(uint8_t addr, uint8_t data);

void wrSensorReg16_8(uint16_t regID, uint8_t regDat); // i2c functions
void rdSensorReg16_8(uint16_t regID, uint8_t* regDat);
void wrSensorRegs16_8(const struct sensor_reg* regs);

uint8_t get_bit(uint8_t addr, uint8_t bit);

uint8_t read_fifo(void);
void set_fifo_burst(void);
void flush_fifo(void);
void start_capture(void);
void clear_fifo_flag(void);

void ArduCam_Init_YCbCr(void);

void convert_24(uint8_t Y, uint8_t Cb, uint8_t Cr, uint8_t array[3]);
void SingleCapTransfer_YCbCr(int debug_terminal, int debug_python, uint8_t* camera_buf);

#endif /* INC_CAMERA_H_ */

