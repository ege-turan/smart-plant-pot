#include "touch.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>

/* We will use I2C1 (global handle from CubeMX) */
extern I2C_HandleTypeDef hi2c1;

static I2C_HandleTypeDef *TOUCH_hi2c = NULL;
static volatile uint8_t TOUCH_new_data_flag = 0;

/* FT6206 register addresses */
#define TOUCH_REG_DEV_MODE 0x00
#define TOUCH_REG_GEST_ID 0x01
#define TOUCH_REG_TD_STATUS 0x02
#define TOUCH_REG_P1_XH 0x03
#define TOUCH_REG_P1_XL 0x04
#define TOUCH_REG_P1_YH 0x05
#define TOUCH_REG_P1_YL 0x06

// Optional: chip/vendor ID registers (may vary slightly between variants)
#define TOUCH_REG_VENDOR_ID 0xA8
#define TOUCH_REG_CHIP_ID 0xA3

static HAL_StatusTypeDef TOUCH_ReadReg(uint8_t reg, uint8_t *data,
                                       uint16_t len);
static void TOUCH_GPIO_Init(void);

HAL_StatusTypeDef TOUCH_Init(void) {
  TOUCH_hi2c = &hi2c1;

  TOUCH_GPIO_Init();

  HAL_GPIO_WritePin(TOUCH_RST_GPIO_Port, TOUCH_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(5);
  HAL_GPIO_WritePin(TOUCH_RST_GPIO_Port, TOUCH_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(50);

  uint8_t chip_id = 0;
  HAL_StatusTypeDef st;

  st = TOUCH_ReadReg(TOUCH_REG_CHIP_ID, &chip_id, 1);
  if (st != HAL_OK) {
    printf("[TOUCH][ERR] Failed to read CHIP_ID (status=%d)\r\n", (int)st);
    return st;
  }

  printf("[TOUCH] Chip ID: 0x%02X\r\n", chip_id);

  return HAL_OK;
}

HAL_StatusTypeDef TOUCH_ReadTouch(TOUCH_TouchPoint *p) {
  if (!p) {
    return HAL_ERROR;
  }
  if (!TOUCH_hi2c) {
    p->touched = 0;
    return HAL_ERROR;
  }

  uint8_t buf[5];
  HAL_StatusTypeDef st;

  st = TOUCH_ReadReg(TOUCH_REG_TD_STATUS, buf, sizeof(buf));
  if (st != HAL_OK) {
    p->touched = 0;
    return st;
  }

  uint8_t touches = buf[0] & 0x0F;

  if (touches == 0) {
    p->touched = 0;
    p->x = 0;
    p->y = 0;
  } else {
    uint16_t x = ((uint16_t)(buf[1] & 0x0F) << 8) | buf[2];
    uint16_t y = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[4];

    p->x = y;
    p->y = 320 - x;
    p->touched = 1;
  }
  return HAL_OK;
}

uint8_t TOUCH_HasNewData(void) { return TOUCH_new_data_flag; }

static void TOUCH_GPIO_Init(void) {
  GPIO_InitTypeDef gi = {0};

  /* Enable GPIOF clock (if not already) */
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /* PF7: RST - push-pull output, no pull, low/med speed */
  gi.Pin = TOUCH_RST_Pin;
  gi.Mode = GPIO_MODE_OUTPUT_PP;
  gi.Pull = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TOUCH_RST_GPIO_Port, &gi);

  /* Default RST high (inactive) */
  HAL_GPIO_WritePin(TOUCH_RST_GPIO_Port, TOUCH_RST_Pin, GPIO_PIN_SET);

  /* PF9: INT - input with EXTI on falling edge, pull-up */
  gi.Pin = TOUCH_INT_Pin;
  gi.Mode = GPIO_MODE_IT_FALLING;
  gi.Pull = GPIO_PULLUP;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TOUCH_INT_GPIO_Port, &gi);

  /* Enable EXTI line interrupt for PF9 (EXTI9_5_IRQn for pins 5..9) */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

static HAL_StatusTypeDef TOUCH_ReadReg(uint8_t reg, uint8_t *data,
                                       uint16_t len) {
  if (!TOUCH_hi2c) {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef st =
      HAL_I2C_Mem_Read(TOUCH_hi2c, TOUCH_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                       data, len, HAL_MAX_DELAY);

  if (st != HAL_OK) {
    printf("[TOUCH][ERR] I2C Mem Read fail: reg=0x%02X, status=%d\r\n", reg,
           (int)st);
  }
  return st;
}
