// airlift_unified.c
// One-call "send + receive" SPI command wrapper for ESP32 AirLift/NINA firmware

#include "airlift.h"
#include <stdio.h>
#include <string.h>

#define START_CMD 0xE0
#define END_CMD 0xEE
#define ERR_CMD  0xEF
#define REPLY_FLAG 0x80
#define DATA_FLAG  0x40

#define SET_NET_CMD            0x10
#define SET_PASSPHRASE_CMD     0x11

#define GET_CONN_STATUS_CMD    0x20
#define GET_IPADDR_CMD         0x21
#define GET_MACADDR_CMD        0x22

#define SCAN_NETWORKS          0x27
#define GET_IDX_RSSI_CMD       0x32
#define GET_IDX_ENCT_CMD       0x33
#define GET_IDX_BSSID_CMD      0x3C
#define GET_IDX_CHAN_CMD       0x3D

#define START_SCAN_NETWORKS    0x36
#define GET_FW_VERSION_CMD     0x37

#define WL_IDLE_STATUS         0
#define WL_NO_SSID_AVAIL       1
#define WL_SCAN_COMPLETED      2
#define WL_CONNECTED           3
#define WL_CONNECT_FAILED      4
#define WL_CONNECTION_LOST     5
#define WL_DISCONNECTED        6

#define SPI_TIMEOUT_MS             1000
#define AIRLIFT_BOOT_TIME_MS        750
#define DEFAULT_READY_WAIT_MS      2000
#define DEFAULT_START_BYTE_TIMEOUT_MS 1000

#define SCAN_READY_WAIT_MS   8000
#define SCAN_START_BYTE_TIMEOUT_MS 2000
#define READY_AFTER_CS_MS    1000
#define STATUS_READY_WAIT_MS 8000

#define SET_ENT_IDENT_CMD    0x4A
#define SET_ENT_UNAME_CMD    0x4B
#define SET_ENT_PASSWD_CMD   0x4C
#define SET_ENT_ENABLE_CMD   0x4F

#define START_CLIENT_TCP_CMD 0x2D
#define STOP_CLIENT_TCP_CMD  0x2E
#define GET_CLIENT_STATE_TCP_CMD 0x2F
#define DATA_SENT_TCP_CMD    0x2A
#define AVAIL_DATA_TCP_CMD   0x2B
#define GET_DATA_TCP_CMD     0x2C
#define GET_SOCKET_CMD       0x3F
#define SEND_DATA_TCP_CMD    0x44
#define GET_DATABUF_TCP_CMD  0x45
#define REQ_HOST_BY_NAME_CMD 0x34
#define GET_HOST_BY_NAME_CMD 0x35

#define CMD_PAYLOAD_ID(x) ((uint8_t)((x)&0x7F))

// -----------------------------------------------------------------------------
// Low-level helpers
// -----------------------------------------------------------------------------

static const char *hal_str(HAL_StatusTypeDef st) {
  switch (st) {
  case HAL_OK:      return "HAL_OK";
  case HAL_ERROR:   return "HAL_ERROR";
  case HAL_BUSY:    return "HAL_BUSY";
  case HAL_TIMEOUT: return "HAL_TIMEOUT";
  default:          return "HAL_???";
  }
}

static inline int busy_is_high(void) {
  return HAL_GPIO_ReadPin(AIRLIFT_BUSY_GPIO_Port, AIRLIFT_BUSY_Pin) == GPIO_PIN_SET;
}

static bool wait_busy_low(uint32_t timeout_ms) {
  uint32_t t0 = HAL_GetTick();
  while (busy_is_high()) {
    if (HAL_GetTick() - t0 > timeout_ms) return false;
  }
  return true;
}

static inline void cs_assert(airlift_t *ctx) {
  __HAL_SPI_ENABLE(ctx->hspi);
  for (volatile int i = 0; i < 80; i++) { }
}

static inline void cs_deassert(airlift_t *ctx) {
  while (__HAL_SPI_GET_FLAG(ctx->hspi, SPI_FLAG_BSY)) { }
  __HAL_SPI_DISABLE(ctx->hspi);
  for (volatile int i = 0; i < 80; i++) { }
}

static HAL_StatusTypeDef select_phase(airlift_t *ctx, const char *name, uint32_t ready_wait_ms) {
  if (!wait_busy_low(ready_wait_ms)) {
    printf("[Airlift] %s: BUSY stayed HIGH (not ready)\r\n", name);
    return HAL_TIMEOUT;
  }

  __HAL_SPI_ENABLE(ctx->hspi);

  uint32_t t1 = HAL_GetTick();
  while (!busy_is_high()) {
    if (HAL_GetTick() - t1 > READY_AFTER_CS_MS) {
      __HAL_SPI_DISABLE(ctx->hspi);
      printf("[Airlift] %s: timeout waiting BUSY HIGH after CS\r\n", name);
      return HAL_TIMEOUT;
    }
  }

  for (volatile int i = 0; i < 200; i++) { }
  return HAL_OK;
}

static HAL_StatusTypeDef write_bytes(airlift_t *ctx, const uint8_t *buf, uint16_t len) {
  return HAL_SPI_Transmit(ctx->hspi, (uint8_t *)buf, len, SPI_TIMEOUT_MS);
}

static HAL_StatusTypeDef read_bytes(airlift_t *ctx, uint8_t *buf, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    uint8_t tx = 0xFF, rx = 0;
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(ctx->hspi, &tx, &rx, 1, SPI_TIMEOUT_MS);
    if (st != HAL_OK) return st;
    buf[i] = rx;
  }
  return HAL_OK;
}

// Pad send frame to 4-byte multiple (write zeros)
static HAL_StatusTypeDef pad_to_4_send(airlift_t *ctx, uint32_t bytes_sent) {
  uint8_t need = (4 - (bytes_sent & 3)) & 3;
  if (!need) return HAL_OK;
  static const uint8_t z[3] = {0};
  return write_bytes(ctx, z, need);
}

// -----------------------------------------------------------------------------
// Unified "send + receive" xfer primitives
// -----------------------------------------------------------------------------

/**
 * airlift_cmd_xfer
 *
 * Sends a command with parameters, then (in the same function call) waits for and
 * receives the reply frame. The caller can optionally capture the first reply parameter
 * into reply_payload/reply_len_io; all additional parameters are drained.
 *
 * - tx_len16: when true, 16-bit parameter lengths are used for sending
 * - rx_len16: when true, 16-bit parameter lengths are expected in the reply
 * - ready_wait_ms: max time to wait for BUSY=LOW before each CS phase
 * - start_byte_timeout_ms: max time to wait for START_CMD byte during reply
 *
 * Optionally returns nparams in out_reply_params (may be NULL).
 */
static HAL_StatusTypeDef airlift_cmd_xfer(airlift_t *ctx,
                                          uint8_t cmd,
                                          const uint8_t **params,
                                          const uint16_t *plen,
                                          uint8_t nparams,
                                          uint8_t *reply_payload,
                                          uint16_t *reply_len_io,
                                          bool tx_len16,
                                          bool rx_len16,
                                          uint32_t ready_wait_ms,
                                          uint32_t start_byte_timeout_ms,
                                          uint8_t *out_reply_params) {
  // ---------------- SEND PHASE ----------------
  char phase_send[32];
  snprintf(phase_send, sizeof(phase_send), "xfer send 0x%02X", cmd);
  HAL_StatusTypeDef st = select_phase(ctx, phase_send, ready_wait_ms);
  if (st != HAL_OK) return st;

  uint8_t hdr[3] = {START_CMD, (uint8_t)(cmd & ~REPLY_FLAG), nparams};
  st = write_bytes(ctx, hdr, sizeof(hdr));
  if (st != HAL_OK) { cs_deassert(ctx); return st; }

  uint32_t bytes_sent = sizeof(hdr);

  for (uint8_t i = 0; i < nparams; i++) {
    uint16_t L = plen ? plen[i] : 0;
    if (tx_len16) {
      uint8_t hi = (uint8_t)((L >> 8) & 0xFF);
      uint8_t lo = (uint8_t)(L & 0xFF);
      st = write_bytes(ctx, &hi, 1); if (st != HAL_OK) { cs_deassert(ctx); return st; }
      st = write_bytes(ctx, &lo, 1); if (st != HAL_OK) { cs_deassert(ctx); return st; }
      bytes_sent += 2;
    } else {
      uint8_t L8 = (uint8_t)(L & 0xFF);
      st = write_bytes(ctx, &L8, 1); if (st != HAL_OK) { cs_deassert(ctx); return st; }
      bytes_sent += 1;
    }

    if (L) {
      st = write_bytes(ctx, params[i], L);
      if (st != HAL_OK) { cs_deassert(ctx); return st; }
      bytes_sent += L;
    }
  }

  uint8_t endb = END_CMD;
  st = write_bytes(ctx, &endb, 1);
  if (st != HAL_OK) { cs_deassert(ctx); return st; }
  bytes_sent += 1;

  st = pad_to_4_send(ctx, bytes_sent);
  if (st != HAL_OK) { cs_deassert(ctx); return st; }

  cs_deassert(ctx);

  // ---------------- RECV PHASE ----------------
  char phase_recv[32];
  snprintf(phase_recv, sizeof(phase_recv), "xfer recv 0x%02X", cmd);
  st = select_phase(ctx, phase_recv, ready_wait_ms);
  if (st != HAL_OK) return st;

  uint8_t b = 0;
  uint32_t t0 = HAL_GetTick();
  do {
    st = read_bytes(ctx, &b, 1);
    if (st != HAL_OK) { cs_deassert(ctx); return st; }
    if (b == ERR_CMD)  { cs_deassert(ctx); printf("[Airlift] xfer: ERR_CMD\r\n"); return HAL_ERROR; }
    if ((HAL_GetTick() - t0) > start_byte_timeout_ms) {
      cs_deassert(ctx);
      printf("[Airlift] xfer: timeout waiting START\r\n");
      return HAL_TIMEOUT;
    }
  } while (b != START_CMD);

  uint8_t rcmd = 0, rparams = 0;
  st = read_bytes(ctx, &rcmd, 1); if (st != HAL_OK) { cs_deassert(ctx); return st; }
  st = read_bytes(ctx, &rparams, 1); if (st != HAL_OK) { cs_deassert(ctx); return st; }

  if ((rcmd & REPLY_FLAG) == 0) {
    printf("[Airlift] xfer: warning, reply without REPLY_FLAG (cmd=0x%02X)\r\n", rcmd);
  }
  uint8_t payload_cmd = CMD_PAYLOAD_ID(rcmd);
  if (payload_cmd != cmd) {
    printf("[Airlift] xfer: unexpected reply cmd 0x%02X (expected 0x%02X)\r\n", payload_cmd, cmd);
  }
  if (out_reply_params) *out_reply_params = rparams;

  // Read parameters; copy only the first one if asked
  uint16_t out_written = 0;
  for (uint8_t i = 0; i < rparams; i++) {
    uint16_t L = 0;
    if (rx_len16) {
      uint8_t hi = 0, lo = 0;
      if (read_bytes(ctx, &hi, 1) != HAL_OK) { cs_deassert(ctx); return HAL_ERROR; }
      if (read_bytes(ctx, &lo, 1) != HAL_OK) { cs_deassert(ctx); return HAL_ERROR; }
      L = ((uint16_t)hi << 8) | lo;
    } else {
      uint8_t L8 = 0;
      if (read_bytes(ctx, &L8, 1) != HAL_OK) { cs_deassert(ctx); return HAL_ERROR; }
      L = L8;
    }

    if (i == 0 && reply_payload && reply_len_io && *reply_len_io > 0) {
      uint16_t to_copy = (L <= *reply_len_io) ? L : *reply_len_io;
      if (to_copy) {
        st = read_bytes(ctx, reply_payload, to_copy);
        if (st != HAL_OK) { cs_deassert(ctx); return st; }
        out_written = to_copy;
      }
      // Drain remainder if any
      uint16_t rem = (L > to_copy) ? (L - to_copy) : 0;
      uint8_t dump[32];
      while (rem) {
        uint16_t ch = (rem > sizeof(dump)) ? sizeof(dump) : rem;
        st = read_bytes(ctx, dump, ch);
        if (st != HAL_OK) { cs_deassert(ctx); return st; }
        rem -= ch;
      }
    } else {
      // Drain full parameter
      uint8_t dump[32];
      uint16_t rem = L;
      while (rem) {
        uint16_t ch = (rem > sizeof(dump)) ? sizeof(dump) : rem;
        st = read_bytes(ctx, dump, ch);
        if (st != HAL_OK) { cs_deassert(ctx); return st; }
        rem -= ch;
      }
    }
  }

  // Read END marker (skip any potential pad or noise until END_CMD)
  do {
    st = read_bytes(ctx, &b, 1);
    if (st != HAL_OK) { cs_deassert(ctx); return st; }
  } while (b != END_CMD);

  cs_deassert(ctx);
  if (reply_len_io) *reply_len_io = out_written;
  return HAL_OK;
}

/**
 * airlift_cmd_xfer_iter
 *
 * Same as airlift_cmd_xfer, but instead of copying only the first reply param,
 * it invokes a per-parameter callback (once per parameter).
 * Use for commands like SCAN_NETWORKS that return many small parameters (SSIDs).
 */
typedef HAL_StatusTypeDef (*airlift_param_cb_t)(void *user, uint8_t index,
                                                const uint8_t *data, uint16_t len);

static HAL_StatusTypeDef airlift_cmd_xfer_iter(airlift_t *ctx,
                                               uint8_t cmd,
                                               const uint8_t **params,
                                               const uint16_t *plen,
                                               uint8_t nparams,
                                               airlift_param_cb_t cb,
                                               void *user,
                                               bool tx_len16,
                                               bool rx_len16,
                                               uint32_t ready_wait_ms,
                                               uint32_t start_byte_timeout_ms,
                                               uint8_t *out_reply_params) {
  // Send (shared with airlift_cmd_xfer)
  char phase_send[32];
  snprintf(phase_send, sizeof(phase_send), "xfer(send) 0x%02X", cmd);
  HAL_StatusTypeDef st = select_phase(ctx, phase_send, ready_wait_ms);
  if (st != HAL_OK) return st;

  uint8_t hdr[3] = {START_CMD, (uint8_t)(cmd & ~REPLY_FLAG), nparams};
  st = write_bytes(ctx, hdr, sizeof(hdr));
  if (st != HAL_OK) { cs_deassert(ctx); return st; }

  uint32_t bytes_sent = sizeof(hdr);
  for (uint8_t i = 0; i < nparams; i++) {
    uint16_t L = plen ? plen[i] : 0;
    if (tx_len16) {
      uint8_t hi = (uint8_t)((L >> 8) & 0xFF);
      uint8_t lo = (uint8_t)(L & 0xFF);
      if ((st = write_bytes(ctx, &hi, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
      if ((st = write_bytes(ctx, &lo, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
      bytes_sent += 2;
    } else {
      uint8_t L8 = (uint8_t)(L & 0xFF);
      if ((st = write_bytes(ctx, &L8, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
      bytes_sent += 1;
    }

    if (L) {
      if ((st = write_bytes(ctx, params[i], L)) != HAL_OK) { cs_deassert(ctx); return st; }
      bytes_sent += L;
    }
  }

  uint8_t endb = END_CMD;
  if ((st = write_bytes(ctx, &endb, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
  bytes_sent++;
  if ((st = pad_to_4_send(ctx, bytes_sent)) != HAL_OK) { cs_deassert(ctx); return st; }
  cs_deassert(ctx);

  // Receive (parse all params and callback)
  char phase_recv[32];
  snprintf(phase_recv, sizeof(phase_recv), "xfer(recv) 0x%02X", cmd);
  st = select_phase(ctx, phase_recv, ready_wait_ms);
  if (st != HAL_OK) return st;

  uint8_t b = 0;
  uint32_t t0 = HAL_GetTick();
  do {
    st = read_bytes(ctx, &b, 1);
    if (st != HAL_OK) { cs_deassert(ctx); return st; }
    if (b == ERR_CMD)  { cs_deassert(ctx); return HAL_ERROR; }
    if ((HAL_GetTick() - t0) > start_byte_timeout_ms) {
      cs_deassert(ctx);
      return HAL_TIMEOUT;
    }
  } while (b != START_CMD);

  uint8_t rcmd = 0, rparams = 0;
  if ((st = read_bytes(ctx, &rcmd, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
  if ((st = read_bytes(ctx, &rparams, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
  if (out_reply_params) *out_reply_params = rparams;

  if (CMD_PAYLOAD_ID(rcmd) != cmd) {
    printf("[Airlift] iter: unexpected cmd 0x%02X (exp 0x%02X)\r\n", rcmd, cmd);
  }

  uint8_t scratch[256];
  for (uint8_t i = 0; i < rparams; i++) {
    uint16_t L = 0;
    if (rx_len16) {
      uint8_t hi=0, lo=0;
      if ((st = read_bytes(ctx, &hi, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
      if ((st = read_bytes(ctx, &lo, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
      L = ((uint16_t)hi << 8) | lo;
    } else {
      uint8_t L8 = 0;
      if ((st = read_bytes(ctx, &L8, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
      L = L8;
    }

    if (L == 0) {
      if (cb) cb(user, i, NULL, 0);
      continue;
    }

    if (L <= sizeof(scratch)) {
      if ((st = read_bytes(ctx, scratch, (uint16_t)L)) != HAL_OK) { cs_deassert(ctx); return st; }
      if (cb) {
        st = cb(user, i, scratch, (uint16_t)L);
        if (st != HAL_OK) { cs_deassert(ctx); return st; }
      }
    } else {
      // Drain too-large parameter without callback to avoid heap usage
      uint32_t rem = L;
      while (rem) {
        uint16_t ch = (rem > sizeof(scratch)) ? sizeof(scratch) : (uint16_t)rem;
        if ((st = read_bytes(ctx, scratch, ch)) != HAL_OK) { cs_deassert(ctx); return st; }
        rem -= ch;
      }
    }
  }

  do {
    if ((st = read_bytes(ctx, &b, 1)) != HAL_OK) { cs_deassert(ctx); return st; }
  } while (b != END_CMD);

  cs_deassert(ctx);
  return HAL_OK;
}

// Convenience wrappers

static HAL_StatusTypeDef airlift_cmd_simple(airlift_t *ctx, uint8_t cmd,
                                            const uint8_t **params, const uint16_t *plen, uint8_t nparams,
                                            uint8_t *reply, uint16_t *reply_len) {
  return airlift_cmd_xfer(ctx, cmd, params, plen, nparams,
                          reply, reply_len,
                          false, false,
                          DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS,
                          NULL);
}

static HAL_StatusTypeDef airlift_cmd_ack_only(airlift_t *ctx, uint8_t cmd,
                                              const uint8_t **params, const uint16_t *plen, uint8_t nparams,
                                              uint32_t ready_wait_ms, uint32_t start_byte_timeout_ms) {
  uint8_t ack = 0;
  uint16_t n = 1;
  HAL_StatusTypeDef st = airlift_cmd_xfer(ctx, cmd, params, plen, nparams,
                                          &ack, &n,
                                          false, false,
                                          ready_wait_ms, start_byte_timeout_ms,
                                          NULL);
  if (st != HAL_OK) return st;
  if (n < 1 || ack != 1) {
    printf("[Airlift] cmd 0x%02X: bad ack %u\r\n", cmd, ack);
    return HAL_ERROR;
  }
  return HAL_OK;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void airlift_hw_reset(void) {
  HAL_GPIO_WritePin(AIRLIFT_RST_GPIO_Port, AIRLIFT_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(AIRLIFT_RST_GPIO_Port, AIRLIFT_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(AIRLIFT_BOOT_TIME_MS);
}

bool airlift_wait_ready(uint32_t timeout_ms) {
  return wait_busy_low(timeout_ms);
}

HAL_StatusTypeDef airlift_init(airlift_t *ctx, SPI_HandleTypeDef *hspi) {
  if (!ctx || !hspi) {
    printf("[Airlift] init: bad args\r\n");
    return HAL_ERROR;
  }
  ctx->hspi = hspi;

  __HAL_RCC_GPIOG_CLK_ENABLE();
  GPIO_InitTypeDef gi = {0};

  gi.Pin = AIRLIFT_RST_Pin;
  gi.Mode = GPIO_MODE_OUTPUT_PP;
  gi.Pull = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(AIRLIFT_RST_GPIO_Port, &gi);
  HAL_GPIO_WritePin(AIRLIFT_RST_GPIO_Port, AIRLIFT_RST_Pin, GPIO_PIN_SET);

  gi.Pin = AIRLIFT_BUSY_Pin;
  gi.Mode = GPIO_MODE_INPUT;
  gi.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(AIRLIFT_BUSY_GPIO_Port, &gi);

  __HAL_SPI_DISABLE(ctx->hspi);
  airlift_hw_reset();

  char fw[AIRLIFT_MAX_FW_STR] = {0};
  HAL_StatusTypeDef st = airlift_get_fw_version(ctx, fw, sizeof(fw));
  if (st != HAL_OK) {
    printf("[Airlift] init: failed to read FW version\r\n");
    return st;
  }
  printf("[Airlift] FW: %s\r\n", fw);

  uint8_t mac[6];
  st = airlift_get_mac(ctx, mac);
  if (st == HAL_OK) {
    printf("[Airlift] MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  return HAL_OK;
}

HAL_StatusTypeDef airlift_get_fw_version(airlift_t *ctx, char *out, size_t out_len) {
  if (!out || out_len < 2) {
    printf("[Airlift] get_fw: small buffer\r\n");
    return HAL_ERROR;
  }
  uint8_t buf[64] = {0};
  uint16_t n = sizeof(buf);
  HAL_StatusTypeDef st = airlift_cmd_simple(ctx, GET_FW_VERSION_CMD, NULL, NULL, 0, buf, &n);
  if (st != HAL_OK) {
    printf("[Airlift] get_fw: xfer failed -> %s\r\n", hal_str(st));
    return st;
  }
  if (n == 0) {
    printf("[Airlift] get_fw: empty payload\r\n");
    return HAL_ERROR;
  }
  size_t cpy = (n < out_len - 1) ? n : (out_len - 1);
  memcpy(out, buf, cpy);
  out[cpy] = 0;
  return HAL_OK;
}

HAL_StatusTypeDef airlift_get_mac(airlift_t *ctx, uint8_t mac_out[6]) {
  if (!mac_out) return HAL_ERROR;
  const uint8_t *pp[1] = { (const uint8_t *)"\xFF" };
  uint16_t ll[1] = { 1 };
  uint8_t buf[6] = {0};
  uint16_t n = sizeof(buf);
  HAL_StatusTypeDef st = airlift_cmd_simple(ctx, GET_MACADDR_CMD, pp, ll, 1, buf, &n);
  if (st != HAL_OK || n < 6) {
    printf("[Airlift] get_mac: xfer failed (st=%s, n=%u)\r\n", hal_str(st), n);
    return (st == HAL_OK) ? HAL_ERROR : st;
  }
  memcpy(mac_out, buf, 6);
  return HAL_OK;
}

static HAL_StatusTypeDef get_conn_status(airlift_t *ctx, uint8_t *status_out) {
  if (!status_out) return HAL_ERROR;
  uint8_t s = 0;
  uint16_t n = 1;
  HAL_StatusTypeDef st = airlift_cmd_simple(ctx, GET_CONN_STATUS_CMD, NULL, NULL, 0, &s, &n);
  if (st != HAL_OK || n < 1) return (st == HAL_OK) ? HAL_ERROR : st;
  *status_out = s;
  return HAL_OK;
}

static HAL_StatusTypeDef get_conn_status_roundtrip(airlift_t *ctx, uint8_t *status_out) {
  if (!status_out) return HAL_ERROR;
  uint8_t s = 0;
  uint16_t n = 1;
  HAL_StatusTypeDef st = airlift_cmd_xfer(ctx, GET_CONN_STATUS_CMD,
                                          NULL, NULL, 0,
                                          &s, &n,
                                          false, false,
                                          STATUS_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS,
                                          NULL);
  if (st != HAL_OK || n < 1) return (st == HAL_OK) ? HAL_ERROR : st;
  *status_out = s;
  return HAL_OK;
}

HAL_StatusTypeDef airlift_get_ip(airlift_t *ctx, uint8_t ip_out[4]) {
  if (!ip_out) return HAL_ERROR;
  const uint8_t *pp[1] = { (const uint8_t *)"\xFF" };
  uint16_t ll[1] = { 1 };
  uint8_t buf[4] = {0};
  uint16_t n = sizeof(buf);
  HAL_StatusTypeDef st = airlift_cmd_simple(ctx, GET_IPADDR_CMD, pp, ll, 1, buf, &n);
  if (st != HAL_OK) { printf("[Airlift] get_ip: xfer failed\r\n"); return st; }
  if (n < 4)        { printf("[Airlift] get_ip: short reply (%u)\r\n", n); return HAL_ERROR; }
  memcpy(ip_out, buf, 4);
  printf("[Airlift] IP: %u.%u.%u.%u\r\n", ip_out[0], ip_out[1], ip_out[2], ip_out[3]);
  return HAL_OK;
}

HAL_StatusTypeDef airlift_set_ent_identity(airlift_t *ctx, const char *ident) {
  const uint8_t *pp[1] = { (const uint8_t *)(ident ? ident : "") };
  uint16_t ll[1] = { (uint16_t)(ident ? strlen(ident) : 0) };
  return airlift_cmd_ack_only(ctx, SET_ENT_IDENT_CMD, pp, ll, 1,
                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
}

HAL_StatusTypeDef airlift_set_ent_username(airlift_t *ctx, const char *uname) {
  if (!uname) return HAL_ERROR;
  uint16_t len = (uint16_t)strlen(uname);
  if (len > 255) { printf("[Airlift] username too long (%u)\r\n", len); return HAL_ERROR; }
  const uint8_t *pp[1] = { (const uint8_t *)uname };
  uint16_t ll[1] = { len };
  return airlift_cmd_ack_only(ctx, SET_ENT_UNAME_CMD, pp, ll, 1,
                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
}

HAL_StatusTypeDef airlift_set_ent_password(airlift_t *ctx, const char *pass) {
  if (!pass) return HAL_ERROR;
  uint16_t len = (uint16_t)strlen(pass);
  if (len > 255) { printf("[Airlift] password too long (%u)\r\n", len); return HAL_ERROR; }
  const uint8_t *pp[1] = { (const uint8_t *)pass };
  uint16_t ll[1] = { len };
  return airlift_cmd_ack_only(ctx, SET_ENT_PASSWD_CMD, pp, ll, 1,
                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
}

HAL_StatusTypeDef airlift_enable_enterprise(airlift_t *ctx) {
  return airlift_cmd_ack_only(ctx, SET_ENT_ENABLE_CMD, NULL, NULL, 0,
                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
}

// ---------------- Scan helper using iter xfer ----------------

typedef struct {
  uint8_t **ssids;
  uint16_t *ssid_lens;  // IN: capacity; OUT: bytes copied
  uint8_t max_slots;
} ssid_cb_ctx_t;

static HAL_StatusTypeDef ssid_collect_cb(void *user, uint8_t index,
                                         const uint8_t *data, uint16_t len) {
  ssid_cb_ctx_t *s = (ssid_cb_ctx_t *)user;
  if (!s || !s->ssids || !s->ssid_lens) return HAL_OK;
  if (index >= s->max_slots) {
    // Ignore extras; they will still be drained by the caller
    return HAL_OK;
  }
  uint8_t *dst = s->ssids[index];
  uint16_t cap = s->ssid_lens[index];
  uint16_t to_copy = (len <= cap) ? len : cap;
  if (dst && to_copy) memcpy(dst, data, to_copy);
  s->ssid_lens[index] = to_copy; // report how many were actually written
  return HAL_OK;
}

static HAL_StatusTypeDef airlift_scan_recv_list(airlift_t *ctx,
                                                uint8_t **ssids,
                                                uint16_t *ssid_lens,
                                                uint8_t max_slots,
                                                uint8_t *out_count) {
  ssid_cb_ctx_t cbctx = { ssids, ssid_lens, max_slots };
  uint8_t count = 0;
  HAL_StatusTypeDef st = airlift_cmd_xfer_iter(ctx, SCAN_NETWORKS,
                                               NULL, NULL, 0,
                                               ssid_collect_cb, &cbctx,
                                               false, false,
                                               SCAN_READY_WAIT_MS, SCAN_START_BYTE_TIMEOUT_MS,
                                               &count);
  if (st != HAL_OK) return st;
  if (out_count) *out_count = count;
  return HAL_OK;
}

HAL_StatusTypeDef airlift_scan(airlift_t *ctx, uint8_t *num_found) {
  // Kick scan
  HAL_StatusTypeDef st = airlift_cmd_ack_only(ctx, START_SCAN_NETWORKS, NULL, NULL, 0,
                                              SCAN_READY_WAIT_MS, SCAN_START_BYTE_TIMEOUT_MS);
  if (st != HAL_OK) {
    printf("[Airlift] scan: start failed -> %s\r\n", hal_str(st));
    return st;
  }

  const uint32_t SCAN_TIMEOUT_MS = 15000;
  uint32_t t0 = HAL_GetTick();

  enum { MAX_SSIDS = 20, MAX_SSID_LEN = 33 };
  uint8_t *ssid_ptrs[MAX_SSIDS];
  uint16_t ssid_lens[MAX_SSIDS];
  uint8_t ssid_bufs[MAX_SSIDS][MAX_SSID_LEN];

  for (int i = 0; i < MAX_SSIDS; i++) {
    ssid_ptrs[i] = ssid_bufs[i];
    ssid_lens[i] = MAX_SSID_LEN - 1; // capacity for copy (we'll add NUL after)
  }

  uint8_t N = 0;

  do {
    N = 0;
    st = airlift_scan_recv_list(ctx, ssid_ptrs, ssid_lens, MAX_SSIDS, &N);
    printf("[airlift_scan] recv list -> %s, count=%u\r\n", hal_str(st), N);

    if (st == HAL_TIMEOUT) {
      HAL_Delay(250);
      continue;
    }
    if (st != HAL_OK) {
      return st;
    }

    if (N > 0) break;
    HAL_Delay(250);
  } while ((HAL_GetTick() - t0) < SCAN_TIMEOUT_MS);

  if (num_found) *num_found = N;
  printf("[Airlift] scan: %u networks\r\n", N);
  if (N == 0) return HAL_OK;

  // NUL-terminate for printing
  for (uint8_t i = 0; i < N && i < MAX_SSIDS; i++) {
    uint16_t c = (ssid_lens[i] < (MAX_SSID_LEN - 1)) ? ssid_lens[i] : (MAX_SSID_LEN - 1);
    ssid_bufs[i][c] = 0;
  }

  // Print extended info for each SSID
  for (uint8_t i = 0; i < N && i < MAX_SSIDS; i++) {
    const uint8_t *pp[1] = { &i };
    uint16_t ll[1] = { 1 };

    // RSSI
    uint8_t rssi_raw = 0;
    uint16_t n = 1;
    st = airlift_cmd_simple(ctx, GET_IDX_RSSI_CMD, pp, ll, 1, &rssi_raw, &n);
    if (st != HAL_OK) return st;
    int8_t rssi = (int8_t)rssi_raw;

    // CHAN
    uint8_t chan = 0;
    n = 1;
    st = airlift_cmd_simple(ctx, GET_IDX_CHAN_CMD, pp, ll, 1, &chan, &n);
    if (st != HAL_OK) return st;

    // ENC
    uint8_t enct = 0;
    n = 1;
    st = airlift_cmd_simple(ctx, GET_IDX_ENCT_CMD, pp, ll, 1, &enct, &n);
    if (st != HAL_OK) return st;

    // BSSID
    uint8_t bssid[6] = {0};
    n = sizeof(bssid);
    st = airlift_cmd_simple(ctx, GET_IDX_BSSID_CMD, pp, ll, 1, bssid, &n);
    if (st != HAL_OK || n < 6) return HAL_ERROR;

    printf("  %2u: %.*s  RSSI %d dBm  chan %u  enc %u  BSSID %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           (unsigned)(i + 1), (int)ssid_lens[i], (char *)ssid_ptrs[i],
           (int)rssi, chan, enct, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  }

  return HAL_OK;
}

HAL_StatusTypeDef airlift_connect(airlift_t *ctx, const char *ssid,
                                  const char *pass, uint32_t timeout_ms) {
  if (!ssid) { printf("[Airlift] connect: null SSID\r\n"); return HAL_ERROR; }

  HAL_StatusTypeDef st;
  if (pass && pass[0]) {
    const uint8_t *pp[2] = { (const uint8_t *)ssid, (const uint8_t *)pass };
    uint16_t ll[2] = { (uint16_t)strlen(ssid), (uint16_t)strlen(pass) };
    st = airlift_cmd_ack_only(ctx, SET_PASSPHRASE_CMD, pp, ll, 2,
                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
    if (st != HAL_OK) {
      printf("[Airlift] connect: SET_PASSPHRASE failed -> %s\r\n", hal_str(st));
      return st;
    }
  } else {
    const uint8_t *pp[1] = { (const uint8_t *)ssid };
    uint16_t ll[1] = { (uint16_t)strlen(ssid) };
    st = airlift_cmd_ack_only(ctx, SET_NET_CMD, pp, ll, 1,
                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
    if (st != HAL_OK) {
      printf("[Airlift] connect: SET_NET failed -> %s\r\n", hal_str(st));
      return st;
    }
  }

  uint8_t wl = 0xFF;
  uint32_t tmo = timeout_ms ? timeout_ms : 15000;
  uint32_t t0 = HAL_GetTick(), last_print = 0;

  while (HAL_GetTick() - t0 < tmo) {
    HAL_StatusTypeDef st2 = get_conn_status(ctx, &wl);
    if (st2 != HAL_OK) {
      printf("[Airlift] status: failed -> %s\r\n", hal_str(st2));
      return st2;
    }

    if (HAL_GetTick() - last_print > 300) {
      printf("[Airlift] status: %u\r\n", wl);
      last_print = HAL_GetTick();
    }

    if (wl == WL_CONNECTED) {
      printf("[Airlift] connected to \"%s\"\r\n", ssid);
      return HAL_OK;
    }
    if (wl == WL_CONNECT_FAILED) {
      printf("[Airlift] connect failed\r\n");
      return HAL_ERROR;
    }
    HAL_Delay(200);
  }

  printf("[Airlift] status: timeout waiting WL_CONNECTED\r\n");
  return HAL_TIMEOUT;
}

HAL_StatusTypeDef airlift_connect_enterprise(airlift_t *ctx, const char *ssid,
                                             const char *username,
                                             const char *password,
                                             const char *anonymous_identity,
                                             uint32_t timeout_ms) {
  if (!ssid || !username || !password) {
    printf("[Airlift] enterprise: missing ssid/username/password\r\n");
    return HAL_ERROR;
  }

  // Send SSID
  {
    const uint8_t *pp[1] = { (const uint8_t *)ssid };
    uint16_t ll[1] = { (uint16_t)strlen(ssid) };
    HAL_StatusTypeDef st = airlift_cmd_ack_only(ctx, SET_NET_CMD, pp, ll, 1,
                                                DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
    if (st != HAL_OK) { printf("[Airlift] enterprise: SET_NET failed\r\n"); return st; }
  }

  HAL_StatusTypeDef st;
  st = airlift_set_ent_identity(ctx, anonymous_identity); if (st != HAL_OK) return st;
  st = airlift_set_ent_username(ctx, username);          if (st != HAL_OK) return st;
  st = airlift_set_ent_password(ctx, password);          if (st != HAL_OK) return st;
  st = airlift_enable_enterprise(ctx);                   if (st != HAL_OK) return st;

  const uint32_t t0 = HAL_GetTick();
  const uint32_t tmo = timeout_ms ? timeout_ms : 30000;

  bool resent_ssid = false;
  bool kicked_scan = false;
  uint8_t err_streak = 0;

  while ((HAL_GetTick() - t0) < tmo) {
    uint8_t s = 0;
    st = get_conn_status_roundtrip(ctx, &s);
    if (st == HAL_OK) {
      err_streak = 0;
      printf("[Airlift] enterprise status: %u\r\n", s);
      if (s == WL_CONNECTED) {
        printf("[Airlift] enterprise connected to \"%s\"\r\n", ssid);
        return HAL_OK;
      }
      if (s == WL_CONNECT_FAILED) {
        printf("[Airlift] enterprise connect failed\r\n");
        return HAL_ERROR;
      }

      if (s == WL_NO_SSID_AVAIL) {
        uint32_t elapsed = HAL_GetTick() - t0;

        if (!resent_ssid && elapsed > 2000) {
          const uint8_t *pp[1] = { (const uint8_t *)ssid };
          uint16_t ll[1] = { (uint16_t)strlen(ssid) };
          uint8_t ack=0; uint16_t n=1;
          st = airlift_cmd_xfer(ctx, SET_NET_CMD, pp, ll, 1, &ack, &n,
                                false, false, DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS, NULL);
          printf("[Airlift] enterprise: reasserted SSID (ack=%u, st=%s)\r\n", ack, hal_str(st));
          resent_ssid = true;
        }

        if (!kicked_scan && elapsed > 4000) {
          uint8_t ack=0; uint16_t n=1;
          st = airlift_cmd_xfer(ctx, START_SCAN_NETWORKS, NULL, NULL, 0, &ack, &n,
                                false, false, SCAN_READY_WAIT_MS, SCAN_START_BYTE_TIMEOUT_MS, NULL);
          printf("[Airlift] enterprise: kicked scan (ack=%u, st=%s)\r\n", ack, hal_str(st));
          kicked_scan = true;
          HAL_Delay(1200);
        }
      }

      HAL_Delay(250);
      continue;
    }

    err_streak++;
    printf("[Airlift] enterprise: status poll err=%s (streak=%u)\r\n", hal_str(st), err_streak);

    (void)airlift_wait_ready(STATUS_READY_WAIT_MS);
    HAL_Delay(200 + (err_streak < 10 ? err_streak * 50 : 500));

    if (err_streak == 6) {
      const uint8_t *pp[1] = {(const uint8_t *)ssid};
      uint16_t ll[1] = {(uint16_t)strlen(ssid)};
      uint8_t ack=0; uint16_t n=1;
      HAL_StatusTypeDef st2 = airlift_cmd_xfer(ctx, SET_NET_CMD, pp, ll, 1, &ack, &n,
                                               false, false, DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS, NULL);
      printf("[Airlift] enterprise: reasserted SSID again (ack=%u, st=%s)\r\n", ack, hal_str(st2));
      HAL_Delay(1500);
    }
  }

  printf("[Airlift] enterprise connect timeout\r\n");
  return HAL_TIMEOUT;
}

// ---------------- DNS + Sockets using unified xfer ----------------

static HAL_StatusTypeDef airlift_dns_resolve(airlift_t *ctx, const char *host, uint8_t ip_out[4]) {
  if (!host || !ip_out) return HAL_ERROR;

  const uint8_t *pp[1] = { (const uint8_t *)host };
  uint16_t ll[1]       = { (uint16_t)strlen(host) };

  // Request
  HAL_StatusTypeDef st = airlift_cmd_ack_only(ctx, REQ_HOST_BY_NAME_CMD, pp, ll, 1,
                                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
  if (st != HAL_OK) { printf("[DNS] request failed\r\n"); return st; }

  // Get result
  uint8_t buf[4] = {0};
  uint16_t n = sizeof(buf);
  st = airlift_cmd_simple(ctx, GET_HOST_BY_NAME_CMD, NULL, NULL, 0, buf, &n);
  if (st != HAL_OK || n < 4) {
    printf("[DNS] reply bad len=%u\r\n", n);
    return (st == HAL_OK) ? HAL_ERROR : st;
  }
  memcpy(ip_out, buf, 4);
  printf("[DNS] %s -> %u.%u.%u.%u\r\n", host, ip_out[0], ip_out[1], ip_out[2], ip_out[3]);
  return HAL_OK;
}

static HAL_StatusTypeDef airlift_socket_alloc(airlift_t *ctx, uint8_t *sock_out) {
  if (!sock_out) return HAL_ERROR;
  uint8_t s = 0;
  uint16_t n = 1;
  HAL_StatusTypeDef st = airlift_cmd_simple(ctx, GET_SOCKET_CMD, NULL, NULL, 0, &s, &n);
  if (st != HAL_OK || n < 1) return (st == HAL_OK) ? HAL_ERROR : st;
  if (s == 255) {
    printf("[Sock] no sockets available\r\n");
    return HAL_ERROR;
  }
  *sock_out = s;
  printf("[Sock] allocated #%u\r\n", s);
  return HAL_OK;
}

static HAL_StatusTypeDef airlift_socket_open_tcp(airlift_t *ctx, uint8_t sock,
                                                 const uint8_t ip[4], uint16_t port) {
  uint8_t port_be[2] = {(uint8_t)((port >> 8) & 0xFF), (uint8_t)(port & 0xFF)};
  uint8_t sock_param[1] = {sock};
  uint8_t conn_mode[1] = {0}; // TCP

  const uint8_t *pp[4] = {ip, port_be, sock_param, conn_mode};
  uint16_t ll[4] = {4, 2, 1, 1};

  return airlift_cmd_ack_only(ctx, START_CLIENT_TCP_CMD, pp, ll, 4,
                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
}

static HAL_StatusTypeDef airlift_socket_write(airlift_t *ctx, uint8_t sock,
                                              const uint8_t *buf, uint16_t len) {
  uint16_t sent_total = 0;

  while (sent_total < len) {
    uint16_t chunk = (uint16_t)((len - sent_total) > 64 ? 64 : (len - sent_total));
    uint8_t sock_param[1] = {sock};
    const uint8_t *pp[2] = { sock_param, buf + sent_total };
    uint16_t ll[2] = { 1, chunk };

    // SEND_DATA_TCP_CMD uses 16-bit param length format on TX; reply is 1 byte
    uint8_t sent = 0;
    uint16_t n = 1;
    HAL_StatusTypeDef st = airlift_cmd_xfer(ctx, SEND_DATA_TCP_CMD, pp, ll, 2,
                                            &sent, &n,
                                            true,  false,
                                            DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS,
                                            NULL);
    if (st != HAL_OK || n < 1) {
      printf("[Sock] send reply error (st=%s)\r\n", hal_str(st));
      return (st == HAL_OK) ? HAL_ERROR : st;
    }
    sent_total += sent;
    if (sent != chunk) {
      printf("[Sock] partial send (%u/%u)\r\n", sent, chunk);
      break;
    }
  }

  // finalize DATA_SENT
  uint8_t sock_param[1] = {sock};
  const uint8_t *pp2[1] = { sock_param };
  uint16_t ll2[1] = { 1 };
  return airlift_cmd_ack_only(ctx, DATA_SENT_TCP_CMD, pp2, ll2, 1,
                              DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS);
}

static HAL_StatusTypeDef airlift_socket_available(airlift_t *ctx, uint8_t sock,
                                                  uint16_t *avail_out) {
  uint8_t sock_param[1] = {sock};
  const uint8_t *pp[1] = { sock_param };
  uint16_t ll[1] = { 1 };
  uint8_t buf[2] = {0};
  uint16_t n = sizeof(buf);
  HAL_StatusTypeDef st = airlift_cmd_simple(ctx, AVAIL_DATA_TCP_CMD, pp, ll, 1, buf, &n);
  if (st != HAL_OK || n < 2) return (st == HAL_OK) ? HAL_ERROR : st;
  *avail_out = (uint16_t)(buf[0] | ((uint16_t)buf[1] << 8));
  return HAL_OK;
}

static HAL_StatusTypeDef airlift_socket_read(airlift_t *ctx, uint8_t sock,
                                             uint8_t *buf, uint16_t want,
                                             uint16_t *got_out) {
  uint8_t sock_param[1] = {sock};
  uint8_t size_be[2] = { (uint8_t)((want >> 8) & 0xFF), (uint8_t)(want & 0xFF) };
  const uint8_t *pp[2] = { sock_param, size_be };
  uint16_t ll[2] = { 1, 2 };

  // GET_DATABUF_TCP_CMD uses 16-bit param lengths both TX and RX
  uint16_t n = want;
  HAL_StatusTypeDef st = airlift_cmd_xfer(ctx, GET_DATABUF_TCP_CMD, pp, ll, 2,
                                          buf, &n,
                                          true, true,
                                          DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS,
                                          NULL);
  if (st != HAL_OK) return st;
  if (got_out) *got_out = n;
  return HAL_OK;
}

static void airlift_socket_close(airlift_t *ctx, uint8_t sock) {
  uint8_t sock_param[1] = {sock};
  const uint8_t *pp[1] = { sock_param };
  uint16_t ll[1] = { 1 };
  (void)airlift_cmd_xfer(ctx, STOP_CLIENT_TCP_CMD, pp, ll, 1,
                         NULL, NULL,
                         false, false,
                         DEFAULT_READY_WAIT_MS, DEFAULT_START_BYTE_TIMEOUT_MS,
                         NULL);
}

HAL_StatusTypeDef airlift_http_get_print(airlift_t *ctx, const char *host, const char *path) {
  if (!host || !path) return HAL_ERROR;

  uint8_t ip[4];
  HAL_StatusTypeDef st = airlift_dns_resolve(ctx, host, ip);
  if (st != HAL_OK) return st;

  uint8_t sock = 0xFF;
  st = airlift_socket_alloc(ctx, &sock);
  if (st != HAL_OK) return st;

  st = airlift_socket_open_tcp(ctx, sock, ip, 80);
  if (st != HAL_OK) { airlift_socket_close(ctx, sock); return st; }

  char req[256];
  int req_len = snprintf(req, sizeof(req),
                         "GET %s HTTP/1.1\r\n"
                         "Host: %s\r\n"
                         "Connection: close\r\n"
                         "User-Agent: AirLift-C/1.0\r\n"
                         "\r\n", path, host);
  if (req_len <= 0 || req_len >= (int)sizeof(req)) { airlift_socket_close(ctx, sock); return HAL_ERROR; }

  st = airlift_socket_write(ctx, sock, (const uint8_t *)req, (uint16_t)req_len);
  if (st != HAL_OK) { airlift_socket_close(ctx, sock); return st; }

  uint32_t idle_start = HAL_GetTick();
  uint8_t rbuf[256];

  while (1) {
    uint16_t avail = 0;
    HAL_StatusTypeDef st2 = airlift_socket_available(ctx, sock, &avail);
    if (st2 != HAL_OK) {
      HAL_Delay(50);
      continue;
    }

    if (avail == 0) {
      if ((HAL_GetTick() - idle_start) > 1000) break;
      HAL_Delay(50);
      continue;
    }

    idle_start = HAL_GetTick();

    uint16_t to_read = (avail > sizeof(rbuf)) ? sizeof(rbuf) : avail;
    uint16_t got = 0;
    st2 = airlift_socket_read(ctx, sock, rbuf, to_read, &got);
    if (st2 != HAL_OK) {
      printf("[HTTP] read err %s\r\n", hal_str(st2));
      break;
    }
    for (uint16_t i = 0; i < got; i++) putchar(rbuf[i]);
  }
  printf("\r\n");
  airlift_socket_close(ctx, sock);
  return HAL_OK;
}
