/*
 * servo.c
 *
 *  Created on: Dec 2, 2025
 *      Author: shuanlinchen
 */


#include "servo.h"

#include "stdio.h"

/*
 * Using 32MHz Sysclk, default PSC + 1 = 640, default ARR + 1 = 1000
 * 5%: CCR = 50. 10%: CCR = 100
 */

void servo_init(void) {
	// default TIM4 channel3 as output
	// set CCR to be 500 for default 0° degree turn
	HAL_TIM_PWM_Init(&htim4);
	uint32_t pwmData = 500;
	HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_3, &pwmData, 1);
	printf("\rHello\n\r");
}

void servo_write_degree(uint32_t deg) {
	// convert degree to duty cycle
	uint32_t duty_cycle = deg * 50 / 180 + 50;
	printf("\rwrote degree: %d\n\r", deg);
	HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_3, &duty_cycle, 1);
	printf("\rwrote duty cycle: %d\n\r", duty_cycle);
}
