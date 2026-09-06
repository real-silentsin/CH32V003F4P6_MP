/********************************** (C) COPYRIGHT ******************************
 * File Name          : uart_lib.c
 * Author             : vantr
 * Description        : Библиотека для работы с UART CH32X033
 *******************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// ----------------------------------------------------------------------------
#include "app_config.h"
#include "uart_lib.h"

#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_Common_Lib_V1/sss_math.h"
#include "SSS_CRC8_Lib_V1/crc8_Lib_V1.h"
#include "SSS_XProto_Lib4/sss_xproto_lib1.h"
// ----------------------------------------------------------------------------
//  Последний статус приемника
//  ВНИМАНИЕ! Используются не все доступные состояния, а только те,
//  которые управляют очередью приема символов с шины UART
static volatile uint8_t uart_lastState = UART_COM_READY;
// ----------------------------------------------------------------------------
// Инициализация механизма USART
void UART_UNIT_Init(uint32_t baudrate) {
    // ------------------------------------------------------------------------
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    // ------------------------------------------------------------------------
    // 1. Включаем тактирование (обязательно раздельно или через ИЛИ)
    RCC_APB2PeriphClockCmd (USART_TX_PORT.rcc | USART_RX_PORT.rcc | USART_RCC, ENABLE);

    // 2. Настройка TX (PD5)
    // ВАЖНО: AF_PP — это то, что оживили ваш вывод
    GPIO_InitStructure.GPIO_Pin = USART_TX_PORT.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (USART_TX_PORT.port, &GPIO_InitStructure);

    // 3. Настройка RX (PD6)
    GPIO_InitStructure.GPIO_Pin = USART_RX_PORT.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init (USART_RX_PORT.port, &GPIO_InitStructure);
    // ------------------------------------------------------------------------
    // 4. Параметры UART
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    // ------------------------------------------------------------------------
    USART_Init (USART_DEV, &USART_InitStructure);
    // ------------------------------------------------------------------------
    // 5. Включаем модуль
    USART_Cmd (USART_DEV, ENABLE);
    // ------------------------------------------------------------------------
    // 5. Настройка NVIC
    // Для CH32V003 лучше использовать простую схему приоритетов
    NVIC_InitStructure.NVIC_IRQChannel = USART_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  // Средний приоритет
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;         // В V003 почти не влияет
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init (&NVIC_InitStructure);
    // ------------------------------------------------------------------------
    // Включаем прерывание именно по приему байта (RX Not Empty)
    USART_ITConfig (USART_DEV, USART_IT_RXNE, ENABLE);
    // ------------------------------------------------------------------------
    // Разрешаем прерывания на уровне ядра RISC-V
    NVIC_EnableIRQ (USART_IRQ);
    // ------------------------------------------------------------------------
    UART_Reset();
    // ------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Получение последнего состояния приемника команд
uint8_t UART_getLastState() {

    return uart_lastState;
}

// -----------------------------------------------------------------------------
// Сброс и подготовка к новому приему
void UART_Reset() {

    uart_lastState = UART_COM_READY;
}
// -----------------------------------------------------------------------------
// Остановка приема
void UART_Stop() {

    uart_lastState = UART_COM_STOPPED;
}
// -----------------------------------------------------------------------------
// Начало нового приема (после остановки "UART_Stop")
void UART_Start() {

    UART_Reset(); //?
    uart_lastState = UART_COM_READY;
}
// -----------------------------------------------------------------------------
// Отправка байта
void UART_SendChar(const uint8_t data) {

    while (USART_GetFlagStatus (USART_DEV, USART_FLAG_TXE) == RESET) { }
    USART_SendData (USART_DEV, data);
}
// ----------------------------------------------------------------------------
// Отправка строки
void UART_SendString(const uint8_t *str) {

    while(*str) { UART_SendChar(*str++); }
    UART_SendChar (UART_ENDOFLINE);
}
// -----------------------------------------------------------------------------
// Безопасная передача строки символов (\0 игнорируется)
// Метод не завершает передачу!
uint16_t UART_TxString2(const uint8_t *ptr_string, uint8_t length) {
    // -------------------------------------------------------------------------
    uint16_t result = 0;

    for (int index = 0; index < length; index++) {

        if (ptr_string[index] != 0) {

            UART_SendChar (ptr_string[index]);
            result += 1;
        } else {

            UART_SendChar ('0');
        }
    }
    // -------------------------------------------------------------------------
    return result;
}

// -----------------------------------------------------------------------------
// Печать в канал UART "OK"
void UART_print_ok() {

    UART_SendString((uint8_t *)"OK");
    UART_SendChar (UART_ENDOFLINE);
}

// -----------------------------------------------------------------------------
// Печать в канал UART "<CR>"
void UART_print_crlf() { 

    UART_SendChar(UART_ENDOFLINE);
}
// -----------------------------------------------------------------------------
// Объявляем "слабую" функцию-обработчик на один символ
__attribute__ ((weak)) uint8_t UART_OnReceive (uint8_t pData) {
    // По умолчанию ничего не делаем
    (void)pData;

    return UART_COM_READY;
}

// ----------------------------------------------------------------------------
// Обработчик прерывания
void USART_HANDLER (void) __attribute__ ((interrupt ("WCH-Interrupt-fast")));

void USART_HANDLER (void) {

    if (USART_GetITStatus (USART_DEV, USART_IT_RXNE) != RESET) {

        if (uart_lastState == UART_COM_READY) {

            uint8_t data = USART_ReceiveData (USART_DEV);
            uart_lastState = UART_OnReceive(data);

            // Эхо
            //while (USART_GetFlagStatus (USART_DEV, USART_FLAG_TXE) == RESET) { }
            //USART_SendData (USART_DEV, data);
        }

        USART_ClearITPendingBit (USART_DEV, USART_IT_RXNE);
    }
}
// ----------------------------------------------------------------------------
