/********************************** (C) COPYRIGHT ******************************
 * File Name          : ST920_SPI_Lib1.h
 * Author             : vantr
 * Version            : V1.0.0
 * Date               : 2026/04/30
 * Description        : Драйвер I2C LCD ST920
 * Target             : CH32V003
 *******************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "ch32x035.h"
#include "ST920_I2C_Lib1.h"
#include "SSS_PCF8574_Lib1/PCF8574x.h"
#include "SSS_I2CHW_MLib_V4.4/i2c.h"
#include "SSS_Common_Lib_V1/sss_classes.h"
// ----------------------------------------------------------------------------
// Подстветка дисплея
uint8_t backlight_enabled = 0;
// Физический адрес чипа [A0..A2]
uint8_t chip_addr = 0;
// -----------------------------------------------------------------------------
// Вывод данных в порт
void _lcd_set_data (uint8_t data, uint8_t rs) {

    uint8_t _tmp_byte = 0;

    if (is_set_idx (data, 0)) { sbi (_tmp_byte, LCD_D4); }
    if (is_set_idx (data, 1)) { sbi (_tmp_byte, LCD_D5); }
    if (is_set_idx (data, 2)) { sbi (_tmp_byte, LCD_D6); }
    if (is_set_idx (data, 3)) { sbi (_tmp_byte, LCD_D7); }

    if (rs > 0) { sbi (_tmp_byte, LCD_RS); }
    
    if (backlight_enabled > 0) { sbi (_tmp_byte, LCD_BL); }

    // Новый вариант строба
    sbi (_tmp_byte, LCD_E);
    PCF_Write (HA_PCF8574, chip_addr, _tmp_byte);
    Delay_Us (10);

    cbi (_tmp_byte, LCD_E);
    PCF_Write (HA_PCF8574, chip_addr, _tmp_byte);
    Delay_Us (10);
}
// -----------------------------------------------------------------------------
// Отправка команды
void ST7920_WriteCmd (uint8_t cmd) {

    // RS = 0 (запись команд)
    _lcd_set_data (cmd >> 4, 0);
    _lcd_set_data (cmd, 0);

    Delay_Us (40);
}
// -----------------------------------------------------------------------------
// Отправка данных
void ST7920_WriteData (uint8_t data) {

    // RS = 1 (запись данных)
    _lcd_set_data (data >> 4, 1);
    _lcd_set_data (data, 1);

    Delay_Us (40);
}
// -----------------------------------------------------------------------------
// Управление подсветкой
void ST7920_blcontrol (uint8_t value) {

    backlight_enabled = (value == 0) ? 0 : 1;

    uint8_t _tmp_byte = 0;

    sbi (_tmp_byte, LCD_RS);
    if (backlight_enabled > 0) {
        sbi (_tmp_byte, LCD_BL);
    }

    PCF_Write (HA_PCF8574, chip_addr, _tmp_byte);
}
// -----------------------------------------------------------------------------
// Переключение дисплея в графический режим
// Вызови lcd_init() для возврата в текстовый режим
void ST7920_ToGraphMode() {

    ST7920_WriteCmd (0x20);
    ST7920_WriteCmd (0x24);    // 4bit data, extendid instructions
    ST7920_WriteCmd (0x26);    // +graphics
}
// -----------------------------------------------------------------------------
// Переключение дисплея в текстовый режим
void ST7920_ToTextMode() {

    ST7920_WriteCmd (0x20);  // 4bit data, basic instructions
}
// -----------------------------------------------------------------------------
// Полная очистка экрана
void ST7920_ClearScreen (void) {

    for (uint8_t y = 0; y < 32; y++) {
        // Устанавливаем адрес начала строки
        ST7920_WriteCmd (0x80 + y);  // Y
        ST7920_WriteCmd (0x80);      // X (левая половина)

        // Заполняем 16 байт (левая + правая часть за один проход в некоторых ревизиях,
        // но надежнее писать по 32 байта на адрес Y или двумя блоками)
        for (uint8_t i = 0; i < 32; i++) {
            ST7920_WriteData (0x00);
        }
    }
}
// -----------------------------------------------------------------------------
// Пересылка виртуального буфера (вертикальная структура) в ST7920 (4-bit mode)
void ST7920_FlushBuffer(const uint8_t *fb) {

    for (uint8_t y = 0; y < 64; y++) {
        
        uint8_t current_y = 63 - y;

        uint8_t phys_y = (current_y < 32) ? current_y : (current_y - 32);
        uint8_t phys_x = (current_y < 32) ? 0x80 : 0x88;

        ST7920_WriteCmd (0x80 + phys_y);
        ST7920_WriteCmd (phys_x);

        for (uint8_t x_byte = 0; x_byte < 16; x_byte++) {

            uint8_t out = 0;
            for (uint8_t b = 0; b < 8; b++) {

                uint8_t curr_x = (x_byte * 8) + b;
                // Адаптация под вертикальный виртуальный буфер fb[X + (Y/8)*128]
                if (fb[curr_x + (current_y / 8) * 128] & (1 << (current_y % 8))) {

                    out |= (0x80 >> b);
                }
            }

            ST7920_WriteData (out);
        }
    }
}
// -----------------------------------------------------------------------------
// Инициализация графического режима
void ST7920_Init_Graphic (uint8_t sw_chip_addr) {

    chip_addr = sw_chip_addr;

    Delay_Ms (100);

    // 1. ЖЕСТКИЙ ПЕРЕВОД В 8-БИТ (сброс автомата)
    _lcd_set_data (0x03, 0);  // Выставили 0011 (старшая часть от 0x30)
    Delay_Ms (10);         // Дали время!

    _lcd_set_data (0x03, 0);
    Delay_Ms (1);

    // 2. ПЕРЕХОД В 4-БИТ
    _lcd_set_data (0x02, 0);  // Выставили 0010 (команда 0x20)
    Delay_Ms (5);          // КРИТИЧНО: дисплей перестраивает логику

    // 3. ТЕПЕРЬ МОЖНО СЛАТЬ ПО 2 НИББЛА (WriteCmd)
    ST7920_WriteCmd (0x28);  // Function set: 4-bit, basic
    Delay_Us (100);
    ST7920_WriteCmd (0x0C);  // Display ON
    Delay_Us (100);
    ST7920_WriteCmd (0x01);  // Clear
    Delay_Ms (15);
}
// -----------------------------------------------------------------------------