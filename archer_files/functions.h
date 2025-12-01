/*
 * functions.h
 *
 *  Created on: Nov 27, 2025
 *      Author: fluffatron
 */

#ifndef INC_FUNCTIONS_H_
#define INC_FUNCTIONS_H_

#include <stdint.h>
#include "ov5642_regs.h"
#include "usart.h"

#define OV5642_I2C_ADDR 0x3C << 1
#define OV5642_CHIPID_HIGH 0x300a
#define OV5642_CHIPID_LOW  0x300b

#define OV5642_320x240 		0	//320x240
#define OV5642_640x480		1	//640x480
#define OV5642_1024x768		2	//1024x768
#define OV5642_1280x960 	3	//1280x960
#define OV5642_1600x1200	4	//1600x1200
#define OV5642_2048x1536	5	//2048x1536
#define OV5642_2592x1944	6	//2592x1944

/********/
// SPI BUSINESS
/*******/
#define  CS_PORT	GPIOA
#define  CS_PIN     GPIO_PIN_4

/********/
// ARDUCHIP BUSINESS
/*******/
#define ARDUCHIP_TEST1       	0x00  //TEST register
#define VSYNC_LEVEL_MASK   		0x02  //0 = High active , 		1 = Low active
#define ARDUCHIP_TIM       		0x03  //Timming control

#define ARDUCHIP_FIFO      		0x04  //FIFO and I2C control
#define FIFO_CLEAR_MASK    		0x01
#define FIFO_START_MASK    		0x02

#define BURST_FIFO_READ			0x3C  //Burst FIFO read operation
#define SINGLE_FIFO_READ		0x3D  //Single FIFO read operation

#define FIFO_SIZE1				0x42  //Camera write FIFO size[7:0] for burst to read
#define FIFO_SIZE2				0x43  //Camera write FIFO size[15:8]
#define FIFO_SIZE3				0x44  //Camera write FIFO size[18:16]

#define ARDUCHIP_TRIG      		0x41  //Trigger source
#define VSYNC_MASK         		0x01
#define SHUTTER_MASK       		0x02
#define CAP_DONE_MASK      		0x08

#define  BUFFER_MAX_SIZE 4096


/********/
// FUNCTIONS
/*******/

void CS_HIGH(void);
void CS_LOW(void);

uint8_t bus_read(uint8_t addr);
void bus_write(uint8_t addr, uint8_t data);

void wrSensorReg16_8(uint16_t regID, uint8_t regDat);
void rdSensorReg16_8(uint16_t regID, uint8_t* regDat);
void wrSensorRegs16_8(const struct sensor_reg* regs);

uint8_t get_bit(uint8_t addr, uint8_t bit);

void ArduCam_Init(void);
void OV5642_set_JPEG_size(uint8_t size);

void set_fifo_burst(void);
uint8_t read_fifo(void);
void flush_fifo(void);
void start_capture(void);
void clear_fifo_flag(void );
uint32_t read_fifo_length(void);

void SingleCapTransfer(void);

#endif /* INC_FUNCTIONS_H_ */
