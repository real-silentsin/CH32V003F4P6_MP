/********************************** (C) COPYRIGHT *******************************
 * File Name          : EC11Lib.c
 * Author             : vantr
 * Version            : V1.0.0
 * Date               : 2025/12/20
 * Description        : Библиотека энкодера EC11
 *********************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "EC11Lib.h"
//  -----------------------------------------------------------------------------
uint8_t enc_state = 0;
uint8_t enc_value = 0;
uint8_t enc_button = 0;
//  -----------------------------------------------------------------------------
// Инициализация портов и режимов для работы энкодера
void EC11_Init(void) {
    // -------------------------------------------------------------------------
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    // -------------------------------------------------------------------------
    // Включаем тактирование задействованных портов
    RCC_APB2PeriphClockCmd (RCC_APB2Periph_AFIO, ENABLE); // Обязательно для прерываний!
    RCC_APB2PeriphClockCmd (ENC_EC11_A.rcc | ENC_EC11_B.rcc | ENC_EC11_BUTTON.rcc, ENABLE);
    // -------------------------------------------------------------------------
    // Инициализация линии A
    GPIO_InitStructure.GPIO_Pin = ENC_EC11_A.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (ENC_EC11_A.port, &GPIO_InitStructure);
    // -------------------------------------------------------------------------
    // Инициализация линии B
    GPIO_InitStructure.GPIO_Pin = ENC_EC11_B.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (ENC_EC11_B.port, &GPIO_InitStructure);
    // -------------------------------------------------------------------------
#ifdef ENC_EC11_USE_BUTTON
    // Инициализация линии Button
    GPIO_InitStructure.GPIO_Pin = ENC_EC11_BUTTON.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (ENC_EC11_BUTTON.port, &GPIO_InitStructure);
#endif
    // -------------------------------------------------------------------------
    // Инициализация прерываний
    /* GPIOx ----> ENC_EC11_EXTI_LINE */
    GPIO_EXTILineConfig (ENC_EC11_A.exti_port_src, ENC_EC11_A.exti_pin_src);
    EXTI_InitStructure.EXTI_Line = ENC_EC11_A.exti_line;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init (&EXTI_InitStructure);
    // -------------------------------------------------------------------------
    /* ENC_EC11_EXTI_LINE ----> EXTI7_0_IRQn */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI7_0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init (&NVIC_InitStructure);
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Получение состояния вращения энкодера, обращение сбрасывает состояние!
uint8_t EC11_GetEncoderRotationState (void) { 
    
    uint8_t state_temp = enc_state;
    enc_state = ENC_EC11_TICK_NONE;

    return state_temp;
}
// -----------------------------------------------------------------------------
// Получение значения счетчика энкодера
uint8_t EC11_GetEncoderValue (void) { return enc_value; }
// -----------------------------------------------------------------------------
// Установка счетчика энкодера
void EC11_SetEncoderValue (uint8_t new_value) { enc_value = new_value; }
// -----------------------------------------------------------------------------
// Получение состояния кнопки энкодера
uint8_t EC11_GetEncoderButtonState (void) {

    return GPIO_ReadInputDataBit (ENC_EC11_BUTTON.port, ENC_EC11_BUTTON.pin);
}
// -----------------------------------------------------------------------------
// Прерывания
// -----------------------------------------------------------------------------
void EXTI7_0_IRQHandler (void) __attribute__ ((interrupt ("WCH-Interrupt-fast")));
// -----------------------------------------------------------------------------
// Обработчик прерываний от линии A энкодера
void EXTI7_0_IRQHandler (void) {
    // -------------------------------------------------------------------------
    if (EXTI_GetITStatus (ENC_EC11_A.exti_line) != RESET) {
        // ---------------------------------------------------------------------
        // Запретить прерывание для линии
        EXTI->INTENR &= ~ENC_EC11_A.exti_line;
        // ---------------------------------------------------------------------
        // Уровень на выводе энкодера "A" должен быть лог. 0
        if (GPIO_ReadInputDataBit(ENC_EC11_A.port, ENC_EC11_A.pin) == 0) {

            // Задержка подбирается по четкости срабатывания вращения
            // энкодера против часовой стрелки!!!
            // Цикл "тика" энкодера в норме ~20мСек, значит
            // допустимые параметры задержки [1..10]мСек.
            Delay_Ms(1);

            if (GPIO_ReadInputDataBit(ENC_EC11_B.port, ENC_EC11_B.pin) == 0) {
                // Если уровень на выводе энкодера "B" низкий, значит
                // энкодер вращается по часовой стрелке
                enc_state = ENC_EC11_TICK_CWISE;
                if (enc_value < 255) { enc_value += 1; }
            } else {
                // Если уровень на выводе энкодера "B" высокий, значит
                // энкодер вращается против часовой стрелки
                enc_state = ENC_EC11_TICK_CCWISE;
                if (enc_value > 0) { enc_value -= 1; }
            }
        }
        // ---------------------------------------------------------------------
        // Задержка до окончания цикла (пока ENC A не станет 1)
        while (GPIO_ReadInputDataBit(ENC_EC11_A.port, ENC_EC11_A.pin) == 0) { Delay_Ms(5); }
        // ---------------------------------------------------------------------
        // Очистка флага прерывания
        EXTI_ClearITPendingBit (ENC_EC11_A.exti_line);
        // ---------------------------------------------------------------------
        // Включить прерывания по линии портов
        EXTI->INTENR |= ENC_EC11_A.exti_line;
        // ---------------------------------------------------------------------
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------