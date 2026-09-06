/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_SPIM3_grlib1.c
 * Author             : vantr
 * Description        : Драйвер для TFT дисплея SPIM3 (графика)
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// ----------------------------------------------------------------------------
#include "ch32v00x.h"
#include "sss_spim3_lib.h"
// ----------------------------------------------------------------------------
// Инициализация механизма SPI
void SPIM3_SPI_Init (void) {

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef SPI_InitStructure = {0};

    // 1. Включаем тактирование (здесь ИЛИ использовать можно, так как это просто маски регистров)
    RCC_APB2PeriphClockCmd (SPIHW_MOSI_PORT.rcc | SPIHW_SCK_PORT.rcc | RCC_APB2Periph_SPI1, ENABLE);

    // Настраиваем общие параметры для обоих пинов
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // 2. Инициализируем пин SCK (индивидуально его порт и его пин)
    GPIO_InitStructure.GPIO_Pin = SPIHW_SCK_PORT.pin;
    GPIO_Init (SPIHW_SCK_PORT.port, &GPIO_InitStructure);

    // 3. Инициализируем пин MOSI (индивидуально его порт и его пин)
    GPIO_InitStructure.GPIO_Pin = SPIHW_MOSI_PORT.pin;
    GPIO_Init (SPIHW_MOSI_PORT.port, &GPIO_InitStructure);

#ifdef SPIHW_FULL_DUPLEX
    // PA6 (MISO) - обычный вход
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = SPIHW_MISO_PORT.pin;
    GPIO_Init (SPIHW_MISO_PORT.port, &GPIO_InitStructure);

    // Настройка SPI1
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // Полный дуплекс
#else
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;  // Только передача
#endif
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                         // Mode 3 (обычно для SPIM3)
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                        // Mode 3
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
#ifdef USE_OLED_SSD1306
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;  // Скорость для флешки /4
#else
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;  // Скорость для флешки /2
#endif
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init (SPIHW_DEVICE, &SPI_InitStructure);

    // 8. КЛЮЧЕВОЙ МОМЕНТ: устанавливаем SSI (Internal Slave Select)
    // Это "обманывает" контроллер, заставляя его думать, что на NSS высокий уровень,
    // при этом физический пин PC4 остается свободным для I2C SDA.
    SPI_NSSInternalSoftwareConfig (SPIHW_DEVICE, SPI_NSSInternalSoft_Set);

    SPI_Cmd (SPIHW_DEVICE, ENABLE);
}
// ----------------------------------------------------------------------------
#pragma region Зависимые от передатчика методы
// ----------------------------------------------------------------------------
// Отправка байта (с ожиданием)
void SPIM3_SendByte (uint8_t data) {

    uint8_t tmp = SPIM3_ExchangeByte (data);
    (void)tmp;

    /*while (!(SPIHW_DEVICE->STATR & SPI_STATR_TXE));  // Ждем, пока буфер освободится
    SPIHW_DEVICE->DATAR = data;                      // Пишем байт
    while (SPIHW_DEVICE->STATR & SPI_STATR_BSY);     // ЖДЕМ, пока байт физически вылетит */
}
// ----------------------------------------------------------------------------
// Ожидание освобождения шины
void SPIM3_WaitForBusy() {

    while (SPI_I2S_GetFlagStatus (SPIHW_DEVICE, SPI_I2S_FLAG_BSY) == SET);
}
// ----------------------------------------------------------------------------
// Очистка регистра приема от паразитных значений перед пакетным чтением
void SPIM3_ClearReg() {

    while (SPIHW_DEVICE->STATR & SPI_STATR_RXNE) {
        volatile uint32_t dummy_read = SPIHW_DEVICE->DATAR;
        (void)dummy_read;
    }

    // 2. Сбрасываем флаг переполнения OVR, который набился от работы с SSD1306.
    // По даташиту WCH он сбрасывается чтением регистра STATR, а затем чтением DATAR.
    if (SPIHW_DEVICE->STATR & SPI_STATR_OVR) {
        volatile uint32_t status_dummy = SPIHW_DEVICE->STATR;
        volatile uint32_t data_dummy = SPIHW_DEVICE->DATAR;
        (void)status_dummy;
        (void)data_dummy;
    }
}
// ----------------------------------------------------------------------------
#ifdef SPIHW_FULL_DUPLEX
    // Обмен - отправка байта с одновременным приемом
    uint8_t SPIM3_ExchangeByte (uint8_t data) {

    while (SPI_I2S_GetFlagStatus (SPIHW_DEVICE, SPI_I2S_FLAG_TXE) == RESET);

    SPIHW_DEVICE->DATAR = data;
    while (SPI_I2S_GetFlagStatus (SPIHW_DEVICE, SPI_I2S_FLAG_RXNE) == RESET);

    return SPI_I2S_ReceiveData (SPIHW_DEVICE);
}
#endif
// ----------------------------------------------------------------------------
#pragma endregion
// ----------------------------------------------------------------------------
