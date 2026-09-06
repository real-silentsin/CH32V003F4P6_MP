/********************************** (C) COPYRIGHT ******************************
 * File Name          : sss_math.h
 * Author             : vantr
 * Version            : V1.0.5
 * Date               : 2026/07/08
 * Description        : Дополнительная библиотека простой математики
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе. Допускается к использованию.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef SSS_MATH
// -----------------------------------------------------------------------------
#define SSS_MATH
// -----------------------------------------------------------------------------
#define MATH_USE_LONGINT    // Разрешение использовать преобразования длинных целых чисел
//#define MATH_USE_BCD        // Разрешение двоично-десятичных преобразований
// -----------------------------------------------------------------------------
#include <stdlib.h>
#include "sss_classes.h"
// -----------------------------------------------------------------------------
// Степени и логарифмы
Params decompose (uint8_t value, uint8_t toChar);
uint32_t power (uint16_t basex, uint8_t exp);
uint32_t power10(uint8_t exp);
// Конвертеры
uint16_t hex_to_int16(uint8_t *buffer);
uint32_t hex_to_int32 (uint8_t *buffer);
uint8_t _hex_to_byte (Params value);
uint8_t _hex_to_byte2 (uint8_t value1, uint8_t value2);
// Проверки
uint8_t ValueInRange (uint32_t value, uint32_t begin_range, uint32_t end_range);
// -----------------------------------------------------------------------------
#ifdef MATH_USE_BCD
    // Двоично-десятичная арифметика
    unsigned char BCDconv (unsigned char source);
unsigned char DCBconv(unsigned char source);
#endif
// -----------------------------------------------------------------------------
#ifdef MATH_USE_LONGINT
// Работа с длинными целыми числами
void reset_buffer(uint8_t * buffer, char value, uint16_t buffer_size);
uint32_t calc_buffer(uint8_t * buffer, uint8_t buffer_size);
void fill_buffer(uint32_t value, uint8_t * buffer, uint8_t buffer_size);
void fill_buffer_hex16(uint16_t value, uint8_t * buffer);
void fill_buffer_hex32(uint32_t value, uint8_t *buffer);
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
#endif
    // -----------------------------------------------------------------------------
