//----------------------------------------------------------------------------------
// Модуль для работы с шиной 1-WARE
//----------------------------------------------------------------------------------
#ifndef __SSSDS18B20DRV1__
// -----------------------------------------------------------------------------
#define __SSSDS18B20DRV1__
// ------------------------------------------------------------------------------------------------
#include "app_config.h"
// ------------------------------------------------------------------------------------------------
// Макросы для управления портом
#define ONEWARE_LOW() (ONEWARE_PORT.port->BCR = ONEWARE_PORT.pin)          // 0
#define ONEWARE_HIGH() (ONEWARE_PORT.port->BSHR = ONEWARE_PORT.pin)        // 1
// ------------------------------------------------------------------------------------------------
typedef enum {
    OW_PORT_IN = 0,
    OW_PORT_OUT
} PORT_1WARE_MODE_Types;
// ------------------------------------------------------------------------------------------------
// Установка режима порта
void OW_setPinMode (GPIO_TypeDef *GPIOx, uint8_t pin, PORT_1WARE_MODE_Types mode) {

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = pin;

    switch (mode) {

    case OW_PORT_OUT:
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        break;

    default:
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        break;
    }

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (GPIOx, &GPIO_InitStructure);
}
// ------------------------------------------------------------------------------------------------
void OW_set_PORT_OUT (void) {

    OW_setPinMode (ONEWARE_PORT.port, ONEWARE_PORT.pin, OW_PORT_OUT);
}
// ------------------------------------------------------------------------------------------------
void OW_set_PORT_IN (void) {

    OW_setPinMode (ONEWARE_PORT.port, ONEWARE_PORT.pin, OW_PORT_IN);
}
// ------------------------------------------------------------------------------------------------
void SETOWPIN (uint8_t value) {

    if (value == 0) { ONEWARE_LOW(); } else { ONEWARE_HIGH(); }

    //BitAction tmp_bit = (value == 0) ? Bit_RESET : Bit_SET;
    //GPIO_WriteBit (ONEWARE_PORT.port, ONEWARE_PORT.pin, tmp_bit);
}
// ------------------------------------------------------------------------------------------------
uint8_t READDOW() {

    return GPIO_ReadInputDataBit (ONEWARE_PORT.port, ONEWARE_PORT.pin);
}
// ------------------------------------------------------------------------------------------------
// #pragma endregion
// ------------------------------------------------------------------------------------------------
// Инициализация порта OneWare
void OneWare_Init() {

    RCC_APB2PeriphClockCmd (ONEWARE_PORT.rcc, ENABLE);
    OW_set_PORT_OUT();
}
// ------------------------------------------------------------------------------------------------
// Запись одного бита
void OneWare_WriteBit (uint8_t bit) {
    OW_set_PORT_OUT();
    SETOWPIN (0);  // Начало слота

    if (bit & 0x01) {
        Delay_Us (2);  // Передача "1"
        OW_set_PORT_IN();
        Delay_Us (60);
    } else {
        Delay_Us (60);  // Передача "0"
        OW_set_PORT_IN();
        Delay_Us (2);
    }
    Delay_Us (5);  // Recovery time
}
// ------------------------------------------------------------------------------------------------
// Чтение одного бита
uint8_t OneWare_ReadBit (void) {
    uint8_t bit = 0;

    OW_set_PORT_OUT();
    SETOWPIN (0);
    Delay_Us (2);  // Импульс старта чтения

    OW_set_PORT_IN();
    Delay_Us (8);  // Ждем строб (sampling window)

    if (READDOW())
        bit = 1;

    Delay_Us (50);  // Доигрываем слот до конца
    return bit;
}
// ------------------------------------------------------------------------------------------------
// Инициализация шины 1-WARE
uint8_t OneWare_Reset() {

    uint8_t device_found = 0;

    OW_set_PORT_IN();
    SETOWPIN (0);

    OW_set_PORT_OUT();
    Delay_Us (500);

    OW_set_PORT_IN();
    Delay_Us (65);

    device_found = READDOW();

    Delay_Us (450);

    return device_found;
}
//----------------------------------------------------------------------------------
/*
void OneWare_WriteData(uint8_t Data) {
    for (uint8_t i = 0; i < 8; i++) {
        OneWare_WriteBit(Data & 0x01);
        Data >>= 1;
    }
}
//----------------------------------------------------------------------------------
uint8_t OneWare_ReadData(void) {
    uint8_t result = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (OneWare_ReadBit()) {
            result |= (1 << i);
        }
    }
    return result;
}
*/
//----------------------------------------------------------------------------------
// Передача данных по шине 1-WARE
void OneWare_WriteData (uint8_t Data) {

    for (uint8_t i = 0; i < 8; i++) {
        OW_set_PORT_OUT();
        SETOWPIN (0);  // Начало слота (всегда LOW)

        if (Data & 0x01) {
            // ПЕРЕДАЧА "1"
            // На 24МГц Delay_Us(1) может быть слишком мало, а 10 - много.
            // Нужно попасть в окно 1-15 мкс.
            Delay_Us (2);
            OW_set_PORT_IN();  // ОТПУСКАЕМ СРАЗУ
            Delay_Us (60);
        } else {
            // ПЕРЕДАЧА "0"
            Delay_Us (60);     // Держим почти весь слот
            OW_set_PORT_IN();  // Отпускаем
            Delay_Us (2);
        }
        Data >>= 1;
        Delay_Us (5);  // RECOVERY TIME (Без этого датчик сходит с ума)
    }
}
//----------------------------------------------------------------------------------
// Чтение данных с шины 1-WARE
uint8_t OneWare_ReadData() {

    uint8_t result = 0;

    for (uint8_t i = 0; i < 8; i++) {

        OW_set_PORT_OUT();  // прижимаем линию
        SETOWPIN (0);
        Delay_Us (2);

        OW_set_PORT_IN();
        Delay_Us (8);

        result = (READDOW() << i) | result;

        Delay_Us (50);  // ждем до положенного времени
    }

    return result;
}
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------