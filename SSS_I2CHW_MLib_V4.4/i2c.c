/********************************** (C) COPYRIGHT *******************************
 * File Name          : i2c.c
 * Author             : vantr
 * Description        : Драйвер аппаратного I2C контроллеров CH32V003
 ********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "SSS_Common_Lib_V1/sss_classes.h"
//#include "debug.h"
#include "i2c.h"
// ----------------------------------------------------------------------------
// Последняя ошибка в процессе передачи данных
int i2c_last_error = I2C_ERROR_SUCCESS;
// ----------------------------------------------------------------------------
// Количество принятых/переданных байтов
uint16_t i2c_total_bytes = 0;
// ----------------------------------------------------------------------------
// Инициализация переферии I2C
void IIC_Init(uint16_t address)
{
    RCC_APB1PeriphClockCmd (RCC_APB1Periph_I2C1, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitTStructure = {0};

    I2C_REMAP();

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // SDA
    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PORT.pin;
    GPIO_Init (I2C_SDA_PORT.port, &GPIO_InitStructure);

    // SCL
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PORT.pin;
    GPIO_Init (I2C_SCL_PORT.port, &GPIO_InitStructure);
#ifdef I2C_SPEED_STANDARD_MODE
    I2C_InitTStructure.I2C_ClockSpeed = I2C_SPEED_STANDARD_MODE;
#endif
#ifdef I2C_SPEED_FAST_MODE
    I2C_InitTStructure.I2C_ClockSpeed = I2C_SPEED_FAST_MODE;
#endif
#ifdef I2C_SPEED_FAST_MODE_PLUS
    I2C_InitTStructure.I2C_ClockSpeed = I2C_SPEED_FAST_MODE_PLUS;
#endif
    I2C_InitTStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitTStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitTStructure.I2C_OwnAddress1 = address;
    I2C_InitTStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitTStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init (I2C1, &I2C_InitTStructure);

    I2C_Cmd (I2C1, ENABLE);
}
// ----------------------------------------------------------------------------
// Получение последнего кода ошибки
uint32_t i2c_getLastError(void) {

    return i2c_last_error;
}
// ----------------------------------------------------------------------------
// Получение количества принятых/переданных байтов
uint16_t i2c_getBytesCount(void) {

    return i2c_total_bytes;
}

// ----------------------------------------------------------------------------
// Установка ACK бита
void i2c_setACK (void) {

    I2C1->CTLR1 |= (1 << 10);
}

// ----------------------------------------------------------------------------
// Очистка ACK бита перед чтением регистра DATAR
void i2c_setNACK (void) {

    I2C1->CTLR1 &= ~(1 << 10);
}
// ----------------------------------------------------------------------------
// Ожидание, пока освободятся обе шины (SCL и SDA станут лог "1")
// Возврат: I2C_ERR_SUCCESS или I2C_ERR_BUSY
// Используется конечный цикл ожидания I2C_BUSY_LOOPS
uint32_t i2c_wait_not_busy(void)
{
    uint32_t count = 0;
    
    while ( I2C_GetFlagStatus( I2C1, I2C_FLAG_BUSY ) != RESET) {
        
        if(++count >= I2C_BUSY_LOOPS) { return I2C_ERROR_BUSY; }
    }

    return I2C_ERROR_SUCCESS;
}

// ----------------------------------------------------------------------------
// Ожидание переключения модуля MCU в режим "I2C Master"
// Возврат: I2C_ERR_SUCCESS или I2C_ERROR_TIME_OUT
// Используется конечный цикл ожидания I2C_MASTER_MODE_LOOPS
uint32_t i2c_wait_master_mode(void)
{
    uint32_t count = 0;

    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT ) ) {
        
        if(++count >= I2C_MASTER_MODE_LOOPS) { return I2C_ERROR_TIME_OUT; }
    }

    return I2C_ERROR_SUCCESS;
}

// ----------------------------------------------------------------------------
// Ожидание полной отправки байта модулем I2C MCU (адрес или данные)
// Возврат: I2C_ERR_SUCCESS или I2C_ERROR_TIME_OUT
// Используется конечный цикл ожидания I2C_TRANSMIT_COMPLETE_LOOPS
// I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED : адрес отправлен
// I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED : address sent (переход к приему шины)
// I2C_EVENT_MASTER_BYTE_TRANSMITTED : байт данных отправлен
uint32_t i2c_wait_transmit_complete(void)
{
    uint32_t count = 0;

    while(1) {
        uint32_t i2c_status = I2C_GetLastEvent(I2C1);

        if(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED == i2c_status) break;
        if(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED == i2c_status) break;
        if(I2C_EVENT_MASTER_BYTE_TRANSMITTED == i2c_status) break;
        
        if(++count >= I2C_TRANSMIT_COMPLETE_LOOPS) return I2C_ERROR_TIME_OUT;
    }

    return I2C_ERROR_SUCCESS;
}

// ----------------------------------------------------------------------------
// Ожидание, пока модуль I2C MCU закончит прием байта
// Возврат: I2C_ERR_SUCCESS или I2C_ERROR_TIME_OUT
// Используется конечный цикл ожидания I2C_MASTER_RECEIVER_LOOPS
uint32_t i2c_wait_master_receiver_mode(void)
{
    uint32_t count = 0;

    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED ) ) {
    
        if(++count >= I2C_MASTER_RECEIVER_LOOPS) { return I2C_ERROR_TIME_OUT; }
    }
    
    return I2C_ERROR_SUCCESS;
}

// ----------------------------------------------------------------------------
// Ожидание опустошения передатчика I2C модуля MCU
// Возврат: I2C_ERR_SUCCESS или I2C_ERROR_TIME_OUT
// Используется конечный цикл ожидания I2C_TRANSMIT_EMPTY_LOOPS
uint32_t i2c_wait_transmit_empty(void)
{
    uint32_t count = 0;

    while( I2C_GetFlagStatus( I2C1, I2C_FLAG_TXE ) ==  RESET ) {
        
        if(++count >= I2C_TRANSMIT_EMPTY_LOOPS) { return I2C_ERROR_TIME_OUT; }
    }

    return I2C_ERROR_SUCCESS;
}
// ----------------------------------------------------------------------------
// Метод начинает обмен по шине с введением адреса переферийного устройства, но не заканчивает его.
uint32_t i2c_begin_transaction(uint8_t per_hrdw_address, uint8_t direction) {
    // ------------------------------------------------------------------------
    // Ожидание освобождения шины
    i2c_last_error = i2c_wait_not_busy();
    if (I2C_ERROR_SUCCESS != i2c_last_error) { return i2c_last_error; }
    // ------------------------------------------------------------------------
    // Начинаем передачу по шине I2C
    I2C_GenerateSTART( I2C1, ENABLE );
    // ------------------------------------------------------------------------
    // Ожидание переключения в режим ведущего шины
    i2c_last_error = i2c_wait_master_mode();
    if (I2C_ERROR_SUCCESS != i2c_last_error) { return i2c_last_error; }
    // ------------------------------------------------------------------------
    // Отправка адреса переферийного устройства (запись)
    //i2c_last_error = i2c_send_byte(per_hrdw_address);
	I2C_Send7bitAddress(I2C1, per_hrdw_address, direction);
    // ------------------------------------------------------------------------
    i2c_last_error = i2c_wait_transmit_complete();
    // ------------------------------------------------------------------------
    return i2c_last_error;
    // ------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Предполагая, что передатчик I2C готов к передаче, отправьте байт и подождите
// пока передатчик не опустеет, чтобы затем отправить следующий байт.
uint32_t i2c_send_byte(uint8_t data)
{
    // Отправка байта
    I2C1->DATAR = data;

    // Ожидание окончания (асинхронная передача)
    i2c_last_error = i2c_wait_transmit_complete();
    
    return i2c_last_error;
}
// ----------------------------------------------------------------------------
// Упрощенное чтение с устройства
uint8_t i2c_read_byte_simple(void) {
    
    uint32_t timeout = I2C_MASTER_RECEIVER_LOOPS;
    
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET) {
        
        if (timeout-- == 0) return 0; // Защита от зависания
    }

    return (uint8_t)I2C1->DATAR;
}
// ----------------------------------------------------------------------------
// Предполагая, что I2C-приемник готов к приему, начнем прием байта и будем ждать
// пока приемник заполнится, затем передадим байт в буфер.
uint8_t i2c_read_byte(void)
{
    i2c_last_error = i2c_wait_master_receiver_mode();

    if(I2C_ERROR_SUCCESS != i2c_last_error) {

        //uint32_t status = I2C_GetLastEvent(I2C1);
        return 0;
    }

    // Внимание! Экспериментальная версия. При проблемах - удалить строку ниже.
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET) { }
	
    // Передаем принятый байт в буфер
    uint8_t data_rcv = I2C1->DATAR;

    i2c_last_error = I2C_ERROR_SUCCESS;
    return data_rcv;
}
// ----------------------------------------------------------------------------
// Отправка массива данных в переферию I2C
// Возврат: I2C_ERROR состояния
uint32_t i2c_write(uint8_t i2c_address, uint8_t * data, uint8_t count)
{
    i2c_total_bytes = 0;

    i2c_last_error = i2c_begin_transaction(i2c_address, I2C_Direction_Transmitter);
    if (I2C_ERROR_SUCCESS != i2c_last_error) { return i2c_last_error; }

    // Отправка массива данных побайтово
    while (count) {

        i2c_last_error = i2c_send_byte (*data);
        if (I2C_ERROR_SUCCESS != i2c_last_error) { return i2c_last_error; }

        i2c_total_bytes++;
        data++;
        count--;
    }

    // Заканчиваем цикл передачи данных
    I2C_GenerateSTOP( I2C1, ENABLE );

    return i2c_last_error;
}
// ----------------------------------------------------------------------------
// Прием массива данных из устройства
// Поддерживается ACK/NAK бит для передачи
uint32_t i2c_read(uint16_t i2c_address, uint8_t * data, uint8_t count)
{
    // -------------------------------------------------------------------------
    i2c_total_bytes = 0;
    // -------------------------------------------------------------------------
    i2c_last_error = i2c_begin_transaction(i2c_address, I2C_Direction_Receiver);
    if (I2C_ERROR_SUCCESS != i2c_last_error) { return i2c_last_error; }
    // -------------------------------------------------------------------------
    // Последовательное чтение данных
    for (uint16_t read_index = 0; read_index < count; read_index++) {

        // Если прочитывается последний байт, отправляем NAK
        if (read_index + 1 == count) { i2c_setNACK(); } else { i2c_setACK(); }

        //*(data + read_index) = i2c_read_byte();
        // data[read_index] = i2c_read_byte();
        uint8_t read_data = i2c_read_byte();
        data[read_index] = read_data;

        if (I2C_ERROR_SUCCESS != i2c_last_error) {
            return i2c_last_error;
        }

        i2c_total_bytes++;
    }
    // -------------------------------------------------------------------------
    // Заканчиваем цикл передачи данных
    I2C_GenerateSTOP(I2C1, ENABLE);
    // -------------------------------------------------------------------------
    i2c_setACK();
    // -------------------------------------------------------------------------
    return I2C_ERROR_SUCCESS;
    // -------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Универсальный метод ожидания событий
// Возврат: коды ошибок I2C
uint32_t I2C_WaitFlag (I2C_TypeDef *I2Cx, uint32_t I2C_FLAG) {

    uint32_t timeout = I2C_BUSY_LOOPS;

    while (!I2C_GetFlagStatus (I2Cx, I2C_FLAG)) {

        // Если при отправке адреса или данных устройство не ответило
        if (I2C_GetFlagStatus (I2Cx, I2C_FLAG_AF)) {

            I2C_ClearFlag (I2Cx, I2C_FLAG_AF);
            I2C_GenerateSTOP (I2Cx, ENABLE);
            return I2C_ERROR_ACK;  // Ошибка подтверждения (NACK)
        }
        if (timeout-- == 0) {

            return I2C_ERROR_TIME_OUT;  // Ошибка по времени
        }
    }

    return I2C_ERROR_SUCCESS;  // Флаг успешно дождались
}
// ----------------------------------------------------------------------------
// Получив 7-битный I2C-адрес, инициируем чтение с устройства, ожидая подтверждения (ACK) в ответ на адрес.
// Если устройство присутствует и предоставляет ACK, возвращаем I2C_ERROR_SUCCESS, иначе - I2C_ERROR_ACK.
// Метод работает и проверен в железе!
uint32_t i2c_device_detect(uint16_t i2c_address)
{
    I2C_GenerateSTART (I2C1, ENABLE);
    while (!I2C_CheckEvent (I2C1, I2C_EVENT_MASTER_MODE_SELECT));  // Ждем START

    I2C_Send7bitAddress (I2C1, i2c_address, I2C_Direction_Transmitter);

    uint32_t t = 10000;

    // А ТЕПЕРЬ КРИТИЧЕСКИЙ ЦИКЛ:
    while (!I2C_CheckEvent (I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && t--) {

        if (t == 0) { return I2C_ERROR_TIME_OUT; } 

        if (I2C_GetFlagStatus (I2C1, I2C_FLAG_AF)) {  // Если получили NACK (устройства нет)
            I2C_GenerateSTOP (I2C1, ENABLE);          // Сбрасываем шину
            I2C_ClearFlag (I2C1, I2C_FLAG_AF);        // Обязательно чистим флаг ошибки!
            return I2C_ERROR_ACK;                     // Выходим с ошибкой
        }
    }

    I2C_ClearFlag (I2C1, I2C_FLAG_AF);

    // Если дошли сюда — устройство найдено (ACK получен)
    I2C_GenerateSTOP (I2C1, ENABLE);
    return I2C_ERROR_SUCCESS;
}
// ----------------------------------------------------------------------------
// Создание консольного отображения, показывающего наличие устройств I2C с помощью карты адресов,
// аналогично следующему результату, полученному командой i2cdetect в Linux:
//      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
// 00:          -- -- -- -- -- -- -- -- -- -- -- -- --
// 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 50: -- -- -- -- -- -- 56 -- -- -- -- -- -- -- -- --
// 60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
// 70: -- -- -- -- -- -- -- —-
#ifdef USE_DEBUG_PRINT
void i2c_scan(void)
{
    printf("\r\n     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");

    printf("00:          ");

    for(uint8_t address=0x03; address<=0xEE; address++) {
    
        // Вывод заголовка таблицы (первая строка)
        if((address % 0x10) == 0) printf("\r\n%02X: ",address);

        if(I2C_ERROR_SUCCESS == i2c_device_detect(address)) {
            
            printf("%02X ", address);
        } else {
            
            printf("-- ");
        }
    }

    printf("\n");
}
#endif
// ----------------------------------------------------------------------------
