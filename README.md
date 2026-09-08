# CH32V003F4P6_MP
MEGAPACK library

Without connection to the BB (with transport and X communication protocol disabled), the code takes up:
• Flash - 40%
• RAM - 40%
Assuming only the ST7789 and W25Qxx are connected.

With transport enabled (for the full demo):
• Flash - 87%
• RAM - 95%

The difference between this and the original library is in the way the file list is output.
This version only outputs the ID, without the size or CRC.

The library uses:
• only one timer, no interrupts
• plays sound from protocol files, RAM buffers, and a contiguous area
• it's best to use the "contiguous area" for playing long sounds

• standard for hardware SPI
• support for hardware and software SPI
• all settings in <app_config.h>
• dynamic control menus

Supported hardware:
• W25Q32,64,128 FLASH memory
• ST7789 SPI TFT display (SPI at 24 MHz)
• SSD1306 SPI OLED display (SPI at 12 MHz) (in the demo version, only TXT! due to insufficient RAM)
• I2C module for external peripherals
• I2C OLED text display, can be used with another graphic display
• I2C OLED graphic display, can NOT be used with another graphic display (SPI)
• I2C EEROM 24С02..24С64
• Sound
• Infrared port (NEC protocol)

Library audio standard: 8000Hz samples, Headerless, Unsigned data.

(c) silentsin
03/08/2026

