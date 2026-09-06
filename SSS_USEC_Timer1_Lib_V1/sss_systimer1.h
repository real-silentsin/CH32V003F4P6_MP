// -----------------------------------------------------------------------------
// Библиотека для измерения временных интервалов (до 65 мСек)
// ВНИМАНИЕ! Используется таймер TIM1!
// (c) vantr
// -----------------------------------------------------------------------------
#ifndef __SSS_SYSTMR1_H__
#define __SSS_SYSTMR1_H__
// -----------------------------------------------------------------------------
#include <stdint.h>
#include "ch32x035.h"
// -----------------------------------------------------------------------------
// Максимальное значение времени активности таймера (мкСек) ~65 мСек
#define TIMER_MAX_COUNT 65535
// -----------------------------------------------------------------------------
// Использование софтверного прерывания по окончанию отсчета
//#define SYSTIMER1_USE_SORTIRQ
// -----------------------------------------------------------------------------
// Инициализация TIM3 в режиме одного импульса для отсчета микросекунд
void SysTimer1_Init(void) 
{ 
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0}; 
 
    // 1. Включение тактирования TIM1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); 
 
    // 2. Настройка базы времени 
    // SystemCoreClock автоматически подхватывает частоту (24 или 48 МГц) 
    // Предделитель: (частота / 1 000 000) - 1 даст 1 тик = 1 мкс 
    TIM_TimeBaseStructure.TIM_Prescaler = (SystemCoreClock / 1000000) - 1; 
     
    // Период (ARR): максимальный (65535), чтобы таймер мог считать до ~65 мс 
    // Вы можете изменить это значение на нужное число мкс 
    TIM_TimeBaseStructure.TIM_Period = 65535; 
     
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
     
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); 
 
    // 3. Включение режима One-Pulse Mode (OPM) 
    // После достижения значения Period таймер автоматически сбросит бит CEN (остановится) 
    TIM_SelectOnePulseMode(TIM1, TIM_OPMode_Single);

#ifdef SYSTIMER1_USE_SORTIRQ
    // Включение прерывания по событию обновления (Update Interrupt)
    TIM_ITConfig (TIM1, TIM_IT_Update, ENABLE);

    // Настройка NVIC/PFIC для TIM1
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init (&NVIC_InitStructure);

    // 2. Включаем софтверное прерывание в PFIC
    NVIC_EnableIRQ(Software_IRQn);
    // Ставим средний приоритет
    NVIC_SetPriority (Software_IRQn, 0x80);

    __enable_irq();
#endif
} 
// ----------------------------------------------------------------------------- 
// Функция запуска таймера с нуля на заданное количество микросекунд
void SysTimer1_Start(uint16_t micros) 
{ 
    if (micros == 0) return; 
     
    TIM_Cmd(TIM1, DISABLE);            // На всякий случай выключаем 
    TIM_SetAutoreload(TIM1, micros);   // Устанавливаем предел счета 
    TIM_SetCounter(TIM1, 0);           // Сбрасываем счетчик в 0 
    TIM_Cmd(TIM1, ENABLE);             // Запускаем 
}
// -----------------------------------------------------------------------------
// Проверка: работает ли еще таймер? (0 - остановился, 1 - еще считает)
uint8_t SysTimer1_IsRunning(void) 
{ 
    return (TIM1->CTLR1 & TIM_CEN) ? 1 : 0; 
}
// -----------------------------------------------------------------------------
// Получение длительности с момента запуска в мкСек
uint16_t SysTimer1_GetUs (void) { return TIM_GetCounter (TIM1); }
// -----------------------------------------------------------------------------
#ifdef SYSTIMER1_USE_SORTIRQ
// ----------------------------------------------------------------------------
#pragma region Переносимый текст
// ----------------------------------------------------------------------------
/* // 1. Обработчик прерывания (его можно перенести в любое место вашего проекта)
// Используем Software_IRQn (номер 26 в таблице векторов)
// Имя обработчика смотреть в файле startup_ch32v00x.S!!!
void SW_Handler(void) __attribute__ ((interrupt ("WCH-Interrupt-fast")));
// ----------------------------------------------------------------------------
// Собственно, тело обработчика программного прерывания
void SW_Handler(void) {
    // ------------------------------------------------------------------------
    // Здесь производим различные действия
    printf ("This is a software interrupt!");
    // ------------------------------------------------------------------------
    // Сброс флага
    // NVIC_ClearPendingIRQ (Software_IRQn);
    // ------------------------------------------------------------------------
} */
// ----------------------------------------------------------------------------
#pragma endregion
// ----------------------------------------------------------------------------
// 1. Обработчик прерывания (его можно перенести в любое место вашего проекта)
// Используем Software_IRQn (номер 26 в таблице векторов)
// Имя обработчика смотреть в файле startup_ch32x03x.S!!!
void TIM1_UP_IRQHandler(void) __attribute__ ((interrupt ("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void) {
    if (TIM_GetITStatus (TIM1, TIM_IT_Update) != RESET) {

        // Сброс флага прерывания
        TIM_ClearITPendingBit (TIM1, TIM_IT_Update);

        // Активируем прерывание программно через установку Pending бита
        // Формула: номер прерывания в позицию бита
        // Устанавливает флаг прерывания "в ожидание"
        NVIC_SetPendingIRQ (Software_IRQn);
    }
}
#endif
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------