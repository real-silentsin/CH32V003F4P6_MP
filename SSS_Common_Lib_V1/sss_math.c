/********************************** (C) COPYRIGHT ******************************
 * File Name          : sss_math.c
 * Author             : vantr
 * Description        : Дополнительная библиотека простой математики
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "sss_math.h"
// -----------------------------------------------------------------------------
#ifdef MATH_USE_BCD
// Конвертирование десятичного значения в BCD представление
unsigned char BCDconv(unsigned char source) {
    unsigned char temp_min = 0;
    unsigned char temp_maj = 0;
    
    temp_min = source & 0x0F;
    temp_maj = source >> 4;

    temp_maj *= 10;

    return temp_maj + temp_min;
}
// -----------------------------------------------------------------------------
// Конвертирование числа в BCD представлении в десятичное значение
unsigned char DCBconv(unsigned char source) {
    unsigned char temp_min = 0;
    unsigned char temp_maj = 0;

    temp_maj = source / 10;
    temp_min = source - temp_maj * 10;
    temp_maj <<= 4;

    return temp_maj + temp_min;
}
#endif
// -----------------------------------------------------------------------------
// Проверка, входит ли значение в указанный диапазон: 1 - входит, 0 - нет
uint8_t ValueInRange (uint32_t value, uint32_t begin_range, uint32_t end_range) {

    if (begin_range < end_range) {

        return (value >= begin_range && value <= end_range) ? 1 : 0;
    }

    if (begin_range == end_range && value == begin_range) {
        return 1;
    }

    return 0;
}
// -----------------------------------------------------------------------------
// Декомпозиция (разложение целого числа на составляющие)
Params decompose(uint8_t value, uint8_t toChar) {

    Params result;

    if (toChar > 0) {

        result.extend = ((value / 100) + 0x30);     // сотни
        result.value2 = (((value / 10) % 10) + 0x30); // десятки
        result.value1 = ((value % 10) + 0x30);        // единицы
        result.sign = 0xFE;
    } else {
        result.extend = value / 100;        // сотни
        result.value2 = (value / 10) % 10;  // десятки
        result.value1 = value % 10;         // единицы
        result.sign = 0xFF;
    }

    return result;
}
// -----------------------------------------------------------------------------
// Целочисленное возведение в степень
// base - число, возводимое в степень
// exp - степень числа
uint32_t power(uint16_t basex, uint8_t exp) {
    
    if (exp == 0) { return 1; }
    if (exp == 1) { return basex; }
    
    uint32_t result = 1;
    
    while (exp != 0) {
        
        result *= basex;
        --exp;
    }
    
    return result;
}
// -----------------------------------------------------------------------------
// Вычисление значения 10 в степени exp
uint32_t power10(uint8_t exp) {
    
    return (uint32_t)power(10, exp);
}
// -----------------------------------------------------------------------------
// Получение индекса символа (HEX строки)
uint8_t _char_index_of(uint8_t value) {
    
    for (int index = 0; index < 16; index++) {
        
        if (HEX_STR[index] == value) {
            
            return index;
        }
    }
    
    return 0xFF;
}
// -----------------------------------------------------------------------------
// Конверт 4-значного HEX значения (типа F2ED) в 16-битное целое
uint16_t hex_to_int16(uint8_t * buffer) {
    
    uint16_t result = 0;
    
    for (int index = 0; index < 4; index++) {
        
        uint8_t chind = _char_index_of(buffer[index]);
        
        if (chind < 16) {
            
            result += (uint16_t)(power(16, (uint8_t)(3 - index)) * chind);
        }
    }
    
    return result;
}

// -----------------------------------------------------------------------------
// Конверт 8-значного HEX значения (типа F2EDA509) в 32-битное целое
uint32_t hex_to_int32(uint8_t *buffer) {

    uint32_t result = 0;

    for (int index = 0; index < 8; index++) {

        uint8_t chind = _char_index_of (buffer[index]);

        if (chind < 16) {

            result += (uint32_t)(power(16, (uint8_t)(7 - index)) * chind);
        }
    }

    return result;
}
// -----------------------------------------------------------------------------
// Перевод HEX в байт
uint8_t _hex_to_byte(Params value) {
    
    uint8_t val_tmp1 = _char_index_of(value.value1);
    if (val_tmp1 == 0xFF) { return 0; }
    
    uint8_t val_tmp2 = _char_index_of(value.value2);
    if (val_tmp1 == 0xFF) { return 0; }
    
    return (uint8_t)((val_tmp1 * 16) + val_tmp2);
}
// -----------------------------------------------------------------------------
// Перевод HEX в байт
uint8_t _hex_to_byte2(uint8_t value1, uint8_t value2) {

    uint8_t val_tmp1 = _char_index_of (value1);
    if (val_tmp1 == 0xFF) {
        return 0;
    }

    uint8_t val_tmp2 = _char_index_of (value2);
    if (val_tmp1 == 0xFF) {
        return 0;
    }

    return (uint8_t)((val_tmp1 * 16) + val_tmp2);
}
// -----------------------------------------------------------------------------
#ifdef MATH_USE_LONGINT
// -----------------------------------------------------------------------------
/* Структура буфера для длинных целых чисел.
 * Старшие цифры в старших позициях буфера
 * index      0  1  2  3  4  5  6  7
 * buffer = { 0, 0, 2, 7, 3, 9, 5, 1 } == 15937200
 */
// -----------------------------------------------------------------------------
// Сброс буферов конвертора
// buffer - ссылка на очищаемый байтовый буфер
// Значение, которым заполняют буфер
// buffer_size - размер очищаемого буфера
void reset_buffer(uint8_t * buffer, char value, uint16_t buffer_size) {

    for (uint16_t i = 0; i < buffer_size; i++) {

        buffer[i] = value;
    }
}
// -----------------------------------------------------------------------------
// Рассчет содержимого буфера (char* to int) (старший разряд в старшем адресе массива)
// buffer - ссылка на байтовый буфер, содержащий отдельные цифры длинного целого.
// buffer_size - размер буфера
uint32_t calc_buffer(uint8_t * buffer, uint8_t buffer_size) {

    uint32_t result = 0;

    for (int index = 0; index < buffer_size; index++) {

        if (buffer[index] == 0) { continue; }
        
        uint32_t step_realm = (uint32_t) (buffer[index] * power10(index));
        result += step_realm;
    }

    return result;
}
// -----------------------------------------------------------------------------
// Конвертировать целое число в массив цифр (int to char*) (старший разряд в старшем адресе массива)
// value - длинное целое (32бит), которое будет преобразовано в массив цифр
// buffer - ссылка на байтовый буфер, который будет содержать отдельные цифры длинного целого.
// buffer_size - размер буфера
void fill_buffer(uint32_t value, uint8_t * buffer, uint8_t buffer_size) {

    reset_buffer(buffer, 0, buffer_size);

    for (int index = 0; index < buffer_size; index++) {

        uint32_t divd = power10(index);
        buffer[index] = (uint8_t) ((value / divd) % 10);
    }
}
// -----------------------------------------------------------------------------
// Конвертировать целое число в HEX массив цифр (int to HEX*) (старший разряд в старшем адресе массива)
// value - длинное целое (16бит), которое будет преобразовано в массив цифр
// buffer - ссылка на байтовый буфер (4 байта)
void fill_buffer_hex16(uint16_t value, uint8_t * buffer) {

    reset_buffer(buffer, '0', 4);

    uint8_t index = 0;

    while (value > 0) {
        // Оптимизация: value % 16 заменяем на выделение младшего полубайта
        int temp = (int)(value & 0x0F);

        buffer[3 - index] = HEX_STR[temp];

        index++;
        if (index > 3) {
            return;
        }

        // Оптимизация: value / 16 заменяем на сдвиг вправо на 4 бита
        value >>= 4;
    }
}

// -----------------------------------------------------------------------------
// Конвертировать целое число в HEX массив цифр (int to HEX*) (старший разряд в старшем адресе массива)
// value - длинное целое (32бит), которое будет преобразовано в массив цифр
// buffer - ссылка на байтовый буфер (8 байт)
void fill_buffer_hex32(uint32_t value, uint8_t *buffer) {

    reset_buffer (buffer, '0', 8);

    uint8_t index = 0;

    while (value > 0) {
        // Оптимизация: вместо деления с остатком берем маской нижние 4 бита
        int temp = (int)(value & 0x0F);

        buffer[7 - index] = HEX_STR[temp];

        index++;
        if (index > 7) {
            return;
        }

        // Оптимизация: вместо деления на 16 сдвигаем число на 4 бита вправо
        value >>= 4;
    }
}
#endif
// -----------------------------------------------------------------------------
