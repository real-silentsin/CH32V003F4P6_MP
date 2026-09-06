/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_spisw_lib1.c
 * Author             : vantr
 * Description        : Драйвер софтверного SPI (CH32X033)
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// ----------------------------------------------------------------------------
#include "ch32x035.h"
#include "sss_spisw_lib1.h"
// ----------------------------------------------------------------------------
#ifdef SPISW_USE_DELAY
// Количество итераций пустого цикла (одна итерация ~5-6 тактов процессора)
static volatile uint32_t spi_delay_cycles = 0;
// ----------------------------------------------------------------------------
// ПРАВИЛЬНЫЙ МАКРОС: теперь он содержит и заголовок цикла, и тело (пустое)
#define SPI_WAIT() \
    for (volatile uint32_t d = 0; d < spi_delay_cycles; d++) { __NOP(); }

void SPISW_SetSpeed (uint32_t target_hz) {
    uint32_t ticks = SystemCoreClock / (target_hz * 2);
    // Расчет: (тики - накладные расходы) / такты на один NOP
    spi_delay_cycles = (ticks > 20) ? (ticks - 20) / 6 : 0;
}
#else
// Если задержки выключены — макрос просто "испаряется" из кода
#define SPI_WAIT() (__NOP())
// Заглушка, чтобы вызовы SetSpeed в коде не вызывали ошибок
#define SPISW_SetSpeed(hz) (void)(hz)
#endif
// ----------------------------------------------------------------------------
// 3. Универсальный Transfer
uint8_t SPISW_Transfer (uint8_t data) {
#ifdef SPISW_FULL_DUPLEX
    uint8_t received = 0;
#endif

    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80)
            SPISW_MOSI_HIGH();
        else
            SPISW_MOSI_LOW();
        data <<= 1;

        SPI_WAIT();  // Игнорируется компилятором, если SPISW_USE_DELAY выключен

#ifdef SPISW_MODE_1
        SPISW_SCK_HIGH();
        SPI_WAIT();
        SPISW_SCK_LOW();
#else  // MODE 3
        SPISW_SCK_LOW();
        SPI_WAIT();
        SPISW_SCK_HIGH();
#endif

#ifdef SPISW_FULL_DUPLEX
        received <<= 1;
        if (SPISW_MISO_READ())
            received |= 0x01;
#endif

        SPI_WAIT();
    }

#ifdef SPISW_FULL_DUPLEX
    return received;
#else
    return 0;
#endif
}
// ----------------------------------------------------------------------------
// ИНИЦИАЛИЗАЦИЯ
void SPISW_Init (void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd (SPISW_MOSI_PORT.rcc | SPISW_MISO_PORT.rcc | SPISW_SCK_PORT.rcc, ENABLE);

    // Выходы (Push-Pull)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

    GPIO_InitStructure.GPIO_Pin = SPISW_MOSI_PORT.pin;
    GPIO_Init (SPISW_MOSI_PORT.port, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = SPISW_SCK_PORT.pin;
    GPIO_Init (SPISW_SCK_PORT.port, &GPIO_InitStructure);

#ifdef SPISW_FULL_DUPLEX
    // Вход (Floating)
    GPIO_InitStructure.GPIO_Pin = SPISW_MISO_PORT.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init (SPISW_MISO_PORT.port, &GPIO_InitStructure);
#endif

    // Начальные уровни
#ifdef SPISW_MODE_3
    SPISW_SCK_HIGH();
#else
    SPISW_SCK_LOW();
#endif
}
// ----------------------------------------------------------------------------