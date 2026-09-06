/********************************** (C) COPYRIGHT ******************************
 * File Name          : sss_classes.c
 * Author             : vantr
 * Description        : Общая библиотека
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "sss_classes.h"
// -----------------------------------------------------------------------------
// Согласно даташиту, если биты [6:5] равны 11b, то NRST отключен и пин работает как GPIO
#define OBR_RST_MODE_GPIO ((uint32_t)0x00000060)
// -----------------------------------------------------------------------------
// Перевод целого числа [0..99] в символы
Params _int_to_str(uint8_t value) {
    
    Params result;
    
#ifdef USE_LCD
    result.value1 = (((value / 10) % 10) + 0x30);
    result.value2 = ((value % 10) + 0x30);
#else
    result.value1 = ((value / 10) % 10);
    result.value2 = (value % 10);
#endif
    
    return (result);
}
// -----------------------------------------------------------------------------
// Перевод целого числа в символьное шестнадцатиричное
// Полностью зависит от файла symbols.h и кодировки типа _symbol
Params _int_to_hex(uint8_t value) {
    
    Params result;
    
    result.value1 = HEX_STR[(value >> 4) & 0x0F];
    result.value2 = HEX_STR[value & 0x0F];
    
    return result;
}
// -----------------------------------------------------------------------------
// Разбор чисел до 9999 на отдельные значения - { 9, 9, 9, 9 }
Params decompose_uint16 (uint16_t value) {

    Params result;

    result.sign = (value / 1000) % 10;   // Тысячи
    result.extend = (value / 100) % 10;  // Сотни
    result.value2 = (value / 10) % 10;   // Десятки
    result.value1 = value % 10;          // Единицы

    return result;
}
// -----------------------------------------------------------------------------
#ifdef MCU_TARGET_TYPE_CH32V003
#include "ch32v00x_flash.h"
// Программное отключение линии сброса
void Disable_NRST_Pin (void) {

    // 0.5 секунды, чтобы программатор мог перехватить МК при прошивке
    Delay_Ms (500);

    // Читаем регистр OBR напрямую
    uint32_t current_obr = FLASH->OBR;

    // Выделяем маской биты [6:5] (RST_MODE)
    if ((current_obr & OBR_RST_MODE_GPIO) != OBR_RST_MODE_GPIO) {
        
        // Пин PD7 сейчас работает как АППАРАТНЫЙ RESET
        // (биты равны 00b, 01b или 10b в зависимости от времени задержки)
        FLASH_Unlock();            // Разблокируем работу с Flash
        FLASH_EraseOptionBytes();  // Стираем текущие Option Bytes

        // Записываем конфигурацию. Флаг OB_RST_NoEN — ключевой, он отключает NRST на PD7
        FLASH_UserOptionByteConfig (OB_STOP_NoRST, OB_STDBY_NoRST, OB_RST_NoEN, OB_PowerON_Start_Mode_USER);

        FLASH_Lock();  // Блокируем Flash обратно

        // Для применения настроек Option Bytes микроконтроллер должен перезагрузиться
        NVIC_SystemReset();
    }
}
#endif
// -----------------------------------------------------------------------------
