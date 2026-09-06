/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_fastGpio_lib1.с
 * Author             : vantr
 * Target             : CH32V003
 * Description        : Быстрый доступ к пинам
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "sss_fastGpio_lib1.h"
#include "string.h"
// -----------------------------------------------------------------------------
// Макросы для ультрабыстрой работы, если пин передается как указатель
#define FGPIO_LOW(p) ((p)->port->BCR = (p)->pin)
#define FGPIO_HIGH(p) ((p)->port->BSHR = (p)->pin)
#define FGPIO_READ(p) (((p)->port->INDR & (p)->pin) ? 1 : 0)
// -----------------------------------------------------------------------------
// Проверка на валидность данных пина
uint8_t fgpio_pinDefined (const PinDefine_t *pin) {

    // Если указатель на структуру существует и порт внутри неё не NULL
    return (pin != NULL && pin->port != NULL) ? 1 : 0;
}
// -----------------------------------------------------------------------------
// Открыть пин для работы с ним
void fgpio_openPin (const PinDefine_t *pin, GPIOMode_TypeDef mode) {
 
    if (!pin) { return; }

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd (pin->rcc, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = mode;
    GPIO_InitStructure.GPIO_Pin = pin->pin;

    GPIO_Init (pin->port, &GPIO_InitStructure);
}
// -----------------------------------------------------------------------------
// Установить на пине указанное значение (0/1)
void fgpio_setPin (const PinDefine_t *pin, uint8_t level) {

    if (fgpio_pinDefined (pin)) {
        if (level == 0) {

            FGPIO_LOW (pin);
        } else {

            FGPIO_HIGH (pin);
        }
    }
}
// -----------------------------------------------------------------------------
// Прочитать из пина значение (0/1) - или 0xFF, если невозможно
uint8_t fgpio_getPin (const PinDefine_t *pin) {

    if (fgpio_pinDefined (pin)) {

        return FGPIO_READ (pin);
    }
    return 0xFF;
}
// -----------------------------------------------------------------------------
