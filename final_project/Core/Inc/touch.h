#ifndef TOUCH_H
#define TOUCH_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

#define TOUCH_I2C_ADDR         (0x38u << 1)

#define TOUCH_RST_Pin GPIO_PIN_7
#define TOUCH_RST_GPIO_Port GPIOF

#define TOUCH_INT_Pin GPIO_PIN_9
#define TOUCH_INT_GPIO_Port GPIOF
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint16_t x;
  uint16_t y;
  uint8_t touched;
} TOUCH_TouchPoint;

HAL_StatusTypeDef TOUCH_Init(void);

HAL_StatusTypeDef TOUCH_ReadTouch(TOUCH_TouchPoint *p);

uint8_t TOUCH_HasNewData(void);


#ifdef __cplusplus
}
#endif

#endif /* TOUCH_H */
