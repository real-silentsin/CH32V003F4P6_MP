/********************************** (C) COPYRIGHT ******************************
 * File Name          : LCD1602.h
 * Author             : vantr
 * Version            : V2.1.0
 * Date               : 2026/07/29
 * Description        : Библиотека обслуживания LCD1602A/2004 (аппаратный I2C)
 * Target             : Версия MEGAPACK
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd
 * Библиотека проверена в железе
 *******************************************************************************/
// Библиотека проверена для обоих режимов I2C
// -----------------------------------------------------------------------------
#ifndef SSS_LCD1602H
#define	SSS_LCD1602H
// -----------------------------------------------------------------------------
#define E_Delay     150
#define I_Delay     40
// -----------------------------------------------------------------------------
#include "app_config.h"
// -----------------------------------------------------------------------------
// PCF8574(T)
// ВНИМАНИЕ! Китайский модуль может НЕ РАБОТАТЬ с CH32X033!
#ifdef USE_CHINESE_I2C_MODULE
#define HA_PCF8574     0x40        // Китайский модуль! 0x20 (0x27)
#endif
// -----------------------------------------------------------------------------
#ifdef USE_MY_I2C_MODULE
// PCF8574A
#define HA_PCF8574     0x70         // Мой модуль 0x38
#endif
// -----------------------------------------------------------------------------
#ifdef USE_MY_NEW_3V_MODULE
// PCF8574A
#define HA_PCF8574     0x70         // Мой новый модуль 0x38
#endif
// -----------------------------------------------------------------------------
// Аппаратный адрес драйвера (PCF8574) A0-A2
//#define LCD_I2C_HA  0
// -----------------------------------------------------------------------------
// Номера битов активных портов LCD индикатора, LCD в 4-х битовом режиме
// -----------------------------------------------------------------------------
// Для китайского модуля I2C (проверен в железе)
#if defined(USE_CHINESE_I2C_MODULE) || defined (USE_MY_NEW_3V_MODULE)
#define LCD_D4      4   // Бит 0 данных
#define LCD_D5      5   // Бит 1 данных
#define LCD_D6      6   // Бит 2 данных
#define LCD_D7      7   // Бит 3 данных

#define LCD_RS      0
#define LCD_E       2
#define LCD_RW      1

#define LCD_BL      3   // Подсветка
#endif
// -----------------------------------------------------------------------------
// Мой модуль (5-вольтовый) V1.2(3)
#ifdef USE_MY_I2C_MODULE
#define LCD_D4      0
#define LCD_D5      1
#define LCD_D6      2
#define LCD_D7      3

#define LCD_RS      6
#define LCD_E       5

#define LCD_BL      4   // Подсветка
#endif
// -----------------------------------------------------------------------------
void lcd_init(uint8_t chip_number);
void lcd_command(uint8_t data);
void lcd_data(uint8_t data);
void lcd_clear(void);
void lcd_setCursor(uint8_t line, uint8_t position);

void lcd_writeStr(char *str);
void lcd_writeStrPos(uint8_t line, uint8_t position, char *str);
void lcd_customChar(uint8_t location, uint8_t *data);
void lcd_clearLine(uint8_t line);

void lcd_print_int8(uint8_t value);

void lcd_blcontrol(uint8_t value);
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------