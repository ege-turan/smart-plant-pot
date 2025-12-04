/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

// std
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// display
#include "bigdisplay.h"
// touch
#include "touch.h"
// temp and humidity
#include "si7021.h"
// soil
#include "soil.h"
// light sensor
#include "lightsensor.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// idle = 1: idle and free to receive fast interrupts
int idle = 1;

// Tamagotchi feeling: 1 happy, 0 sad
int tamagotchi_feeling = 1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int __io_putchar(int ch) {
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ch, 1, 10);
  return ch;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_I2C3_Init();
  MX_I2C4_Init();
  MX_LPUART1_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USB_OTG_FS_USB_Init();
  /* USER CODE BEGIN 2 */

  printf("Hello from Nucleo-L4R5ZI-P!\r\n");

  TFT_Init();

  if (HAL_I2C_IsDeviceReady(&hi2c2, SOIL_ADDR, 3, 100) == HAL_OK)
    printf("Soil sensor detected\r\n");
  else
    printf("Soil sensor NOT detected\r\n");

  if (TOUCH_Init() != HAL_OK) {
    printf("Touch controller init FAILED\r\n");
  } else {
    printf("Touch controller init OK\r\n");
  }

  uint16_t read_value = 0;
  // initialize
  bh1750_init(BH1750_ADDR);

  float hum_air = 0.0f;
  float temp_air = 0.0f;
  uint16_t cap_soil = 0;
  float temp_soil = 0.0f;
  uint16_t light_value = 0;

  TFT_FillScreen(COLOR_WHITE);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    hum_air = si7021_read_humidity();
    temp_air = si7021_read_temperature();

    // Read soil sensor
    cap_soil = soil_read_capacitance();
    temp_soil = soil_read_temperature();
    int hum_air_int = (int)hum_air;
    int temp_air_int = (int)temp_air;
    int temp_soil_int = (int)temp_soil;
    int avg_temp = (temp_air_int + temp_soil_int) / 2;
    int avg_temp_f = (temp_air_int + temp_soil_int) / 2 * 9 / 5 + 32;

    light_value = bh1750_read(BH1750_ADDR);

    // print values of sensors
    printf("AirRH: %d %%  \r\n", hum_air_int);
    printf("AirTemp: %d C \r\n", temp_air_int);
    printf("SoilCap: %u  \r\n", cap_soil);
    printf("SoilTemp: %d C\r\n", temp_soil_int);
    printf("Light: %u  \r\n", light_value);

    int moisture_good = 1;
    if (cap_soil < 800) {
      moisture_good = 0;
    }

    int light_good = 1;
    if (light_value < 1000) {
      light_good = 0;
    }

    TFT_PrintfAt(50, 10, COLOR_BLACK, 3, "TAMAGOTCHI FLOWER POT");

    TFT_FillRect(120, 240, 60, 20, COLOR_WHITE);
    if (moisture_good) {
      TFT_PrintfAt(10, 240, COLOR_BLACK, 2, "Moisture: Hydrated  \r\n");
    } else {
      TFT_PrintfAt(10, 240, COLOR_BLACK, 2, "Moisture: Dry  \r\n");
    }

    TFT_FillRect(90, 260, 60, 20, COLOR_WHITE);
    if (light_good) {
      TFT_PrintfAt(10, 260, COLOR_BLACK, 2, "Light: Bright  \r\n");
    } else {
      TFT_PrintfAt(10, 260, COLOR_BLACK, 2, "Light: Dim  \r\n");
    }

    TFT_FillRect(120, 280, 60, 20, COLOR_WHITE);
    TFT_PrintfAt(10, 280, COLOR_BLACK, 2, "Humidity: %d%% \r\n", hum_air_int);

    TFT_FillRect(10, 300, 150, 20, COLOR_WHITE);
    TFT_PrintfAt(10, 300, COLOR_BLACK, 2, "%d C %d F \r\n", avg_temp,
                 avg_temp_f);

    if (tamagotchi_feeling == 1) {
      TFT_PrintfAt(10, 150, COLOR_BLACK, 2, "Plant is happy");
      // TFT_DrawSmiley(320, 150, 100, COLOR_BLACK, COLOR_WHITE, 1);
    } else {
      TFT_PrintfAt(10, 150, COLOR_BLACK, 2, "Plant is sad");
      // TFT_DrawSmiley(320, 150, 100, COLOR_BLACK, COLOR_WHITE, 1);
    }

    // if (TOUCH_HasNewData()) {
    //   TOUCH_TouchPoint tp;
    //   if (TOUCH_ReadTouch(&tp) == HAL_OK && tp.touched) {
    //     printf("Touch: x=%u y=%u\r\n", tp.x, tp.y);

    //     TFT_DrawPixel(tp.x, tp.y, COLOR_WHITE);
    //   }
    // }

    // read and print every second
    // HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
   */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
   */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
 * @brief Peripherals Common Clock Configuration
 * @retval None
 */
void PeriphCommonClock_Config(void) {
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the peripherals clock
   */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB | RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 24;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut =
      RCC_PLLSAI1_48M2CLK | RCC_PLLSAI1_ADC1CLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// touch callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

  if (idle == 0) {
    return;
  }
  if (GPIO_Pin == TOUCH_INT_Pin) {
    idle = 0;
    TOUCH_TouchPoint tp;
    if (TOUCH_ReadTouch(&tp) == HAL_OK && tp.touched) {
      printf("Touch: x=%u y=%u\r\n", tp.x, tp.y);
      TFT_DrawPixel(tp.x, tp.y, COLOR_WHITE);
    }
    idle = 1;
  }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
