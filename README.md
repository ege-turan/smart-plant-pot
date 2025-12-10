# Smart Self-Watering Plant Pot

## Pin map

SPI:
- Display: SPI1 (D0, PA5,6,7 NSS, SCK, MISO, MOSI)
- Camera: SPI1 (PA4,5,6,7 NSS, SCK, MISO, MOSI)

I2C:
- Display (touch): I2C1 (PB8 SCL, PB9 SDA)
- Soil sensor: I2C2 (PF0 SCL, PF1 SDA)
- Temperature sensor: I2C2 (PF0 SCL, PF1 SDA)
- Light sensor: I2C2 (PF0 SCL, PF1 SDA)
- Camera: I2C4 (PF14 SCL, PF15 SDA)

GPIO:
- Pump: one GPIO pin (PB2 OUTPUT)
- Display (touch): PF7 RST, PF9 INT

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