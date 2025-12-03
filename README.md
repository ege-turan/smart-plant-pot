# Smart Self-Watering Plant Pot

## Pin map

SPI:
- Camera: SPI1 (PA4,5,6,7 NSS, SCK, MISO, MOSI)
- WiFi: SPI2 (PB12, PB13, PD3, PB15 NSS, SCK, MISO, MOSI)
- Display (screen): SPI3 (PA15, PB3,4,5 NSS, SCK, MISO, MOSI)

I2C:
- Display (touch): I2C1 (PB8 SCL, PB9 SDA)
- Soil sensor: I2C2 (PF0 SCL, PF1 SDA)
- Temperature sensor: I2C2 (PF0 SCL, PF1 SDA)
- Battery sensor: I2C3 (PC0 SCL, PC1 SDA)
- Camera: I2C4 (PF14 SCL, PF15 SDA)

GPIO:
- WiFi: two GPIO pins (PG0 RST, PG1 BUSY)
- Valve: one GPIO pin gated by NMOS (PF3 OUTPUT)

## Display pins
VCC
GND
LCD_CS
LCD_RST
LCD_RS
MOSI
SCK
LED
MISO
CTP_SCL
CTP_RST
CTP_SDA
CTP_INT
SD_CS

## Loose ends
Need to integrate the wifi to a webserver to pick up the images

1600 x 1200 -> 200 kb
640 x 480 

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