/********************************** (C) COPYRIGHT *******************************
 * File Name          : HT16K33.h
 * Author             : vantr
 * Version            : V2.0.0
 * Date               : 2026/01/03
 * Description        : Общая библиотека
 *********************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef __SSS_HT16K33HW__
// -----------------------------------------------------------------------------
#define __SSS_HT16K33HW__
// -----------------------------------------------------------------------------
#include "app_config.h"
// -----------------------------------------------------------------------------
// Допустимые команды для чипа драйвера
#define HT16K33_CMD_SYSSET_OFF      0x20    // Чип отключен
#define HT16K33_CMD_SYSSET_ON       0x21    // Чип включен

#define HT16K33_CMD_INT_OFF         0xA0    // Вывод прерывание используется как ROW
#define HT16K33_CMD_INT_ON_POS      0xA3    // Прерывание INT с положительным перепадом
#define HT16K33_CMD_INT_ON_NEG      0xA1    // Прерывание INT с отрицательным перепадом

#define HT16K33_CMD_DISP_OFF        0x80    // Дисплей отключен
#define HT16K33_CMD_DISP_NORMAL     0x81    // Дисплей нормально включен
#define HT16K33_CMD_DISP_BL_2HZ     0x83    // Дисплей мерцает с частотой 2 герца
#define HT16K33_CMD_DISP_BL_1HZ     0x85    // Дисплей мерцает с частотой 1 герц
#define HT16K33_CMD_DISP_BL_HHZ     0x87    // Дисплей мерцает с частотой 0.5 герца

#define HT16K33_CMD_DISP_BRIGHT     0xE0    // Нулевое значение яркости (до + 15)
// -----------------------------------------------------------------------------
// Адреса знакомест в памяти чипа
#define HT16K33_ADDR_DIGIT0         0x00
#define HT16K33_ADDR_DIGIT1         0x02
#define HT16K33_ADDR_DIGIT2         0x04
#define HT16K33_ADDR_DIGIT3         0x06
#define HT16K33_ADDR_DIGIT4         0x08
#define HT16K33_ADDR_DIGIT5         0x0A
#define HT16K33_ADDR_DIGIT6         0x0C
#define HT16K33_ADDR_DIGIT7         0x0E

#define HT16K33_ADDR_DIGIT8         0x01
#define HT16K33_ADDR_DIGIT9         0x03
#define HT16K33_ADDR_DIGIT10        0x05
#define HT16K33_ADDR_DIGIT11        0x07
#define HT16K33_ADDR_DIGIT12        0x09
#define HT16K33_ADDR_DIGIT13        0x0B
#define HT16K33_ADDR_DIGIT14        0x0D
#define HT16K33_ADDR_DIGIT15        0x0F
// -----------------------------------------------------------------------------
// Адреса регистров клавиш в памяти чипа
#define HT16K33_KEY_ADDR_P0         0x40    // KS0 [K1-K8]
#define HT16K33_KEY_ADDR_P1         0x41    // KS0 [K9-K16]
#define HT16K33_KEY_ADDR_P2         0x42    // KS1 [K1-K8]
#define HT16K33_KEY_ADDR_P3         0x43    // KS1 [K9-K16]
#define HT16K33_KEY_ADDR_P4         0x44    // KS2 [K1-K8]
#define HT16K33_KEY_ADDR_P5         0x45    // KS2 [K9-K16]
// -----------------------------------------------------------------------------
// Реальные адреса цифр
static const uint8_t HT_DIGITS_ADDRESS[16] = { 
    HT16K33_ADDR_DIGIT0,    // Первое слева знакоместо
    HT16K33_ADDR_DIGIT8,    // ------------------------------
    HT16K33_ADDR_DIGIT1,    // Второе слева знакоместо
    HT16K33_ADDR_DIGIT9,    // ------------------------------
    HT16K33_ADDR_DIGIT2,    // Третье слева знакоместо
    HT16K33_ADDR_DIGIT10,   // ------------------------------
    HT16K33_ADDR_DIGIT3,    // Четвертое слева знакоместо
    HT16K33_ADDR_DIGIT11,   // ------------------------------
    HT16K33_ADDR_DIGIT4,    // Пятое знакоместо
    HT16K33_ADDR_DIGIT12,   // ------------------------------
    HT16K33_ADDR_DIGIT5,    // Шестое знакоместо
    HT16K33_ADDR_DIGIT13,   // ------------------------------
    HT16K33_ADDR_DIGIT6,    // Седьмое знакоместо
    HT16K33_ADDR_DIGIT14,   // ------------------------------
    HT16K33_ADDR_DIGIT7,    // Восьмое знакоместо
    HT16K33_ADDR_DIGIT15,   // ------------------------------
};
// -----------------------------------------------------------------------------
uint8_t HT16K33_GetAddress(uint8_t chip_number);
uint32_t HT16K33_SetHeader(uint8_t chip_number, uint8_t mode);

void HT16K33_Init(uint8_t chip_number, uint8_t int_enabled);
void HT16K33_Intensity(uint8_t chip_number, uint8_t brightness);

uint32_t HT16K33_WriteCMD(uint8_t chip_number, uint8_t data);
uint8_t HT16K33_Write(uint8_t chip_number, uint8_t address, uint8_t data);
uint8_t HT16K33_Read(uint8_t chip_number, uint8_t address, uint8_t default_result);

void HT16K33_Send(uint8_t chip_number, uint8_t place, uint8_t value);
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
