/*
 * ov5642_regs.h
 *
 *  Created on: Nov 28, 2025
 *      Author: fluffatron
 */

#ifndef INC_OV5642_REGS_H_
#define INC_OV5642_REGS_H_

#include <stdint.h>

#ifndef _SENSOR_
#define _SENSOR_
struct sensor_reg {
	uint16_t reg;
	uint16_t val;
};
#endif

extern const struct sensor_reg ov5642_320x240[];
extern const struct sensor_reg ov5642_640x480[];
extern const struct sensor_reg ov5642_1280x960[];
extern const struct sensor_reg ov5642_1600x1200[];
extern const struct sensor_reg ov5642_1024x768[];
extern const struct sensor_reg ov5642_2048x1536[];
extern const struct sensor_reg ov5642_2592x1944[];
extern const struct sensor_reg ov5642_dvp_zoom8[];
extern const struct sensor_reg OV5642_QVGA_Preview[];
extern const struct sensor_reg OV5642_JPEG_Capture_QSXGA[];
extern const struct sensor_reg OV5642_1080P_Video_setting[];
extern const struct sensor_reg OV5642_720P_Video_setting[];



#endif /* INC_OV5642_REGS_H_ */
