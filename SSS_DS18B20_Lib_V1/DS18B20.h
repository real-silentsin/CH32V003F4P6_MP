/********************************** (C) COPYRIGHT *******************************
 * File Name          : DS18B20.h
 * Author             : vantr
 * Version            : V2.0.0
 * Date               : 2026/05/05
 * Description        : Драйвер цифрового датчика температуры DS18B20
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе.
 *******************************************************************************/
// ---------------------------------------------------------------------------------
#ifndef __SSS_DS1820003__
// ---------------------------------------------------------------------------------
#define __SSS_DS1820003__
// ---------------------------------------------------------------------------------
#include "OneWareProtocol.h"
#include "SSS_Common_Lib_V1/sss_classes.h"
// ---------------------------------------------------------------------------------
// Использовать вывод в консоль
//#define USE_DEBUG_PRINT
// ---------------------------------------------------------------------------------
/*
#define SENSOR_1 0x3415167312646128ULL
#define SENSOR_2 0x6745230198ABCE28ULL // Пример второго ID

while(1) {
    DS18B20_StartAll();    // Сказали ВСЕМ датчикам мерить температуру
    Delay_Ms(750);         // Ждем один раз для всех

    // Читаем первый
    Params p1 = DS18B20_ReadByID(SENSOR_1);
    if(p1.sign != 2) printf("T1: %d.%02d\r\n", p1.value1, p1.value2);

    // Читаем второй (сразу, без новых пауз по 750мс!)
    Params p2 = DS18B20_ReadByID(SENSOR_2);
    if(p2.sign != 2) printf("T2: %d.%02d\r\n", p2.value1, p2.value2);

    Delay_Ms(1000); // Пауза перед следующим циклом опроса
}
*/
// ---------------------------------------------------------------------------------
// Разрешение (работает не со всеми клонами!)
typedef enum {
    DS_RES_9BIT = 0x1F,
    DS_RES_10BIT = 0x3F,
    DS_RES_11BIT = 0x5F,
    DS_RES_12BIT = 0x7F
} DS18B20_res_t;
// ---------------------------------------------------------------------------------
typedef struct {
    int16_t temp_c;       // Целые градусы (со знаком: -25, 0, 100)
    uint16_t temp_fract;  // Дробная часть (0, 625, 1250... 9375)
    uint8_t is_negative;  // Флаг отрицательной температуры

    uint8_t th_trip;
    uint8_t tl_trip;

    DS18B20_res_t resolution;

    uint8_t is_valid;  // 1 если данные верны, 0 если CRC ошибка
} DS18B20_Data;
// ---------------------------------------------------------------------------------
static uint8_t LastDiscrepancy = 0;  // Место последней развилки
static uint8_t LastDeviceFlag = 0;   // Флаг окончания поиска
// ---------------------------------------------------------------------------------
// Инициализация шины
void DS18B20_Init() {

    OneWare_Init();
}
// ---------------------------------------------------------------------------------
// Расчет CRC8 по методике Dallas
uint8_t dallas_crc8 (const uint8_t *data, uint8_t len) {

    uint8_t crc = 0;

    while (len--) {

        uint8_t inbyte = *data++;
        
        for (uint8_t i = 8; i; i--) {

            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            
            if (mix)
                crc ^= 0x8C;  // Реверсивный полином 0x31
            
            inbyte >>= 1;
        }
    }
    return crc;
}
// ---------------------------------------------------------------------------------
// Выбор конкретного датчика по его 64-битному ID
void DS18B20_MatchROM (uint64_t rom_id) {

    if (OneWare_Reset() != 0)
        return;

    if (rom_id == 0) {

        OneWare_WriteData (0xCC);  // SKIP ROM
        return;
    }

    // 1. Посылаем команду Match ROM
    OneWare_WriteData (0x55);

    // 2. Посылаем 64 бита (8 байт) адреса
    // Поскольку у нас Little-Endian, мы просто идем побайтово
    uint8_t *id_bytes = (uint8_t *)&rom_id;

    for (uint8_t i = 0; i < 8; i++) {
        OneWare_WriteData (id_bytes[i]);
    }
}
// ---------------------------------------------------------------------------------
// Функция поиска ОДНОГО следующего устройства
// Возвращает 1, если устройство найдено, и 0, если устройств больше нет
uint8_t DS18B20_SearchNext (uint64_t *found_id) {

    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t rom_byte_mask = 1;
    uint8_t search_direction;

    uint8_t rom_code[8] = {0};  // Временный буфер для ID

    if (LastDeviceFlag) {
        LastDeviceFlag = 0;
        LastDiscrepancy = 0;
        return 0;
    }

    if (OneWare_Reset() != 0)
        return 0;

    OneWare_WriteData (0xF0);  // Команда SEARCH ROM

    while (id_bit_number <= 64) {
        // Читаем два бита (прямой и инверсный)
        uint8_t bit = OneWare_ReadBit();
        uint8_t bit_inv = OneWare_ReadBit();

        if (bit == 1 && bit_inv == 1)
            return 0;  // Ошибка: на шине никого нет

        if (bit != bit_inv) {
            // У всех датчиков в этом месте одинаковый бит
            search_direction = bit;
        } else {
            // КОЛЛИЗИЯ! У датчиков разные биты
            if (id_bit_number < LastDiscrepancy) {
                // Идем по старому пути
                search_direction = ((*(uint64_t *)rom_code) >> (id_bit_number - 1)) & 0x01;
            } else if (id_bit_number == LastDiscrepancy) {
                // В прошлый раз шли в "0", теперь идем в "1"
                search_direction = 1;
            } else {
                // Новая коллизия, всегда сначала идем в "0"
                search_direction = 0;
            }

            if (search_direction == 0)
                last_zero = id_bit_number;
        }

        // Записываем выбранный бит в массив
        if (search_direction)
            rom_code[rom_byte_number] |= rom_byte_mask;
        else
            rom_code[rom_byte_number] &= ~rom_byte_mask;

        // Отправляем выбранное направление датчикам
        OneWare_WriteBit (search_direction);

        id_bit_number++;
        rom_byte_mask <<= 1;
        if (rom_byte_mask == 0) {
            rom_byte_number++;
            rom_byte_mask = 1;
        }
    }

    LastDiscrepancy = last_zero;
    if (LastDiscrepancy == 0)
        LastDeviceFlag = 1;

    *found_id = *(uint64_t *)rom_code;
    return 1;
}
// ---------------------------------------------------------------------------------
// Поиск всех сенсоров, подключенных к этой шине
uint8_t Discover_All_Sensors (uint64_t *sensors) {
    
    uint64_t next_id;
    uint8_t count = 0;

#ifdef USE_DEBUG_PRINT
    printf ("Searching for sensors...\r\n");
#endif

    while (DS18B20_SearchNext (&next_id)) {
        // Проверяем CRC найденного ID, чтобы отсечь мусор
        if (dallas_crc8 ((uint8_t *)&next_id, 7) == ((uint8_t *)&next_id)[7]) {

            sensors[count] = next_id;
            count += 1;
#ifdef USE_DEBUG_PRINT
            // Разбиваем 64-битное число на две 32-битные части для printf
            uint32_t high = (uint32_t)(next_id >> 32);
            uint32_t low = (uint32_t)(next_id & 0xFFFFFFFF);

            printf ("Found Sensor %d: %08X%08X\r\n", count, high, low);

        } else {

            printf ("Found Sensor %d: CRC Error!\r\n", count);
#endif
        }
    }

#ifdef USE_DEBUG_PRINT
    if (count == 0)
        printf ("No sensors found.\r\n");
#endif

    return (count == 0) ? 0 : count;
}
// ---------------------------------------------------------------------------------
// Чтение настроек выбранного сенсора
uint8_t DS18B20_ReadPresets (uint64_t sensor_id, uint8_t *preset_data) {

    uint8_t scratchpad[9];

    if (OneWare_Reset() != 0) {
        return 0;  // Ошибка: датчик не ответил
    }

    DS18B20_MatchROM (sensor_id);
    OneWare_WriteData (0xBE);  // READ SCRATCHPAD

    // Читаем все 9 байт, включая CRC
    for (int i = 0; i < 9; i++) {
        scratchpad[i] = OneWare_ReadData();
    }

    // Проверяем контрольную сумму (CRC8 от первых 8 байт должна совпасть с 9-м байтом)
    if (dallas_crc8(scratchpad, 8) != scratchpad[8]) {

        return 0;  // Ошибка: данные повреждены
    }

    // Если всё ок, копируем нужные байты в preset_data (TH, TL и Config)
    // Байты 2, 3, 4 в Scratchpad — это индексы [2], [3], [4]
    for (int i = 0; i < 5; i++) {
        preset_data[i] = scratchpad[i];
    }

    return 1;  // Успех, данные проверены
}
// ---------------------------------------------------------------------------------
// Чтение ID одиночного датчика
uint64_t DS18B20_ReadID (void) {

    uint8_t rom_code[8];

    if (OneWare_Reset() != 0) {
#ifdef USE_DEBUG_PRINT
        printf ("Sensor not found!\r\n");
#endif
        return 0;
    }

    // Команда чтения ROM
    OneWare_WriteData (0x33);

    // Читаем 8 байт
    for (int i = 0; i < 8; i++) {
        rom_code[i] = OneWare_ReadData();
    }

#ifdef USE_DEBUG_PRINT
    // Выводим результат
    printf ("ROM ID: ");
    for (int i = 0; i < 8; i++) {
        printf ("%02X ", rom_code[i]);
    }

    printf ("\r\n");
#endif

    uint8_t crc = dallas_crc8(rom_code, 7);
    if (crc == rom_code[7]) {
#ifdef USE_DEBUG_PRINT
        printf ("CRC: OK\r\n");
#endif
    } else {
#ifdef USE_DEBUG_PRINT
        printf ("CRC: FAILED\r\n");
#endif
        return 0;
    }
    
    // Проверка семейного кода (Family Code)    
    if (rom_code[0] == 0x28) {
#ifdef USE_DEBUG_PRINT
        printf ("Family: DS18B20\r\n");
#endif
    } else {
#ifdef USE_DEBUG_PRINT
        printf ("Unknown Family: %02X\r\n", rom_code[0]);
#endif
        return 0;
    }

    // Приведение типа через указатель:
    uint64_t sensor_id = *(uint64_t *)rom_code;

#ifdef USE_DEBUG_PRINT
    // Печатаем как два 32-битных числа в HEX
    uint32_t high = (uint32_t)(sensor_id >> 32);
    uint32_t low = (uint32_t)(sensor_id & 0xFFFFFFFF);

    printf ("ID: %08X%08X\r\n", high, low);
#endif

    return sensor_id;
}
// ---------------------------------------------------------------------------------
/* // Установка точности измерения датчика
void DS18B20_setResolution (uint64_t sensor_id, DS18B20_res_t res) {

    // 1. Сброс и пропуск ROM
    if (OneWare_Reset() != 0)
        return;

    DS18B20_MatchROM (sensor_id);

    // 2. Команда ЗАПИСИ
    OneWare_WriteData (0x4E);

    // 3. ТРИ БАЙТА ПОДРЯД БЕЗ ПАУЗ (без printf и чтений между ними)
    OneWare_WriteData (0xFF);  // TH (любое значение)
    OneWare_WriteData (0xFF);  // TL (любое значение)
    OneWare_WriteData (res);   // Наше разрешение (0x1F, 0x3F, 0x5F или 0x7F)

    // 4. Завершающий сброс, чтобы подтвердить запись в RAM
    OneWare_Reset();

    // 1. Reset и выбор устройства
    if (OneWare_Reset() != 0) {

        return;                // Датчик не найден
    }

    OneWare_WriteData(0xCC);  // SKIP ROM

    // 2. Читаем текущие TH и TL, чтобы не затереть их мусором
    // (Полезно, если вы используете функции термостата)
    OneWare_WriteData(0xBE);  // READ SCRATCHPAD

    uint8_t data[5];
    for (int i = 0; i < 5; i++) {

        data[i] = OneWare_ReadData();
    }

    // Нас интересуют: data[2] - TH, data[3] - TL

    // 3. Записываем изменения
    OneWare_Reset();
    OneWare_WriteData(0xCC);     // SKIP ROM
    OneWare_WriteData(0x4E);     // WRITE SCRATCHPAD

    OneWare_WriteData(data[2]);  // Возвращаем TH
    OneWare_WriteData(data[3]);  // Возвращаем TL
    OneWare_WriteData(res);      // Устанавливаем новое разрешение

    // 4. (Опционально) Сохраняем в EEPROM
    // Если этого не сделать, после сброса питания вернется 12 бит
    OneWare_Reset();
    OneWare_WriteData(0xCC);
    OneWare_WriteData(0x48);  // COPY SCRATCHPAD

    // Важно: на время записи в EEPROM (10мс)
    // шина должна быть подтянута к VCC
    Delay_Ms(10);
} */
// ---------------------------------------------------------------------------------
// Запуск преобразования для ВСЕХ подключенных датчиков
void DS18B20_StartAll (void) {

    if (OneWare_Reset() == 0) {
        OneWare_WriteData (0xCC);  // SKIP ROM (обращаемся ко всем сразу)
        OneWare_WriteData (0x44);  // CONVERT T (всем начать замер)
    }
}
// ---------------------------------------------------------------------------------
// Чтение полного состояния указанного сенсора (данные должны быть к этому времени готовы)
// Если сенсор один - передайте параметр sensor_id = 0
DS18B20_Data DS18B20_ReadSensor(uint64_t sensor_id) {

    DS18B20_Data res = {0};
    uint8_t scratchpad[9] = {0};

    if (OneWare_Reset() != 0)
        return res;

    // Выбор датчика (если 0, можно использовать Skip ROM 0xCC для одиночного)
    DS18B20_MatchROM (sensor_id);

    OneWare_WriteData (0xBE);  // READ SCRATCHPAD

    for (uint8_t i = 0; i < 9; i++) {
        scratchpad[i] = OneWare_ReadData();
    }

    // Проверка контрольной суммы (CRC8)
    if (dallas_crc8 (scratchpad, 8) == scratchpad[8]) {
        // 1. Парсим температуру
        int16_t raw = (int16_t)((scratchpad[1] << 8) | scratchpad[0]);
        if (raw < 0) {
            res.is_negative = 1;
            raw = -raw;
        }
        res.temp_c = raw >> 4;
        res.temp_fract = (uint16_t)((raw & 0x0F) * 625);

        // 2. Достаем настройки
        res.th_trip = scratchpad[2];
        res.tl_trip = scratchpad[3];

        // 3. Сохраняем разрешение как тип DS18B20_res_t
        res.resolution = (DS18B20_res_t)scratchpad[4];

        res.is_valid = 1;
    }

    return res;
}
// ---------------------------------------------------------------------------------
// Запись настроек в указанный сенсор
void DS18B20_WriteSettings (uint64_t sensor_id, DS18B20_Data settings) {

    if (OneWare_Reset() != 0)
        return;

    DS18B20_MatchROM (sensor_id);
    OneWare_WriteData (0x4E);  // Команда WRITE SCRATCHPAD

    // 1. Записываем TH
    OneWare_WriteData (settings.th_trip);
    // 2. Записываем TL
    OneWare_WriteData (settings.tl_trip);

    // 3. Записываем конфигурацию (разрешение)
    // Маска для разрешения: 9bit=0x1F, 10bit=0x3F, 11bit=0x5F, 12bit=0x7F
    OneWare_WriteData ((uint8_t)settings.resolution);

    // ВАЖНО: Настройки в оперативной памяти.
    // Чтобы они выжили после сброса питания, их нужно сохранить в EEPROM:
    if (OneWare_Reset() == 0) {

        DS18B20_MatchROM (sensor_id);
        OneWare_WriteData (0x48);  // Команда COPY SCRATCHPAD

        // Здесь нужно подождать 10мс, пока датчик пишет в EEPROM
        Delay_Ms(10);
    }
}
// ---------------------------------------------------------------------------------
// Установка разрешения указанного сенсора
void DS18B20_SetResolution(uint64_t sensor_id, DS18B20_res_t res) {

    if (OneWare_Reset() != 0)
        return;

    DS18B20_MatchROM (sensor_id);
    OneWare_WriteData (0x4E);  // WRITE SCRATCHPAD

    OneWare_WriteData (0x4B);  // TH (просто заглушка 75°C)
    OneWare_WriteData (0x46);  // TL (просто заглушка 70°C)
    OneWare_WriteData (res);   // Наше разрешение из enum

    // Сохраняем в EEPROM, чтобы проверить "живучесть"
    if (OneWare_Reset() == 0) {

        DS18B20_MatchROM (sensor_id);
        OneWare_WriteData (0x48);  // COPY SCRATCHPAD
        Delay_Ms(15);
    }
}
// ---------------------------------------------------------------------------------
#ifdef USE_DEBUG_PRINT
void Test_Sensor_Capabilities (uint64_t id) {
    printf ("Testing sensor: %llX\n", id);

    // Пробуем поставить 9 бит
    DS18B20_SetResolutionTest(id, DS_RES_9BIT);

    // Читаем всё обратно единым методом
    DS18B20_Data data = DS18B20_ReadSensor(id);

    if (data.is_valid) {
        if (data.resolution == 9) {
            printf ("Success: Sensor supports 9-bit mode!\n");
        } else {
            printf ("Fake: Sensor is locked to %d-bit mode.\n", data.resolution);
        }
    } else {
        printf ("Error: CRC failed during test.\n");
    }
}
#endif
// ---------------------------------------------------------------------------------
#endif
// ---------------------------------------------------------------------------------
