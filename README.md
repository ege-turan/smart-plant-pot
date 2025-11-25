# Smart Self-Watering Plant Pot

## Pin map

SPI:
- Camera: SPI1 (PA4,5,6,7 NSS, SCK, MISO, MOSI)
- WiFi: SPI2 (PB12, PB13, PD3, PB15 NSS, SCK, MISO, MOSI)
- (N/A): SPI3 (PA15, PB3,4,5 NSS, SCK, MISO, MOSI)

I2C:
- Display: I2C1 (PB8 SCL, PB9 SDA)
- Soil sensor: I2C2 (PF0 SCL, PF1 SDA)
- Battery sensor: I2C3 (PC0 SCL, PC1 SDA)
- Camera: I2C4 (PF14 SCL, PF15 SDA)
- Temperature sensor: I2C2 (PF0 SCL, PF1 SDA)

GPIO:
- WiFi: two GPIO pins (PG0 RST, PG1 BUSY)
- Valve: one GPIO pin gated by NMOS (PF3 OUTPUT)
- Grow light: DAC or just a GPIO pin (PE4 OUTPUT)

ADC:
- Light Sensor: ADC1 IN3 (PC2)

## Commands

Sync code
- From vscode to stm32:
```bash
python sync_vscode_to_stm32.py <vscode_code_folder> <stm32_code_folder>
```
- From stm32 to vscode (to update HAL):
```bash
python sync_vscode_to_stm32.py <vscode_code_folder> <stm32_code_folder> --reverse
```

Read printf output with terminal:
```bash
screen /dev/tty.usbmodem2103 115200
```