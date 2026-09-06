// -----------------------------------------------------------------------------
#include "ch32v00x.h"
#include "sss_soundLib.h"
#include "SSS_W25Qxx_Lib_1/w25qxx.h"

#include <string.h>
// -----------------------------------------------------------------------------
// Звуковой ШИМ-сигнал выводится на пин PD4 (TIM2, Канал 1, выв 1)
// -----------------------------------------------------------------------------
// Использовать программный дополнительный фильтр нижних частот
//#define USE_PRG_FLC
// -----------------------------------------------------------------------------
// Использовать "инверсный" пересчет выводимых данных
//#define USE_INVERSE_OUTPUT
// -----------------------------------------------------------------------------
#ifdef USE_PRG_FLC
volatile uint8_t prev_sample = 128;  // Для программного ФНЧ
#endif
// -----------------------------------------------------------------------------
uint8_t libIsInit = 0;          // Флаг инициализации библиотеки
uint8_t currentVolume = 16;     // Текущая громкость [0..16]
// -----------------------------------------------------------------------------
/* // Таблица беззнакового синуса (тишина на 128)
const uint8_t sine_table[20] = {
    128, 167, 203, 232, 250, 255, 250, 232, 203, 167,
    128, 89, 53, 24, 6, 0, 6, 24, 53, 89};

volatile uint8_t sine_ptr = 0; */
// -----------------------------------------------------------------------------
// Настройка Таймера 2 для генерации несущей частоты ШИМ (8 бит, ~187 кГц)
void Init_PWM_Timer2 (void) {

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};

    // 1. Включаем тактирование (Таймер 2 — APB1, Порт D и AFIO — APB2)
    RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd (SND_OUT_PORT.rcc | RCC_APB2Periph_AFIO, ENABLE);

    // 2. Настраиваем физический пин PD4 на вывод ШИМ
    GPIO_InitStructure.GPIO_Pin = SND_OUT_PORT.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // Альтернативная функция Push-Pull
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (SND_OUT_PORT.port, &GPIO_InitStructure);

    // 3. Настройка базы таймера (период 255)
    TIM_TimeBaseStructure.TIM_Period = 255;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit (TIM2, &TIM_TimeBaseStructure);

    // 4. Конфигурация Канала 1 (TIM2_CH1 на PD4) в режим ШИМ
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 128;       // Середина шкалы (тишина)
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init (TIM2, &TIM_OCInitStructure);  // Строго OC1

    // Отключаем кеширование прелоада для мгновенного обновления звука
    TIM_OC1PreloadConfig (TIM2, TIM_OCPreload_Disable);
    TIM_UpdateDisableConfig (TIM2, DISABLE);

    TIM_ARRPreloadConfig (TIM2, ENABLE);

    // Принудительно генерируем событие обновления, чтобы протолкнуть настройки в регистры
    TIM_GenerateEvent (TIM2, TIM_EventSource_Update);

    // Запуск таймера
    TIM_Cmd (TIM2, ENABLE);
}

// -----------------------------------------------------------------------------
// Установка громкости (в процентах) [0..100]
void Play_SetVolume(uint8_t value_percent) {

    if (value_percent > 100)
        value_percent = 100;

    // Быстрый перевод 0..100 в индекс 0..10 без вызова тяжелой функции деления.
    // (value_percent * 205) >> 11 — это математически точное деление на 10 для этого диапазона
    uint8_t index = (uint8_t)((value_percent * 205) >> 11);

    // Логарифмическая шкала коэффициентов (из масштаба 16) для плавного восприятия на слух.
    // Больше никаких скачков и кривизны!
    static const uint8_t volume_scale[11] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16};

    // Мгновенное присвоение
    currentVolume = volume_scale[index];
}
// -----------------------------------------------------------------------------
// Подстройка громкости семпла под уставку пользователя [0..8]
uint8_t Play_AjustSampleVolume(uint8_t sample_data) {

    // 1. Быстрые проверки крайних состояний
    if (currentVolume == 0)
        return 128;          // Идеальная тишина
    if (currentVolume == 16)
        return sample_data;  // Исходный чистый звук

    uint32_t vol = currentVolume;
    uint32_t result;

    // 2. Если волна идет ВВЕРХ от центра тишины (128..255)
    if (sample_data >= 128) {
        uint32_t amplitude = sample_data - 128;
        // Масштабируем амплитуду с правильным округлением (+8 перед делением на 16)
        result = 128 + (((amplitude * vol) + 8) >> 4);
    }
    // 3. Если волна идет ВНИЗ от центра тишины (0..127)
    else {
        uint32_t amplitude = 128 - sample_data;
        // Масштабируем амплитуду и вычитаем ее из центра
        result = 128 - (((amplitude * vol) + 8) >> 4);
    }

    return (uint8_t)result;
}
// -----------------------------------------------------------------------------
// Метод, принимающий данные от Flash памяти
uint8_t W25_OnFileReadCH3 (const uint32_t address, const uint16_t segment_num, const uint8_t *data_chunk, const uint16_t len) {

    (void)address;
    (void)segment_num;

    if (data_chunk != NULL && len > 0) {

        for (uint16_t i = 0; i < len; i++) {

            // 1. Выводим сэмпл в ШИМ (TIM2_CH1)
            AUDIO_PWM_REG = Play_AjustSampleVolume (data_chunk[i]);
            Delay_Us (SOUND_FM_DELAY_US);
        }        
    }

    // Возвращаем СТРОГО 0, чтобы библиотека продолжала чтение
    return 0;
}
// -----------------------------------------------------------------------------
// Инициализация звука
void Play_SoundInit() {

    Delay_Ms(1000);

    Init_PWM_Timer2();
    libIsInit = 1;
}
// -----------------------------------------------------------------------------
// Проигрывать файл с указанным ID
void Play_SoundFromFlash (uint16_t file_id) {

    // Ленивая инициализация звука
    if (libIsInit == 0) {

        Init_PWM_Timer2();
        libIsInit = 1;
    }

    // 2. Просто запускаем функцию библиотеки
    W25_ReadFileByIDEx (file_id, W25_OnFileReadCH3);
}

// -----------------------------------------------------------------------------
// Функция для проигрывания ЛЮБОГО буфера из ОЗУ
void Play_SoundFromBuffer (const uint8_t *buffer, uint16_t length) {

    // Ленивая инициализация звука
    if (libIsInit == 0) {

        Init_PWM_Timer2();
        libIsInit = 1;
    }

    W25_OnFileReadCH3(0, 0, buffer, length);
}
// -----------------------------------------------------------------------------
// Проигрывать файл с указанным ID из непрерывной области
void Play_SoundContFromFlash (uint16_t file_id) {

    // 1. Ленивая инициализация звука
    if (libIsInit == 0) {

        Init_PWM_Timer2();
        libIsInit = 1;
    }

    // 2. Запускаем чтение
    W25_ReadContFileByIDEx (file_id, W25_OnFileReadCH3);
}
// -----------------------------------------------------------------------------
