#pragma once
#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===== Pins you wired =====
#define AIRLIFT_RST_GPIO_Port   GPIOG  // PG0 -> RST (active-low)
#define AIRLIFT_RST_Pin         GPIO_PIN_0
#define AIRLIFT_BUSY_GPIO_Port  GPIOG  // PG1 -> BUSY (input)
#define AIRLIFT_BUSY_Pin        GPIO_PIN_1

// ===== Tunables =====
#define AIRLIFT_MAX_FW_STR      64

typedef struct {
    SPI_HandleTypeDef *hspi;   // pass &hspi1 (HW NSS on PA4)
} airlift_t;

// High-level API
HAL_StatusTypeDef airlift_init(airlift_t *ctx, SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef airlift_get_fw_version(airlift_t *ctx, char *out, size_t out_len);
HAL_StatusTypeDef airlift_get_mac(airlift_t *ctx, uint8_t mac_out[6]);
HAL_StatusTypeDef airlift_scan(airlift_t *ctx, uint8_t *num_found);
HAL_StatusTypeDef airlift_connect(airlift_t *ctx, const char *ssid, const char *pass, uint32_t timeout_ms);
HAL_StatusTypeDef airlift_get_ip(airlift_t *ctx, uint8_t ip_out[4]);
HAL_StatusTypeDef airlift_http_get_print(airlift_t *ctx, const char *host, const char *path);

// Optional helpers
void  airlift_hw_reset(void);
bool  airlift_wait_ready(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
