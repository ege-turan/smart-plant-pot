# Smart Self-Watering Plant Pot

## Pin map

SPI:
- Camera: SPI1 (PA4,5,6,7 NSS, SCK, MISO, MOSI)
- WiFi: SPI3 (PA15, PB3,4,5 NSS, SCK, MISO, MOSI)

I2C:
- Display: I2C1 (PB8 SCL, PB9 SDA)
- Soil sensor: I2C2 (PF0 SCL, PF1 SDA)
- Camera: I2C4 (PF14 SCL, PF15 SDA)
- Battery sensor: Not assigned for now

Other:
- Light Sensor: ADC
- Temperature sensor: one GPIO pin
- Valve: one GPIO pin (gated by NMOS)
- Grow light: DAC or just a GPIO pin

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