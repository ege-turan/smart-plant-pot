/*
 * functions.c
 *
 *  Created on: Nov 27, 2025
 *      Author: fluffatron
 */


#include "functions.h"
#include "spi.h"
#include "i2c.h"

void CS_HIGH(void)
{
	HAL_GPIO_WritePin(CS_PORT,CS_PIN, GPIO_PIN_SET);
}

void CS_LOW(void)
{
	HAL_GPIO_WritePin(CS_PORT,CS_PIN, GPIO_PIN_RESET);
}

// previously read_reg and read_write but collapsed down to bus read/write
uint8_t bus_read(uint8_t addr)
{
	uint8_t taddr = addr & 0x7F;
	uint8_t data = 0;
	uint8_t dummy = 0;

	CS_LOW();
	HAL_SPI_Transmit(&hspi1, &taddr, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummy, &data, 1, HAL_MAX_DELAY);
	CS_HIGH();

	return data;
}

void bus_write(uint8_t addr, uint8_t data)
{
	uint8_t taddr = addr | 0x80;
	CS_LOW();
	HAL_SPI_Transmit(&hspi1, &taddr, 1, HAL_MAX_DELAY);
	HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
	CS_HIGH();
}


// all registers are 16 bit addresses
void wrSensorReg16_8(uint16_t regID, uint8_t regDat)
{
	HAL_I2C_Mem_Write(&hi2c4, OV5642_I2C_ADDR, regID, I2C_MEMADD_SIZE_16BIT, &regDat, 1, HAL_MAX_DELAY);
}


void rdSensorReg16_8(uint16_t regID, uint8_t* regDat)
{
	HAL_I2C_Mem_Read(&hi2c4, OV5642_I2C_ADDR, regID, I2C_MEMADD_SIZE_16BIT, regDat, 1, HAL_MAX_DELAY);
}

void wrSensorRegs16_8(const struct sensor_reg* regs)
{

	uint16_t reg_addr = regs-> reg;
	uint8_t reg_val = regs -> val;

	while ((reg_addr != 0xffff) || (reg_val != 0xff)) {
		HAL_I2C_Mem_Write(&hi2c4, OV5642_I2C_ADDR, reg_addr, I2C_MEMADD_SIZE_16BIT, &reg_val, 1, HAL_MAX_DELAY);
		HAL_Delay(1);
		regs++;
		reg_addr =regs->reg;
		reg_val = regs->val;
	}

}

uint8_t get_bit(uint8_t addr, uint8_t bit)
{
    uint8_t temp = bus_read(addr);
    return temp & bit;
}


// lets just use jpeg
void ArduCam_Init(void){

	wrSensorReg16_8(0x3008, 0x82);
	HAL_Delay(100);

	// wakes the sensor, not in the example driver though...
	wrSensorReg16_8(0x3008, 0x02);
	HAL_Delay(100);

	wrSensorRegs16_8(OV5642_QVGA_Preview);
	HAL_Delay(100);

	wrSensorRegs16_8(OV5642_JPEG_Capture_QSXGA);
	HAL_Delay(100);

	wrSensorRegs16_8(OV5642_320x240);
	HAL_Delay(100);

	wrSensorReg16_8(0x3818, 0xa8);
	wrSensorReg16_8(0x3621, 0x10);
	wrSensorReg16_8(0x3801, 0xb0);
	wrSensorReg16_8(0x4407, 0x04);

	bus_write(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH // WAS WRITE_REG
}


void OV5642_set_JPEG_size(uint8_t size)
{
  switch (size)
  {
    case OV5642_320x240:
      wrSensorRegs16_8(ov5642_320x240);
      break;
    case OV5642_640x480:
      wrSensorRegs16_8(ov5642_640x480);
      break;
    case OV5642_1024x768:
      wrSensorRegs16_8(ov5642_1024x768);
      break;
    case OV5642_1280x960:
      wrSensorRegs16_8(ov5642_1280x960);
      break;
    case OV5642_1600x1200:
      wrSensorRegs16_8(ov5642_1600x1200);
      break;
    case OV5642_2048x1536:
      wrSensorRegs16_8(ov5642_2048x1536);
      break;
    case OV5642_2592x1944:
      wrSensorRegs16_8(ov5642_2592x1944);
      break;
    default:
      wrSensorRegs16_8(ov5642_320x240);
      break;
  }
}


uint8_t read_fifo(void)
{
	uint8_t data;
	data = bus_read(SINGLE_FIFO_READ);
	return data;
}

// only needed for super fast transfers e.g. bmp or video but is also used in singlecaptransfer
void set_fifo_burst(void )
{
	uint8_t data = BURST_FIFO_READ;   // 0x3C
	HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}


void flush_fifo(void)
{
	bus_write(ARDUCHIP_FIFO, FIFO_CLEAR_MASK); // write_reg --> bus_write
}

void start_capture(void)
{
	bus_write(ARDUCHIP_FIFO, FIFO_START_MASK); // write_reg --> bus_write
}

void clear_fifo_flag(void )
{
	bus_write(ARDUCHIP_TRIG, CAP_DONE_MASK);
}

uint32_t read_fifo_length(void)
{
	uint32_t len1,len2,len3,len=0;
	len1 = bus_read(FIFO_SIZE1);
	len2 = bus_read(FIFO_SIZE2);
	len3 = bus_read(FIFO_SIZE3) & 0x7f;
	len = ((len3 << 16) | (len2 << 8) | len1) & 0x07fffff;
	return len;
}

void SingleCapTransfer(void)
{
	uint32_t length = 0;
	uint8_t temp;
	flush_fifo();
	clear_fifo_flag();
	start_capture();
	while(!get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK)){;}
	length= read_fifo_length();


	CS_LOW();
	set_fifo_burst();

	for (uint32_t i = 0; i < length; i++)
	{
		HAL_SPI_Receive(&hspi1, &temp, 1, HAL_MAX_DELAY);
		HAL_UART_Transmit(&hlpuart1, &temp, 1, HAL_MAX_DELAY);
	}

	CS_HIGH();

}

