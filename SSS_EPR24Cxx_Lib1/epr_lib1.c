/********************************** (C) COPYRIGHT *******************************
 * File Name          : epr_lib1.h
 * Author             : vantr
 * Description        : Драйвер для EEPROM 24Cxx (аппаратный I2C) CH32V003 (SOP16)
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * ------------------------------------------------------------------------------
 * ВНИМАНИЕ! Применение в программе отладочных printf может привести
 * к неработоспособности данного драйвера!
 * Не забывайте правильно настраивать источник тактирования MCU!
 *******************************************************************************/
// ----------------------------------------------------------------------------
#include "epr_lib1.h"
#include "SSS_I2CHW_MLib_V4.4/i2c.h"
// ----------------------------------------------------------------------------
volatile I2C_ERROR epr_last_error = I2C_ERROR_SUCCESS;
// ----------------------------------------------------------------------------
// Инициализация механизма
void EPR_Init(){

    epr_last_error = I2C_ERROR_SUCCESS;

/*#ifdef EPR_INIT_I2C
    IIC_Init(I2C_SPEED_STANDARD_MODE, I2C_SELF_ADDRESS);
#endif */
}
// ----------------------------------------------------------------------------
// Получить код ошибки последней операции
uint32_t EPR_getLastError() {

    return epr_last_error;
}
// ----------------------------------------------------------------------------
// Получение полного набора адреса под выбранный чип EEPROM
Params EPR_getAddress (uint8_t epr_device_phy_addr, uint16_t address) {
    // -------------------------------------------------------------------------
    Params _addr_result;
    // -------------------------------------------------------------------------
    Int16x _addr_tmp;
    _addr_tmp.ivalue = address;
    // -------------------------------------------------------------------------
#ifdef DEV_2402
    // Адресация в массиве памяти 24C02
    // Bit | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00
    // [1] | 01 | 00 | 01 | 00 | P2 | P1 | P0 | RW
    // [2] | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0
    // [S] | XX | XX | XX | XX | XX | XX | XX | XX
    _addr_result.value1 = (uint8_t)((((epr_device_phy_addr & 0x07) << 1) | EPR_CHIP_I2C_ADDR));
    _addr_result.value2 = _addr_tmp.cvalue[0];
    _addr_result.sign = 0;

    _addr_result.extend = 1;
#endif
    // -------------------------------------------------------------------------
#ifdef DEV_2404
    // Адресация в массиве памяти 24C04
    // Bit | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00
    // [1] | 01 | 00 | 01 | 00 | P2 | P1 | A8 | RW
    // [2] | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0
    // [S] | XX | XX | XX | XX | XX | XX | XX | XX
    _addr_result.value1 = (uint8_t)((((epr_device_phy_addr & 0x06) << 1) | ((_addr_tmp.cvalue[1] & 0x01) << 1) | EPR_CHIP_I2C_ADDR));
    _addr_result.value2 = _addr_tmp.cvalue[0];
    _addr_result.sign = 0;

    _addr_result.extend = 1;
#endif
    // -------------------------------------------------------------------------
#ifdef DEV_2408
    // Адресация в массиве памяти 24C08 (нет внешних P0-P2)
    // Bit | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00
    // [1] | 01 | 00 | 01 | 00 | XX | A9 | A8 | RW
    // [2] | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0
    // [S] | XX | XX | XX | XX | XX | XX | XX | XX
    _addr_result.value1 = (uint8_t)((((_addr_tmp.cvalue[1] & 0x03) << 1) | EPR_CHIP_I2C_ADDR));
    _addr_result.value2 = _addr_tmp.cvalue[0];
    _addr_result.sign = 0;

    _addr_result.extend = 1;
#endif
    // -------------------------------------------------------------------------
#ifdef DEV_2416
    // Адресация в массиве памяти 24C16 (нет внешних P0-P2)
    // Bit | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00
    // [1] | 01 | 00 | 01 | 00 | A10| A9 | A8 | RW
    // [2] | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0
    // [S] | XX | XX | XX | XX | XX | XX | XX | XX
    _addr_result.value1 = (uint8_t)(((_addr_tmp.cvalue[1] & 0x07) << 1) | EPR_CHIP_I2C_ADDR);
    _addr_result.value2 = _addr_tmp.cvalue[0];
    _addr_result.sign = 0;

    _addr_result.extend = 1;
#endif
    // -------------------------------------------------------------------------
#ifdef DEV_2432
    // Адресация в массиве памяти 24C32
    // Bit | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00
    // [1] | 01 | 00 | 01 | 00 | P2 | P1 | P0 | RW
    // [2] | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0
    // [S] | XX | XX | XX | XX | A11| A10| A9 | A8
    _addr_result.value1 = (uint8_t)(((epr_device_phy_addr & 0x07) << 1) | EPR_CHIP_I2C_ADDR);
    _addr_result.value2 = _addr_tmp.cvalue[0];
    _addr_result.sign = (_addr_tmp.cvalue[1] & 0x0F);

    _addr_result.extend = 2;
#endif
    // -------------------------------------------------------------------------
#ifdef DEV_2464
    // Адресация в массиве памяти 24C64
    // Bit | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00
    // [1] | 01 | 00 | 01 | 00 | P2 | P1 | P0 | RW
    // [2] | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0
    // [S] | XX | XX | XX | A12| A11| A10| A9 | A8
    _addr_result.value1 = (uint8_t)(((epr_device_phy_addr & 0x07) << 1) | EPR_CHIP_I2C_ADDR);
    _addr_result.value2 = _addr_tmp.cvalue[0];
    _addr_result.sign = (_addr_tmp.cvalue[1] & 0x1F);

    _addr_result.extend = 2;
#endif
    // -------------------------------------------------------------------------
    /* if (mode == 0) {

        cbi(_addr_result.value1, 0);
    } else {

        sbi(_addr_result.value1, 0);
    } */
    // -------------------------------------------------------------------------
    return _addr_result;
    // -------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Не зависимый от типа чипа метод. Вызывайте его!
uint16_t EPR_GetPageSize() {

/*  Старшие модели: Если решите добавить 24C128 или 24C256, 
    там размер страницы увеличивается до 64 байт. А у 24C512 — до 128 байт.*/

#ifdef DEV_2402
    return 8;
#endif

#if defined(DEV_2404) || defined(DEV_2408) || defined(DEV_2416)
    return 16;
#endif

#if defined(DEV_2432) || defined(DEV_2464)
    return 32;
#endif
}
// ----------------------------------------------------------------------------
// Инициализация обмена с чипом EEPROM
Params EPR_DummyWrite (uint8_t epr_device_phy_addr, uint16_t address) {

    Params slave_address = EPR_getAddress (epr_device_phy_addr, address);
    epr_last_error = I2C_ERROR_SUCCESS;

    // 1. Ожидание освобождения шины с таймаутом
    uint32_t timeout = 100000;
    while (I2C_GetFlagStatus (I2C1, I2C_FLAG_BUSY)) {
        
        if (timeout-- == 0) {

            epr_last_error = I2C_ERROR_BUSY;
            return slave_address;
        }
    }

    // 2. Старт
    I2C_GenerateSTART (I2C1, ENABLE);
    if (I2C_WaitFlag (I2C1, I2C_FLAG_SB)) {

        epr_last_error = I2C_ERROR_TIME_OUT;
        return slave_address;
    }

    // 3. Адрес устройства (запись)
    I2C_Send7bitAddress (I2C1, (slave_address.value1 & 0xFE), I2C_Direction_Transmitter);

    if (I2C_WaitFlag (I2C1, I2C_FLAG_ADDR)) {
        
        epr_last_error = I2C_ERROR_ADDR;
        return slave_address;
    }

    (void)I2C1->STAR2;  // Очистка флага ADDR после чтения SR1 и SR2

    // 4. Передача адреса ячейки (MSB если 16-битный адрес)
    if (slave_address.extend == 2) {

        I2C_SendData (I2C1, slave_address.sign);

        if (I2C_WaitFlag (I2C1, I2C_FLAG_BTF)) {

            epr_last_error = I2C_ERROR_BUSY;
            return slave_address;
        }
    }

    // 5. Передача адреса ячейки LSB
    I2C_SendData (I2C1, slave_address.value2);

    if (I2C_WaitFlag (I2C1, I2C_FLAG_BTF)) {

        epr_last_error = I2C_ERROR_BUSY;
        return slave_address;
    }

    return slave_address;
}
// ----------------------------------------------------------------------------
// Чтение одиночного байта из EEPROM
uint8_t EPR_readByte (uint8_t epr_device_phy_addr, uint16_t ReadAddr) {

    epr_last_error = I2C_ERROR_SUCCESS;

    // 1. Установка адреса (Dummy Write) с проверкой
    Params sl_addr = EPR_DummyWrite (epr_device_phy_addr, ReadAddr);

    if (epr_last_error != I2C_ERROR_SUCCESS) { return 0; }

    // 2. Повторный старт для перехода в режим чтения
    I2C_GenerateSTART (I2C1, ENABLE);

    if (I2C_WaitFlag (I2C1, I2C_FLAG_SB)) {

        epr_last_error = I2C_ERROR_BUSY;
        
        return 0;
    }

    // 3. Отправка адреса устройства + бит чтения (0x01)
    I2C_Send7bitAddress (I2C1, (sl_addr.value1 | 0x01), I2C_Direction_Receiver);

    // Ждем флаг ADDR (подтверждение адреса девайсом)
    if (I2C_WaitFlag (I2C1, I2C_FLAG_ADDR)) {

        epr_last_error = I2C_ERROR_ADDR;
        I2C_GenerateSTOP (I2C1, ENABLE);

        return 0;
    }

    // --- КРИТИЧЕСКАЯ ПОСЛЕДОВАТЕЛЬНОСТЬ ДЛЯ ЧТЕНИЯ 1 БАЙТА ---
    // Согласно Reference Manual STM32, для чтения 1 байта:
    // ACK = 0 -> Clear ADDR -> Generate STOP -> Read Data

    I2C_AcknowledgeConfig (I2C1, DISABLE);  // Выключаем подтверждение (NACK)

    //__disable_irq();                        // Защита от прерываний для точности тайминга
    (void)I2C1->STAR2;                      // Очистка флага ADDR
    I2C_GenerateSTOP (I2C1, ENABLE);        // Формируем СТОП сразу после очистки ADDR
    //__enable_irq();                         // Включаем прерывания обратно

    // ---------------------------------------------------------

    // 4. Ожидание данных в приемном регистре
    if (I2C_WaitFlag (I2C1, I2C_FLAG_RXNE)) {

        epr_last_error = I2C_ERROR_TIME_OUT;

        return 0;
    }

    // 5. Читаем полученный байт
    uint8_t data = I2C_ReceiveData (I2C1);

    // 6. Восстанавливаем состояние ACK для корректной работы других функций
    I2C_AcknowledgeConfig (I2C1, ENABLE);

    return data;
}
// ----------------------------------------------------------------------------
// Чтение массива данных из EEPROM
uint16_t EPR_readPage (uint8_t epr_device_phy_addr, uint16_t ReadAddr, uint8_t *buffer, uint16_t length) {

    epr_last_error = I2C_ERROR_SUCCESS;

    if (length == 0) { return 0; }

    Params sl_addr = EPR_DummyWrite (epr_device_phy_addr, ReadAddr);

    if (epr_last_error != I2C_ERROR_SUCCESS) { return 0; }

    // 2. Рестарт для чтения
    I2C_GenerateSTART (I2C1, ENABLE);

    if (I2C_WaitFlag (I2C1, I2C_FLAG_SB)) {

        epr_last_error = I2C_ERROR_BUSY;
        return 0;
    }

    // 3. Адрес + READ
    I2C_Send7bitAddress (I2C1, (sl_addr.value1 | 0x01), I2C_Direction_Receiver);

    if (I2C_WaitFlag (I2C1, I2C_FLAG_ADDR)) {
        
        epr_last_error = I2C_ERROR_ADDR;
        I2C_GenerateSTOP (I2C1, ENABLE);

        return 0;
    }

    // 4. Подготовка к чтению
    if (length == 1) {

        // Специфика для 1 байта (как в readByte)
        I2C_AcknowledgeConfig (I2C1, DISABLE);
        //__disable_irq();
        (void)I2C1->STAR2;
        I2C_GenerateSTOP (I2C1, ENABLE);
        //__enable_irq();
    } else {

        (void)I2C1->STAR2;  // Просто очистка ADDR для многих байт
    }

    uint16_t total_bytes = 0;

    // 5. Цикл приема данных
    for (uint16_t i = 0; i < length; i++) {

        // Если это предпоследний байт, готовим NACK и STOP для последнего
        if (i == length - 1 && length > 1) {

            I2C_AcknowledgeConfig (I2C1, DISABLE);
            I2C_GenerateSTOP (I2C1, ENABLE);
        }

        // Ждем байт
        if (I2C_WaitFlag (I2C1, I2C_FLAG_RXNE)) {

            epr_last_error = I2C_ERROR_TIME_OUT;
            return i;  // Возвращаем сколько успели прочитать до ошибки
        }

        buffer[i] = I2C_ReceiveData (I2C1);
        total_bytes += 1;
    }

    // 6. Финал
    I2C_AcknowledgeConfig (I2C1, ENABLE);  // Возвращаем ACK

    return total_bytes;
}
// ----------------------------------------------------------------------------
// Чтение одиночного байта из EEPROM
void EPR_writeByte (uint8_t epr_device_phy_addr, uint16_t WriteAddr, uint8_t Data) {

    epr_last_error = I2C_ERROR_SUCCESS;

    // 1. Установка адреса (Dummy Write)
    // Важно: EPR_DummyWrite тоже должна внутри использовать I2C_WaitFlag
    // и возвращать ошибку, если девайс не ответил.
    EPR_DummyWrite (epr_device_phy_addr, WriteAddr);

    if (epr_last_error != I2C_ERROR_SUCCESS) {

        return;  // Ошибка: девайс не отозвался на Dummy Write
    }

    // 2. Отправка данных
    I2C_SendData (I2C1, Data);

    // Используем нашу обертку для ожидания окончания передачи
    if (I2C_WaitFlag (I2C1, I2C_FLAG_BTF)) {  // BTF (Byte Transfer Finished) надежнее для STOP

        epr_last_error = I2C_ERROR_TIME_OUT;
        I2C_GenerateSTOP (I2C1, ENABLE);
        return;                             // Ошибка при передаче байта
    }

    // 3. Завершаем транзакцию
    I2C_GenerateSTOP (I2C1, ENABLE);

    // 4. Ждем физического завершения записи в ячейки
    // WP=1 на твоей плате защитит от записи, но задержка не помешает
    // для корректного освобождения логики шины.
    Delay_Ms (5);
    // ------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Запись массива байтов в EEPROM
void EPR_writePage (uint8_t epr_device_phy_addr, uint16_t WriteAddr, uint8_t *buffer, uint16_t length) {

    uint16_t bytes_remaining = length;
    uint16_t current_addr = WriteAddr;
    uint8_t *data_ptr = buffer;

    uint16_t page_size = EPR_GetPageSize();

    while (bytes_remaining > 0) {
        epr_last_error = I2C_ERROR_SUCCESS;

        // 1. Вычисляем, сколько байт можно впихнуть в текущую страницу (16 байт для 24C16)
        uint8_t page_offset = (uint8_t)(current_addr % page_size);
        uint8_t space_in_page = page_size - page_offset;
        uint8_t chunk_size = (bytes_remaining < space_in_page) ? (uint8_t)bytes_remaining : space_in_page;

        // 2. Установка адреса начала записи
        EPR_DummyWrite (epr_device_phy_addr, current_addr);
        if (epr_last_error != I2C_ERROR_SUCCESS)
            return;

        // 3. Последовательная отправка данных в буфер страницы чипа
        for (uint8_t i = 0; i < chunk_size; i++) {
            I2C_SendData (I2C1, data_ptr[i]);

            // Ждем завершения передачи каждого байта
            if (I2C_WaitFlag (I2C1, I2C_FLAG_BTF)) {
                epr_last_error = I2C_ERROR_TIME_OUT;
                I2C_GenerateSTOP (I2C1, ENABLE);
                return;
            }
        }

        // 4. Генерируем STOP — только сейчас чип начнет физически писать данные из буфера в память
        I2C_GenerateSTOP (I2C1, ENABLE);

        // 5. Ждем завершения внутреннего цикла записи (Twr = 5ms)
        // Без этого следующая итерация DummyWrite провалится (чип будет BUSY)
        Delay_Ms (5);

        // 6. Сдвигаем указатели
        bytes_remaining -= chunk_size;
        current_addr += chunk_size;
        data_ptr += chunk_size;
    }
}
// ----------------------------------------------------------------------------
/* // Потоковое чтение из eeprom
uint16_t EPR_readPage (uint8_t epr_device_phy_addr, uint16_t ReadAddr, uint8_t *buffer, uint16_t length) {
    // ------------------------------------------------------------------------
    epr_last_error = I2C_ERROR_SUCCESS;
    // ------------------------------------------------------------------------
    if (length == 0) { return 0; }
    // --- Dummy Write --------------------------------------------------------
    Params sl_addr = EPR_DummyWrite (epr_device_phy_addr, ReadAddr);
    // ------------------------------------------------------------------------
    // 1. Старт
    I2C_GenerateSTART (I2C1, ENABLE);
    if (I2C_WaitFlag (I2C1, I2C_FLAG_SB)) {

        epr_last_error = I2C_ERROR_BUSY;
        return 0;
    }
    // ----------------------------------------------------------------------------
    // 2. Адрес + Read
    I2C_Send7bitAddress (I2C1, (sl_addr.value1 | 0x01), I2C_Direction_Receiver);
    if (I2C_WaitFlag (I2C1, I2C_FLAG_ADDR)) {

        epr_last_error = I2C_ERROR_TIME_OUT;
        return 0;
    }
    // ----------------------------------------------------------------------------
    // 3. Подготовка к приему
    if (length == 1) {

        I2C_AcknowledgeConfig (I2C1, DISABLE);
        (void)I2C1->STAR2;  // Очистка ADDR
        I2C_GenerateSTOP (I2C1, ENABLE);

    } else {
        (void)I2C1->STAR2;
    }
    // ----------------------------------------------------------------------------
    uint16_t total_reader_bytes = 0;
    // ----------------------------------------------------------------------------
    // 4. Цикл чтения
    for (uint16_t i = 0; i < length; i++) {

        if (i == length - 1 && length > 1) {
            
            I2C_AcknowledgeConfig (I2C1, DISABLE);
            I2C_GenerateSTOP (I2C1, ENABLE);
        }

        if (I2C_WaitFlag (I2C1, I2C_FLAG_RXNE)) {

            epr_last_error = I2C_ERROR_TIME_OUT;
            return i;  // Вернем сколько успели прочитать
        }

        buffer[i] = I2C_ReceiveData (I2C1);
        total_reader_bytes += 1;
    }

    I2C_AcknowledgeConfig (I2C1, ENABLE);

    return total_reader_bytes;
    // ------------------------------------------------------------------------
} */
// ----------------------------------------------------------------------------
