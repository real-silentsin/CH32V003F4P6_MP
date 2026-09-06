/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_pwm4ch.c
 * Author             : vantr
 * Description        : Библиотека для работы с PWM, 4 канала, TIM2
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "ch32v00x.h"
#include "ch32v00x_gpio.h"
// -----------------------------------------------------------------------------
#include "sss_pwm4ch.h"
// -----------------------------------------------------------------------------
uint16_t arr_private_init = 0;
// -----------------------------------------------------------------------------
// Инициализация PWM для выбранных каналов
void SSS_PWM_Init(uint16_t arr, uint16_t psc) {

    arr_private_init = arr;

    GPIO_InitTypeDef GPIO_InitStructure = {0}; 
    TIM_OCInitTypeDef TIM_OCInitStructure = {0}; 
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0}; 
 
    // 1. Включение тактирования TIM2 и портов 
    // Внимание: TIM2 находится на шине APB1 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // 2. Настройка GPIO (PD4, PD3, PC0, PC1) 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 

#ifdef PWM_USE_CH_1
    RCC_APB2PeriphClockCmd (PWM1_PORT.rcc, ENABLE);
    GPIO_InitStructure.GPIO_Pin = PWM1_PORT.pin;
    GPIO_Init (PWM1_PORT.port, &GPIO_InitStructure);  // CH1
#endif

#ifdef PWM_USE_CH_2
    RCC_APB2PeriphClockCmd (PWM2_PORT.rcc, ENABLE);
    GPIO_InitStructure.GPIO_Pin = PWM2_PORT.pin;
    GPIO_Init (PWM2_PORT.port, &GPIO_InitStructure);  // CH2
#endif

#ifdef PWM_USE_CH_3
    RCC_APB2PeriphClockCmd (PWM3_PORT.rcc, ENABLE);
    GPIO_InitStructure.GPIO_Pin = PWM3_PORT.pin;
    GPIO_Init (PWM3_PORT.port, &GPIO_InitStructure);  // CH3
#endif

#ifdef PWM_USE_CH_4
    RCC_APB2PeriphClockCmd (PWM4_PORT.rcc, ENABLE);
    GPIO_InitStructure.GPIO_Pin = PWM4_PORT.pin;
    GPIO_Init (PWM4_PORT.port, &GPIO_InitStructure);  // CH1
#endif
 
    // 3. Базовая настройка таймера 
    TIM_TimeBaseInitStructure.TIM_Period = arr; 
    TIM_TimeBaseInitStructure.TIM_Prescaler = psc; 
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; 
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure); 
 
    // 4. Настройка каналов PWM 
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 
    TIM_OCInitStructure.TIM_Pulse = 0; 
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 
 
#ifdef PWM_USE_CH_1
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
#endif

#ifdef PWM_USE_CH_2
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
#endif

#ifdef PWM_USE_CH_3
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
#endif

#ifdef PWM_USE_CH_4
    TIM_OC4Init(TIM2, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);
#endif

    TIM_ARRPreloadConfig(TIM2, ENABLE); 
 
    // 5. Запуск таймера 
    TIM_Cmd(TIM2, ENABLE); 
}
// -----------------------------------------------------------------------------
// Рассчет коэффициента заполнения PWM, используя стартовые параметры PWM
uint16_t TIM2_GetDutyValue(uint8_t percents) {

    return (uint16_t)((percents * arr_private_init) / 100);
}
// -----------------------------------------------------------------------------
// Установка коэффициента заполнения PWM указанного канала
void TIM2_SetDuty_CH(uint8_t channel, uint8_t duty_percents) {

    uint16_t duty_value = TIM2_GetDutyValue(duty_percents);

    switch(channel) {
#ifdef PWM_USE_CH_1
        case 1: TIM_SetCompare1(TIM2, duty_value); break;
#endif
#ifdef PWM_USE_CH_2
        case 2: TIM_SetCompare2(TIM2, duty_value); break;
#endif
#ifdef PWM_USE_CH_3
        case 3: TIM_SetCompare3(TIM2, duty_value); break;
#endif
#ifdef PWM_USE_CH_4
        case 4: TIM_SetCompare4(TIM2, duty_value); break;
#endif
    }
}
// -----------------------------------------------------------------------------
#ifdef PWM_USE_CH_1
void SSS_SetDuty_CH1(uint8_t duty_percents) {

    TIM2_SetDuty_CH(1, duty_percents);
}
#endif
// -----------------------------------------------------------------------------
#ifdef PWM_USE_CH_2
void SSS_SetDuty_CH2(uint8_t duty_percents) {

    TIM2_SetDuty_CH(2, duty_percents);
}
#endif
// -----------------------------------------------------------------------------
#ifdef PWM_USE_CH_3
void SSS_SetDuty_CH3(uint8_t duty_percents) {

    TIM2_SetDuty_CH(3, duty_percents);
}
#endif
// -----------------------------------------------------------------------------
#ifdef PWM_USE_CH_4
void SSS_SetDuty_CH4(uint8_t duty_percents) {

    TIM2_SetDuty_CH(4, duty_percents);
}
#endif
// -----------------------------------------------------------------------------