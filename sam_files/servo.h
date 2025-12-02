/*
 * servo.h
 *
 *  Created on: Dec 2, 2025
 *      Author: shuanlinchen
 */

#ifndef SRC_SERVO_H_
#define SRC_SERVO_H_

#include "tim.h"

void servo_init(void);

void servo_write_degree(uint32_t deg);


#endif /* SRC_SERVO_H_ */
