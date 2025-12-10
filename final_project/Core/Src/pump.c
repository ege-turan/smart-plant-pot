/*
 * pump.c
 *
 *  Created on: Dec 3, 2025
 *      Author: shuanlinchen
 */

#include "pump.h"

void pump_init(void) {
  // drive GPIO PB2 high, drive pump high for some period of time first (based
  // on tubing length) to fill the tubing
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
  HAL_Delay(2000);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
}

void pump_on(void) {
  // drive GPIO PB2 low
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
}

void pump_off(void) {
  // drive GPIO PB2 low
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
}
