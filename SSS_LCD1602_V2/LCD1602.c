/********************************** (C) COPYRIGHT ******************************
 * File Name          : LCD1602.c
 * Author             : vantr
 * Version            : V1.0.1
 * Date               : 2026/01/20
 * Description        : Библиотека обслуживания LCD1602A/2004
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd
 * Модификация драйвера LCD для работы через порт PCF8574
 *******************************************************************************/
#include "LCD1602.h"
#include "SSS_PCF8574_Lib1/PCF8574x.h"
#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_I2CHW_MLib_V4.4/i2c.h"
#include "debug.h"
// -----------------------------------------------------------------------------
// Подстветка дисплея
uint8_t backlight_enabled = 0;
// Физический адрес чипа [A0..A2]
uint8_t chip_addr = 0;
// -----------------------------------------------------------------------------
#ifdef DISPLAY_IS_2004
// Адреса строк четырехстрочного дисплея
static const uint8_t DISPLAY_LINE_ADDRS[] = { 0x80, 0xC0, 0x90, 0xD0 };
#endif
// -----------------------------------------------------------------------------
// Вывод данных в порт
void _lcd_set_data(uint8_t data, uint8_t rs) {
    
    uint8_t _tmp_byte = 0;
    
    if (is_set_idx(data, 0)) { sbi(_tmp_byte, LCD_D4); }
    if (is_set_idx(data, 1)) { sbi(_tmp_byte, LCD_D5); }
    if (is_set_idx(data, 2)) { sbi(_tmp_byte, LCD_D6); }
    if (is_set_idx(data, 3)) { sbi(_tmp_byte, LCD_D7); }
    
    if (rs > 0) { sbi(_tmp_byte, LCD_RS); }
    //if (e > 0) { sbi(_tmp_byte, LCD_E); }
    
    if (backlight_enabled > 0) { sbi(_tmp_byte, LCD_BL); }
    
    // Новый вариант строба
    sbi(_tmp_byte, LCD_E);    
    PCF_Write(HA_PCF8574, chip_addr, _tmp_byte);
    Delay_Us(10);
    
    cbi(_tmp_byte, LCD_E);
    PCF_Write(HA_PCF8574, chip_addr, _tmp_byte);
    Delay_Us(10);
}
// -----------------------------------------------------------------------------
// Функция записи команды в LCD
void lcd_command(uint8_t data) {

    // RS = 0 (запись команд)
    _lcd_set_data(data >> 4, 0);
    _lcd_set_data(data, 0);

    Delay_Us(40);
}
// -----------------------------------------------------------------------------
// Функция записи данных в LCD
void lcd_data(uint8_t data) {

    // RS = 1 (запись данных)
    _lcd_set_data(data >> 4, 1);
    _lcd_set_data(data, 1);

    Delay_Us(40);
}
// -----------------------------------------------------------------------------
// Управление подсветкой
void lcd_blcontrol(uint8_t value) {
    
    backlight_enabled = (value == 0) ? 0 : 1;
    
    uint8_t _tmp_byte = 0;
    
    sbi(_tmp_byte, LCD_RS);
    if (backlight_enabled > 0) { sbi(_tmp_byte, LCD_BL); }
    
    PCF_Write(HA_PCF8574, chip_addr, _tmp_byte);
}
// -----------------------------------------------------------------------------
// Очистка экрана LCD
void lcd_clear(void) {
    
    lcd_command(0x01); // Очистка дисплея
    Delay_Ms(2);
    //lcd_command(0x80); // Курсор "домой"
}
// -----------------------------------------------------------------------------
// Установка курсора в указанную позицию
void lcd_setCursor(uint8_t line, uint8_t position) {
    
#ifndef DISPLAY_IS_2004
    if (line > 1) { line = 1; }
    lcd_command((line * 0x40 + position) | 0x80);
#else
    if (line > 3) { line = 3; }
    lcd_command((DISPLAY_LINE_ADDRS[line] + position) | 0x80);
#endif
}
// -----------------------------------------------------------------------------
// Вывод строки на индикатор в позицию курсора
void lcd_writeStr(char *str) {

    int abs;
    
    for (abs = 0; str[abs] != '\0'; ++abs) {
        lcd_data(str[abs]);
    }
}
// -----------------------------------------------------------------------------
// Вывод строки на индикатор в позицию курсора
void lcd_writeStrPos(uint8_t line, uint8_t position, char *str) {

    lcd_setCursor(line, position);
    lcd_writeStr(str);
}
// -----------------------------------------------------------------------------
// Запись в индикатор пользовательского символа (до восьми штук)
void lcd_customChar(uint8_t location, uint8_t *data) {
    uint8_t i;

    if (location < 8) {

        lcd_command(0x40 + (location * 8));
        for (i = 0; i < 8; i++)
            lcd_data(data[i]);
    }
}
// -----------------------------------------------------------------------------
// Очистка указанной строки дисплея
void lcd_clearLine(uint8_t line) {
    
    lcd_setCursor(line, 0);
    
    for (int i = 0; i < 16; i++) {
        
        lcd_data(' ');
    }
}
// -----------------------------------------------------------------------------
// Печать 8-битного целого в текущую позицию индикатора
void lcd_print_int8(uint8_t value) {
    
    Params t_temp = _int_to_str(value);
    
    lcd_data(t_temp.value1);
    lcd_data(t_temp.value2);
}
// -----------------------------------------------------------------------------
// Первичная инициализация LCD после включения питания
void lcd_init(uint8_t chip_number) {
    // -------------------------------------------------------------------------
    // Сброс дисплея при включении питания
    Delay_Ms(100);
    
    chip_addr = chip_number;
    backlight_enabled = 1;
    
    //IIC_Init(I2C_SELF_ADDRESS);

    _lcd_set_data(0x03, 0);
    Delay_Ms(5);

    _lcd_set_data(0x03, 0);
    Delay_Ms(100);

    _lcd_set_data(0x03, 0);
    Delay_Ms(100);

    // set 4 bit mode
    _lcd_set_data(0x02, 0);
    Delay_Us(100);
    // -------------------------------------------------------------------------
    // Конфигурация дисплея
    lcd_command(0x28); // 4 bit mode, 1/16 duty, 5x8 font
    Delay_Us(60);

    lcd_command(0x08); // display off
    Delay_Us(60);

    lcd_command(0x01); // display off (bit 1), blink curson off (bit 0)
    Delay_Ms(3);

    lcd_command(0x06); // entry mode
    Delay_Us(60);

    lcd_command(0x0C); // entry mode
    Delay_Us(60);
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------