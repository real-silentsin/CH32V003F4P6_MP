/********************************** (C) COPYRIGHT *******************************
 * File Name          : w25qxx.c
 * Author             : vantr
 * Description        : Драйвер флеш памяти Winbond W25Qxx (протокол X)
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// ----------------------------------------------------------------------------
#include "w25qxx.h"
#ifdef USE_SPI_HARDWARE
#include "SSS_SPIHW_Lib1/sss_spim3_lib.h"
#endif
#ifdef USE_SPI_SOFTWARE
#include "SSS_SPISW_Lib1/sss_spisw_lib1.h"
#endif
#include "SSS_CRC8_Lib_V1/crc8_Lib_V1.h"
#include "SSS_Common_Lib_V1/sss_math.h"

#include "SSS_W25Qxx_Lib_1/hardware.h"
#include "SSS_W25Qxx_Lib_1/sss_filetypes.h"

#include <string.h>
// ----------------------------------------------------------------------------
// Макросы управления CS - НЕ ИЗМЕНЯТЬ!
#define SPI_CS_LOW() (W25Q_SC_PORT.port->BCR = W25Q_SC_PORT.pin)    // CS = 0
#define SPI_CS_HIGH() (W25Q_SC_PORT.port->BSHR = W25Q_SC_PORT.pin)  // CS = 1
// ----------------------------------------------------------------------------
// Указатель на структуру данных подключенной флеш-памяти
const flash_hardware_t *flash_hw = NULL;
// Выделяем ПАМЯТЬ в RAM под одну активную структуру (скрыта через static)
__attribute__ ((aligned (4))) static flash_hardware_t active_flash_hw = {0};
// ----------------------------------------------------------------------------
static uint32_t last_free_page_idx = 0;  // Глобальный курсор для ускорения поиска
static sectorPage_t temp_page = {0};     // Временный буфер для чтения/записи
static systemRecord_t system_page = {0}; // Системный файл
static uint8_t last_error_code = LAST_ERROR_NO_ERROR;   // Код ошибки для некоторых операций
// ----------------------------------------------------------------------------
static uint16_t tmp_page_map[W25Q_CHUNK_SIZE]; // Буфер для работы с 16-битными значениями
// ----------------------------------------------------------------------------
// Инициализация механизма
void W25_Init (void) {

#ifdef USE_SPI_HARDWARE
    // Сначала инициализируем пин CS, он одинаков для любого транспорта
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    // Включаем тактирование
    RCC_APB2PeriphClockCmd (W25Q_SC_PORT.rcc, ENABLE);

    GPIO_InitStructure.GPIO_Pin = W25Q_SC_PORT.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (W25Q_SC_PORT.port, &GPIO_InitStructure);

    SPI_CS_HIGH();
#ifdef FLASH_INIT_SPI
    SPIM3_SPI_Init();
#endif
#endif

#ifdef USE_SPI_SOFTWARE
#endif
}

// ----------------------------------------------------------------------------
// Инициализация механики драйвера
uint8_t W25_InitHardware (uint32_t chipID, uint8_t ignore_sysfile) {

    for (int id_idx = 0; id_idx < HW_DEV_COUNT; id_idx++) {

        // printf ("check: %08x\r\n", (unsigned int)FLASH_HW_TABLE[id_idx].chip_id);

        if (chipID == FLASH_HW_TABLE[id_idx].chip_id) {

            flash_hw = &FLASH_HW_TABLE[id_idx];

            if (ignore_sysfile == 0) {
                // Чтение системного файла, если таковой есть
                systemRecord_t *sys_rec = W25_ReadSystemFile();
                // Если системный файл прочитан, но в нем начало непрерывной области не равно
                // считанному из стандартной конфигурации, заменяем это значение считанным из сис. файла
                if (sys_rec != NULL && sys_rec->data.ContAreaBegin != flash_hw->nonx_area_begin) {

                    // Замена адреса начала области
                    W25_HW_ChangeContAreaBegin (sys_rec->data.ContAreaBegin);
                }
            }
            // printf("OK\r\n");
            return 1;
        }
    }

    // Если дошли сюда — чип не опознан в таблице
    flash_hw = NULL;  // Явно сбрасываем в NULL для безопасности
    return 0;
}
// ----------------------------------------------------------------------------
// Замена начала области непрерывной памяти
void W25_HW_ChangeContAreaBegin(uint32_t new_value) {

    if (flash_hw == NULL)
        return;

    // Безопасное побайтовое копирование, которое никогда не вызовет Alignment Fault
    memcpy (&active_flash_hw, flash_hw, sizeof (flash_hardware_t));

    active_flash_hw.nonx_area_begin = new_value;
    active_flash_hw.totalx_pages = new_value;

    flash_hw = &active_flash_hw;
}
// ----------------------------------------------------------------------------
// Приватная функция обмена байтом по SPI
static uint8_t SPI_ExchangeByte (uint8_t data) {

#ifdef USE_SPI_HARDWARE
    return SPIM3_ExchangeByte(data);
#endif
#ifdef USE_SPI_SOFTWARE
    return SPISW_Transfer (data);
#endif
}
// ----------------------------------------------------------------------------
// Управление CS
static void W25_Select (void) { SPI_CS_LOW(); }

static void W25_Unselect (void) {

#ifdef USE_SPI_HARDWARE
    // Ждем окончания передачи последнего бита перед поднятием CS
    SPIM3_WaitForBusy();
#endif

#ifdef USE_SPI_SOFTWARE

#endif

    SPI_CS_HIGH();
}
// ----------------------------------------------------------------------------
// Проверка связи
// W25Q32 должна вернуть 0xEF4016
uint32_t W25_ReadID (void) {

    uint32_t id = 0;

    W25_Select();

    SPI_ExchangeByte (W25_CMD_JEDEC_ID);

    id |= (SPI_ExchangeByte (0xFF) << 16);
    id |= (SPI_ExchangeByte (0xFF) << 8);
    id |= SPI_ExchangeByte (0xFF);

    W25_Unselect();

    return id;
}
// ----------------------------------------------------------------------------
// Разрешение записи (нужно перед каждым стиранием или программированием)
void W25_WriteEnable (void) {

    W25_Select();

    SPI_ExchangeByte (W25_CMD_WRITE_ENABLE);

    W25_Unselect();
}
// ----------------------------------------------------------------------------
// Получение кода ошибки, метод сбрасывает значение кода
uint8_t W25_GetLastError() {

    uint8_t result = last_error_code;
    last_error_code = LAST_ERROR_NO_ERROR;

    return result;
}
// ----------------------------------------------------------------------------
// Получение объема чипа в страницах
uint32_t W25_GetChipSize() {

    return flash_hw->chip_size;
}
// ----------------------------------------------------------------------------
// Получение объема в страницах памяти, выделенной под протокол X
uint32_t W25_GetProtoVolumeSize() {

    return flash_hw->totalx_pages;
}
// ----------------------------------------------------------------------------
// Проверка валидности адреса в пределах всей флеш памяти
uint8_t W25_CheckPageAddr(uint32_t Address) {

    return (Address < flash_hw->chip_size) ? 1 : 0;
}
// ----------------------------------------------------------------------------
// Функция проверки адреса страницы
flash_area_t W25_CheckPageArea (uint32_t page_num) {

    // 1. Проверяем, инициализирован ли драйвер вообще
    if (flash_hw == NULL) {
        return FLASH_AREA_INVALID;
    }

    // 2. Проверяем физический выход за границы чипа
    if (page_num >= flash_hw->chip_size) {
        return FLASH_AREA_INVALID;
    }

    // 3. Определяем область на основе вашей таблицы
    if (page_num < flash_hw->nonx_area_begin) {
        return FLASH_AREA_X;
    } else {
        return FLASH_AREA_NONX;
    }
}
// ----------------------------------------------------------------------------
// Чтение потоком
void W25_ReadData (uint32_t addr, uint8_t *buffer, uint32_t length) {

    W25_Select();

    // Сброс первого байта
    //SPIM3_ClearReg();

    SPI_ExchangeByte (W25_CMD_READ_DATA);
    SPI_ExchangeByte ((uint8_t)(addr >> 16));
    SPI_ExchangeByte ((uint8_t)(addr >> 8));
    SPI_ExchangeByte ((uint8_t)addr);

    while (length--) {
        *buffer++ = SPI_ExchangeByte (0xFF);
    }

    W25_Unselect();
}

// ----------------------------------------------------------------------------
// Чтение страницы по её индексу
uint8_t W25_ReadPage (uint32_t page_idx, uint8_t *buffer, uint16_t length) {

    if (W25_CheckPageAddr(page_idx) == 1) {

        uint32_t addr = (page_idx * W25Q_PAGE_SIZE);
        W25_ReadData (addr, buffer, length);

        return 0;
    }

    return 1;
}
// ----------------------------------------------------------------------------
// Чтение страницы по её индексу
sectorPage_t *W25_ReadPageEx (uint32_t page_idx, uint16_t len) {

    memset (temp_page.raw, 0xFF, W25Q_PAGE_SIZE);

    if ((len > 0 && len <= W25Q_PAGE_SIZE) && W25_CheckPageAddr(page_idx) == 1) {

        uint32_t addr = (page_idx * W25Q_PAGE_SIZE);
        W25_ReadData (addr, temp_page.raw, len);
    }

    return &temp_page;
}

// ----------------------------------------------------------------------------
// Чтение системного файла
systemRecord_t *W25_ReadSystemFile() {

    if (flash_hw == NULL) {
        return NULL;
    }

    system_page.data.FlashID = flash_hw->chip_id;

    uint32_t target_page_addr = 0;

    // Ищем страницу. Передаем адрес напрямую, без создания массива из 1 элемента
    uint16_t t_fnd = W25_BuildFileMap_SinglePass (0, &target_page_addr, 1);

    if (t_fnd == 1) {
        sectorPage_t *p_data = W25_ReadPageEx (target_page_addr / W25Q_PAGE_SIZE, W25Q_PAGE_SIZE);

        if (p_data != NULL) {
            // Безопасное копирование: проверяем, чтобы не скопировать лишнего
            // Убедитесь, что p_data->data содержит как минимум sizeof(systemRecord_t) байт!
            memcpy (&system_page.data, p_data->data, sizeof (systemRecord_t));

            // ОБЯЗАТЕЛЬНО, если W25_ReadPageEx использует внутри malloc/calloc:
            // free(p_data);
        }
    }

    return &system_page;
}

// ----------------------------------------------------------------------------
// Запись нового/обновление системного файла
uint8_t W25_WriteSystemFile (systemRecord_t *sys_data) {

    // 1. Ищем и стираем старый системный файл
    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {

        // Читаем только заголовок
        W25_ReadPage (page_idx, temp_page.raw, 7);
        if ((temp_page.STATUS == SECTOR_STATUS_USABLE || temp_page.STATUS == SECTOR_STATUS_LAST_USABLE) && temp_page.ID == 0) {

            // Нашли системный файл, удаляем его
            uint8_t st_data = SECTOR_STATUS_TRASH;
            uint32_t addr = page_idx * W25Q_PAGE_SIZE;

            W25_WritePage (addr, &st_data, 1);
            break;
        }
    }

    uint32_t new_sf_page_addr = 0;

    // Попытки записи нового системного файла до окончания свободных страниц
    while (new_sf_page_addr < flash_hw->totalx_pages) {

        uint32_t new_sf_page_addr = W25_GetFreeAddress();
        if (new_sf_page_addr == W25_PAGE_ZERO_ID) {

            // Нет места в протокольной области
            return 1;
        }

        memset (temp_page.raw, 0xFF, W25Q_PAGE_SIZE);
        memcpy (temp_page.data, sys_data->raw, sizeof (systemRecord_t));

        temp_page.STATUS = SECTOR_STATUS_LAST_USABLE;
        temp_page.SEGMENT = 0;
        temp_page.ID = 0;
        temp_page.LENGTH = sizeof (systemRecord_t);
        temp_page.CRC8 = crc8_calculate (temp_page.data, temp_page.LENGTH);

        if (W25_WritePage_Verified (new_sf_page_addr, temp_page.raw, W25Q_PAGE_SIZE) == 0) {

            // успешная запись, выходим... Дело сделано!
            return 0;
        }
    }

    return 1;
}

// ----------------------------------------------------------------------------
// Проверка, все ли байты массива соответствуют эталону?
uint8_t W25_DataIsEqu(uint8_t *buffer, uint16_t length, uint8_t value) {

    if (buffer == NULL) { return 1; }

    for (uint16_t data_idx = 0; data_idx < length; data_idx++) {

        if (buffer[data_idx] != value) { return 0; }
    }

    return 1;
}

// ----------------------------------------------------------------------------
// Чтение страницы по её индексу
sectorPage_t *W25_ReadFileSegment (uint16_t fileID, uint16_t seg_idx, volatile uint32_t *seg_page_idx) {

    // Инициализация статуса
    last_error_code = LAST_ERROR_NO_ERROR;

    // Безопасно зануляем внешнюю переменную через разыменование
    if (seg_page_idx != NULL) {
        *seg_page_idx = 0;
    }

    // Очищаем буфер перед началом работы
    memset (temp_page.raw, 0xFF, W25Q_PAGE_SIZE);

    uint8_t page_found = 0;  // Флаг успешного поиска

    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;

        // Читаем только заголовок
        W25_ReadData (current_addr, temp_page.raw, 7);

        // Пропускаем пустые и "мусорные" сектора
        if (temp_page.STATUS == 0xFF || temp_page.STATUS == SECTOR_STATUS_TRASH) {
            continue;
        }

        // Если это наш файл
        if (temp_page.ID == fileID) {

            //printf("page:%u, ID:%u, SEG:%u\r\n", (unsigned int)page_idx, (unsigned int)temp_page.ID, (unsigned int)temp_page.SEGMENT);
            
            uint16_t rdseg_idx = temp_page.SEGMENT;

            if (seg_idx == rdseg_idx) {

                // Записываем индекс найденной страницы наружу
                if (seg_page_idx != NULL) {
                    *seg_page_idx = page_idx;
                }

                memset (temp_page.raw, 0xFF, W25Q_PAGE_SIZE);
                
                // Читаем страницу полностью!
                W25_ReadData (current_addr, temp_page.raw, W25Q_PAGE_SIZE);

                //W25_PrintPageDump(0, temp_page.raw, 16);

                page_found = 1;  // Поднимаем флаг
                break;           // Выходим из цикла, страница найдена!
            }
        }
    }

    // Обработка результата поиска
    if (page_found != 1) {
        // Если цикл дошел до конца и ничего не нашел

        memset (temp_page.raw, 0, W25Q_PAGE_SIZE);
        last_error_code = LAST_ERROR_NOT_FOUND;

        return NULL;  // В случае ошибки безопаснее вернуть NULL
    }

    // Если нашли, last_error_code остается LAST_ERROR_NO_ERROR
    return &temp_page;
}

// ----------------------------------------------------------------------------
// Удаление сегмента указанного файла
uint8_t W25_EraseFileSegment (uint16_t fileID, uint16_t seg_idx) {

    // Инициализация статуса
    last_error_code = LAST_ERROR_NO_ERROR;

    uint8_t page_found = 0;  // Флаг успешного поиска

    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;

        // Читаем только заголовок
        W25_ReadData (current_addr, temp_page.raw, 7);

        // Пропускаем пустые и "мусорные" сектора
        if (temp_page.STATUS == 0xFF || temp_page.STATUS == SECTOR_STATUS_TRASH) {
            continue;
        }

        // Если это наш файл
        if (temp_page.ID == fileID) {

            uint16_t rdseg_idx = temp_page.SEGMENT;

            if (seg_idx == rdseg_idx) {

                uint8_t bad_status = SECTOR_STATUS_TRASH;
                W25_WritePage (current_addr, &bad_status, 1);

                page_found = 1;  // Поднимаем флаг
                break;           // Выходим из цикла, страница найдена!
            }
        }
    }

    // Обработка результата поиска
    if (!page_found) {
        
        // Если цикл дошел до конца и ничего не нашел
        last_error_code = LAST_ERROR_NOT_FOUND;
        return 0;
    }

    // Если нашли, last_error_code остается LAST_ERROR_NO_ERROR
    return 1;
}
// ----------------------------------------------------------------------------
// Функция ожидания завершения внутренних операций (стирание/запись)
uint8_t W25_WaitBusy (void) {

    uint8_t status = 0;
    uint32_t counter = 0;

    do {
        W25_Select();
        SPI_ExchangeByte (W25_CMD_READ_STATUS_1);
        status = SPI_ExchangeByte (0xFF);
        W25_Unselect();

        // Небольшая задержка, чтобы не перегружать шину SPI постоянными запросами
        Delay_Ms (1);

        counter++;

        // ТАЙМАУТ ПРЕДОСТОРОЖНОСТИ (на всякий случай)
        // Если флешка не ответила за 40 секунд - что-то пошло не так
        if (counter > 40000) {

            return 1;
        }

    } while (status & W25_STATUS_BUSY);

    return 0;
}
// ----------------------------------------------------------------------------
/**
 * @brief Запускает полное стирание чипа (Chip Erase).
 * Не блокирует выполнение основной программы.
 */
void W25_WipeAll_Async (uint8_t wait) {

    W25_WriteEnable();

    W25_Select();
    SPI_ExchangeByte (0xC7);  // Команда Chip Erase
    W25_Unselect();

    if (wait) {

        W25_WaitBusy();
    }

    // Сбрасываем курсор ФС сразу, так как данные "логически" мертвы
    last_free_page_idx = 0;
}
// ----------------------------------------------------------------------------
// Стирание сектора (4 КБ). Адрес должен быть кратен 4096.
void W25_EraseSector4K (uint32_t addr) {

    W25_WriteEnable();

    W25_Select();

    SPI_ExchangeByte (W25_CMD_SECTOR_ERASE4K);
    SPI_ExchangeByte ((uint8_t)(addr >> 16));
    SPI_ExchangeByte ((uint8_t)(addr >> 8));
    SPI_ExchangeByte ((uint8_t)addr);

    W25_Unselect();

    W25_WaitBusy();  // Стирание сектора занимает ~45-400 мс
}
// ----------------------------------------------------------------------------
// Полное стирание флешки
void W25_WipeAll() {

    W25_WriteEnable();

    W25_Select();

    SPI_ExchangeByte (W25_CMD_ERASE_ALL);

    W25_Unselect();

    W25_WaitBusy();  // Стирание сектора занимает ~45-400 мс
}
// ----------------------------------------------------------------------------
// Запись страницы (до 256 байт). Данные не должны пересекать границу страницы (256 байт).
// Внепротокольный метод
void W25_WritePage (uint32_t addr, uint8_t *buffer, uint16_t length) {

    if (length > W25Q_PAGE_SIZE)
        length = W25Q_PAGE_SIZE;

    W25_WriteEnable();

    W25_Select();

    SPI_ExchangeByte (W25_CMD_PAGE_PROGRAM);
    SPI_ExchangeByte ((uint8_t)(addr >> 16));
    SPI_ExchangeByte ((uint8_t)(addr >> 8));
    SPI_ExchangeByte ((uint8_t)addr);

    while (length--) {
        SPI_ExchangeByte (*buffer++);
    }

    W25_Unselect();
    W25_WaitBusy();  // Запись страницы занимает ~0.7-3 мс
}
// ----------------------------------------------------------------------------
// Начало Сектора (4 Кб), в котором лежит адрес
uint32_t W25_GetSectorAddr (uint32_t addr) {

    return (addr & ~(W25Q_SECTOR_SIZE - 1));
}
// ----------------------------------------------------------------------------
// Начало Блока (64 Кб), в котором лежит адрес
uint32_t W25_GetBlockAddr (uint32_t addr) {

    return (addr & ~(W25Q_BLOCK_SIZE - 1));
}
// ----------------------------------------------------------------------------
// Индекс страницы (0..16383)
uint16_t W25_GetPageIndex (uint32_t addr) {

    return (uint16_t)(addr / W25Q_PAGE_SIZE);
}
// ----------------------------------------------------------------------------
/**
 * Ищет свободные страницы и записывает их адреса в массив.
 * @param address_list - массив, куда запишем адреса
 * @param required_count - сколько страниц нужно найти
 * @return 0 - успех, 1 - не хватило места в памяти
 */
// ----------------------------------------------------------------------------
// Получение первой свободной страницы памяти
uint32_t W25_GetFreeAddress() {

    // Проходим по всей флешке (шаг — размер страницы 256 байт)
    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;
        uint8_t status_byte = 0;

        // Читаем только 1-й байт (SECTOR_STATUS_FLAG)
        W25_ReadData (current_addr, &status_byte, 1);

        // Если сектор чист
        if (status_byte == SECTOR_STATUS_NEW) {

            return current_addr;
        }
    }

    return W25_PAGE_ZERO_ID;
}
// ----------------------------------------------------------------------------
/* uint8_t W25_GetFreeAddresses (uint32_t *address_list, uint16_t required_count) {

    uint16_t found_so_far = 0;

    // Проходим по всей флешке (шаг — размер страницы 256 байт)
    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;
        uint8_t status_byte = 0;

        // Читаем только 1-й байт (SECTOR_STATUS_FLAG)
        W25_ReadData (current_addr, &status_byte, 1);

        // Если сектор чист
        if (status_byte == SECTOR_STATUS_NEW) {

            address_list[found_so_far] = current_addr;
            found_so_far++;

            // Если нашли всё, что просили — выходим сразу
            if (found_so_far == required_count) {

                return 0;
            }
        }
    }

    return 1;  // Не нашли нужного количества свободных секторов
} */
// ----------------------------------------------------------------------------
/**
 * Ищет свободные страницы и записывает их адреса в массив, исключая указанный сектор.
 * @param address_list - массив для адресов страниц
 * @param required_count - сколько страниц нужно найти
 * @param excluded_sector_addr - адрес начала физического сектора (4кБ), который нужно пропустить
 * @return 0 - успех, 1 - не хватило места
 */
uint8_t W25_GetFreeAddressesEx (uint32_t *address_list, uint16_t required_count, uint32_t excluded_sector_addr) {

    uint16_t found_so_far = 0;
    uint32_t start_search = last_free_page_idx;

    for (uint32_t i = 0; i < flash_hw->totalx_pages; i++) {

        // Кольцевой индекс (идем от старого места до конца, потом с начала)
        uint32_t current_page_idx = (start_search + i) % flash_hw->totalx_pages;
        uint32_t current_addr = current_page_idx * W25Q_PAGE_SIZE;

        // Проверка исключенного сектора
        if (current_addr >= excluded_sector_addr && current_addr < (excluded_sector_addr + W25Q_SECTOR_SIZE)) {
            continue;
        }

        uint8_t status_byte;
        W25_ReadData (current_addr, &status_byte, 1);

        if (status_byte == SECTOR_STATUS_NEW) {
            address_list[found_so_far] = current_addr;
            found_so_far++;

            if (found_so_far == required_count) {
                // ОБНОВЛЯЕМ КУРСОР: следующая запись начнется после этой страницы
                last_free_page_idx = (current_page_idx + 1) % flash_hw->totalx_pages;
                return 0;
            }
        }
    }
    return 1;  // Места нет
}
// ----------------------------------------------------------------------------
/**
 * Сканирует всю флеш за один проход и собирает карту конкретного файла.
 * @param target_id - ID файла, который ищем
 * @param out_map - массив для адресов (индекс = номер сегмента)
 * @param max_segs - размер массива
 * @return Общее количество уникальных найденных сегментов
 */
uint16_t W25_BuildFileMap_SinglePass (uint16_t target_id, uint32_t *out_map, uint16_t max_segs) {

    uint16_t found_count = 0;

    // 1. Инициализируем карту "пустотой"
    for (uint16_t i = 0; i < max_segs; i++) out_map[i] = W25_PAGE_ZERO_ID;

    // 2. Линейный скан всей флеш от 0 до конца
    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {
        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;

        // Читаем только заголовок (7 байт)
        W25_ReadData (current_addr, temp_page.raw, 7);

        // Пропускаем пустые и "мусорные" сектора
        if (temp_page.STATUS == 0xFF || temp_page.STATUS == SECTOR_STATUS_TRASH)
            continue;

        // Если это наш файл (неважно, обычный сегмент или последний)
        if (temp_page.ID == target_id) {
            uint16_t seg_idx = temp_page.SEGMENT;

            if (seg_idx < max_segs) {
                // Если мы нашли дубликат сегмента дальше по памяти,
                // перезаписываем адрес (считаем его более свежим)
                if (out_map[seg_idx] == 0xFFFFFFFF) {
                    found_count++;
                }
                out_map[seg_idx] = current_addr;
            }
        }
    }

    return found_count;
}
// ----------------------------------------------------------------------------
/**
 * Сканирует всю флеш за один проход и собирает карту конкретного файла.
 * @return Общее количество найденных сегментов в указанном диапазоне
 */
uint16_t W25_BuildFileMapEx (const Params16 target_id, uint16_t *out_map) {
    // target_id.param1 - искомый ID файла
    // target_id.param2 - размер массива out_map (сколько сегментов ищем)
    // target_id.param3 - с какого сегмента начинаем (offset)

    uint16_t found_count = 0;
    uint16_t start_seg = target_id.param3;
    uint16_t max_segments = target_id.param2;

    // 1. Чистим карту
    for (uint16_t i = 0; i < max_segments; i++) out_map[i] = 0xFFFF;

    // 2. Скан
    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {
        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;
        W25_ReadData (current_addr, temp_page.raw, 7);

        // Проверка статуса (пропускаем битые/пустые)
        if (temp_page.STATUS != SECTOR_STATUS_USABLE &&
            temp_page.STATUS != SECTOR_STATUS_LAST_USABLE) {
            continue;
        }

        // Если файл наш
        if (temp_page.ID == target_id.param1) {
            uint16_t current_seg = temp_page.SEGMENT;

            //printf ("page :%08x\r\n", (unsigned int)page_idx);

            // Попадает ли текущий сегмент в наше "окно" поиска?
            if (current_seg >= start_seg && current_seg < (start_seg + max_segments)) {

                //printf ("added :%08x\r\n", (unsigned int)page_idx);
                // ВЫЧИСЛЯЕМ ИНДЕКС: куда в массив положить номер страницы
                uint16_t map_idx = current_seg - start_seg;

                out_map[map_idx] = (uint16_t)page_idx;
                found_count++;
            }
        }
    }

    return found_count;  // Вернет количество найденных фрагментов в этом окне
}

// ----------------------------------------------------------------------------
/**
 * Сканирует всю флеш за один проход и собирает карту конкретного файла.
 * @return Общее количество найденных сегментов в указанном диапазоне
 */
uint16_t W25_BuildFileMapEx2 (const Params16 target_id, uint8_t *out_map) {
    // target_id.param1 - искомый ID файла
    // target_id.param2 - размер массива out_map (сколько сегментов ищем)
    // target_id.param3 - с какого сегмента начинаем (offset)

    uint16_t found_count = 0;
    uint16_t start_seg = target_id.param3;
    uint16_t max_segments = target_id.param2;

    // Int16x tmp_data;

    // 1. Чистим карту
    for (uint16_t i = 0; i < max_segments * 2; i++) {

        out_map[i] = 0xFF;
    }

    // 2. Скан
    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;
        W25_ReadData (current_addr, temp_page.raw, 7);

        // Проверка статуса (пропускаем битые/пустые)
        if (temp_page.STATUS != SECTOR_STATUS_USABLE &&
            temp_page.STATUS != SECTOR_STATUS_LAST_USABLE) {
            continue;
        }

        // Если файл наш
        if (temp_page.ID == target_id.param1) {
            uint16_t current_seg = temp_page.SEGMENT;

            // Попадает ли текущий сегмент в наше "окно" поиска?
            // Проверяем, попадает ли сегмент в наше окно (например, от 10 до 10+128)
            if (current_seg >= start_seg && current_seg < (start_seg + max_segments)) {

                // ВЫЧИСЛЯЕМ БАЙТОВЫЙ ИНДЕКС:
                // Если нашли 10-й сегмент при start_seg=10, индекс будет (10-10)*2 = 0
                // Если нашли 11-й сегмент при start_seg=10, индекс будет (11-10)*2 = 2
                uint16_t byte_idx = (current_seg - start_seg) * 2;

                // Сохраняем адрес страницы (uint32_t page_idx кастуем в 2 байта)
                // Little-endian (как в твоем дампе: 24 00)
                out_map[byte_idx] = (uint8_t)(page_idx & 0xFF);
                out_map[byte_idx + 1] = (uint8_t)((page_idx >> 8) & 0xFF);

                found_count++;
            }
        }
    }

    return found_count;  // Вернет количество найденных фрагментов в этом окне
}

// ----------------------------------------------------------------------------
// Поиск всех имен файлов на массиве флеш
uint16_t W25_createFilesMap_SinglePass (fileRecord_t *out_map, uint16_t max_segs) {

    uint16_t found_count = 0;

    // 1. Инициализируем карту "пустотой"
    for (uint16_t i = 0; i < max_segs; i++) out_map[i].data.ID = W25_FILE_ZERO_ID;

    // 2. Линейный скан всей флеш
    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {
        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;

        W25_ReadData (current_addr, temp_page.raw, 7);

        if (temp_page.STATUS == SECTOR_STATUS_USABLE || temp_page.STATUS == SECTOR_STATUS_LAST_USABLE) {
            fileRecord_t *target = NULL;

            // 1. Ищем, есть ли уже такой ID
            for (uint16_t i = 0; i < found_count; i++) {
                if (out_map[i].data.ID == temp_page.ID) {
                    target = &out_map[i];
                    break;
                }
            }

            // 2. Если не нашли — инициализируем новый слот
            if (target == NULL && found_count < max_segs) {

                target = &out_map[found_count++];
                target->data.ID = temp_page.ID;
                target->data.Size = 0;
                target->data.Blocks = 0;
                target->data.Type = 0;
                target->data.Address = 0;

                //printf ("add file: ID %04X\r\n", target->data.ID);
            }

            // 3. Общая логика обновления для обоих случаев
            if (target != NULL) {
                target->data.Size += temp_page.LENGTH;
                target->data.Blocks++;
            }
        }
    }

    return found_count;
}

// ----------------------------------------------------------------------------
// Поиск всех имен файлов на массиве флеш 2
// Возврат - начало следующего поиска или W25Q_TOTAL_PAGES, если поиск закончен
uint32_t W25_createFilesMap_SinglePassEx (uint32_t start_page_idx, fileRecord_t *out_map) {

    out_map->data.ID = 0;
    out_map->data.Size = 0;
    out_map->data.Blocks = 0;

    if (start_page_idx >= flash_hw->totalx_pages) {

        return flash_hw->totalx_pages;
    }

    // 2. Линейный скан флеш
    for (uint32_t page_idx = start_page_idx; page_idx < flash_hw->totalx_pages; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;
        W25_ReadData (current_addr, temp_page.raw, 7);

        if (temp_page.STATUS == SECTOR_STATUS_USABLE || temp_page.STATUS == SECTOR_STATUS_LAST_USABLE) {

            if (temp_page.SEGMENT == 0) {

                out_map->data.ID = temp_page.ID;

                return page_idx + 1;
            }
        }
    }

    return flash_hw->totalx_pages;
}

// ----------------------------------------------------------------------------
// Пошаговый поиск всех имен файлов на протокольном поле флеш-памяти
void W25_createFilesMap_SinglePass2 (callback_fileinfo_found OnFileFound) {

    uint32_t start_find_page = 0;

    // 2. Линейный скан флеш
    for (uint32_t page_idx = start_find_page; page_idx < flash_hw->totalx_pages; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;
        W25_ReadData (current_addr, temp_page.raw, 7);

        if (temp_page.STATUS == SECTOR_STATUS_USABLE || temp_page.STATUS == SECTOR_STATUS_LAST_USABLE) {

            if (temp_page.SEGMENT == 0) {

                if (OnFileFound (temp_page.ID) != 1) {
                    break;
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Получение свойств указанного файла
// 0 - свойства получены
// 1 - файл не найден
// 2 - поиск невозможен
uint8_t W25_getFilesProperty (fileRecord_t *out_fileprops) {

    if (out_fileprops == NULL) { return 2; }

    // 1. Подготовка данных
    out_fileprops->data.Blocks = 0;
    out_fileprops->data.Size = 0;
    out_fileprops->data.Type = 0;

    // 2. Линейный скан всей флеш
    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;

        W25_ReadData (current_addr, temp_page.raw, 7);

        if (temp_page.STATUS == SECTOR_STATUS_USABLE || temp_page.STATUS == SECTOR_STATUS_LAST_USABLE) {

            // 3. Общая логика обновления для обоих случаев
            if (temp_page.ID == out_fileprops->data.ID) {

                out_fileprops->data.Size += temp_page.LENGTH;
                out_fileprops->data.Blocks++;
            }
        }
    }

    return (out_fileprops->data.Blocks == 0) ? 1 : 0;
}
// ----------------------------------------------------------------------------
/**
 * @return 0 - успех, 1 - ошибка верификации (битая страница)
 */
uint8_t W25_WritePage_Verified (uint32_t addr, uint8_t *buffer, uint16_t length) {

    // 1. Записываем
    W25_WritePage (addr, buffer, length);

    // printf ("addr: %u, len: (%u)\r\n", (unsigned int)addr, (unsigned int)length);

    sectorPage_t* chk_page = W25_ReadPageEx (W25_GetPageIndex(addr), length);

    // 2. Побайтная верификация без выделения буфера в RAM
    for (uint16_t i = 0; i < length; i++) {

        uint8_t flash_byte = chk_page->raw[i];

        if (flash_byte != buffer[i]) {

            // Пытаемся пометить как BAD (сбрасываем статус в 0xF0)
            uint8_t bad_status = SECTOR_STATUS_BAD;
            W25_WritePage (addr, &bad_status, 1);

            return 1;  // Ошибка верификации
        }
    }

    return 0;  // Успех
}
// ----------------------------------------------------------------------------
// Проверка на существование файла
uint8_t W25_FileExists (uint16_t file_id) {

    uint32_t first_segment_addr;

    // Пытаемся построить карту, запросив всего 1 сегмент (голову)
    if (W25_BuildFileMap_SinglePass (file_id, &first_segment_addr, 1) > 0) {
        return 1;  // Карта построена (хотя бы частично) -> файл есть
    }

    return 0;  // Маппер ничего не нашел
}
// ----------------------------------------------------------------------------
// Частичное (удаленное) сохранение файла частями - внешняя логика обязана создать все части отдельно
uint8_t W25_StoreBinDataPart (uint8_t *bin_data, uint16_t length) {

    if (bin_data) {

        // Заполняем структуру
        memset (temp_page.raw, 0xFF, W25Q_PAGE_SIZE);
        memcpy (temp_page.raw, &bin_data[0], length);

        uint8_t retry = 0;

        while (retry < 3) {  // Лимит попыток

            uint32_t current_addr = W25_GetFreeAddress();

            // Ищем свободное место
            if (current_addr == W25_PAGE_ZERO_ID) {

                return 2;
            }

            // W25_PrintPageDump(current_addr, temp_page.raw);
            //printf ("wr_page_addr: %08X\r\n", (unsigned int)current_addr);

            // Пытаемся записать с проверкой
            if (W25_WritePage_Verified (current_addr, temp_page.raw, W25Q_PAGE_SIZE) == 0) {

                return 0;
            }

            retry++;
        }

        // Если попали сюда — страница оказалась битой.
        // W25_WritePage_Verified уже пометила её как BAD.
        // while(1) заставит нас найти НОВЫЙ адрес и попробовать снова.
    }

    return 2;
}
// ----------------------------------------------------------------------------
// Сохранение данных во флеш-память
uint16_t W25_StoreBinData (uint8_t *bin_data, uint32_t total_size, uint16_t data_id) {

    // Нет данных, это не ошибка
    if (total_size == 0) {
        return 4;
    }

    // 1. Считаем количество нужных чанков
    uint16_t chunks_count = (total_size + W25Q_PAGE_MAX_DATA_SIZE - 1) / W25Q_PAGE_MAX_DATA_SIZE;

    // 2. Создаем временный массив для адресов (в стеке, если chunks_count небольшой, иначе static)
    uint32_t page_addresses[chunks_count];

    // 3. За один проход собираем адреса свободных мест
    if (W25_GetFreeAddresses (page_addresses, chunks_count) != 0) {

        return 1;  // Ошибка: память переполнена
    }

    // 4. Пишем данные, используя подготовленный список адресов
    for (uint16_t i = 0; i < chunks_count; i++) {

        uint32_t offset = i * W25Q_PAGE_MAX_DATA_SIZE;
        uint32_t remaining = total_size - offset;
        uint16_t current_len = (remaining > W25Q_PAGE_MAX_DATA_SIZE) ? W25Q_PAGE_MAX_DATA_SIZE : (uint16_t)remaining;

        // Заполняем структуру
        memset (temp_page.raw, 0xFF, W25Q_PAGE_SIZE);

        // МЕТКА ПОСЛЕДНЕГО ЧАНКА
        if (i == chunks_count - 1) {
            temp_page.STATUS = SECTOR_STATUS_LAST_USABLE;  // 0xFD
        } else {
            temp_page.STATUS = SECTOR_STATUS_USABLE;       // 0xFE
        }

        temp_page.ID = data_id;
        temp_page.SEGMENT = i;
        temp_page.LENGTH = (uint8_t)current_len;

        memcpy (temp_page.data, &bin_data[offset], current_len);

        temp_page.CRC8 = crc8_calculate (temp_page.data, temp_page.LENGTH);

        // Пишем по адресу из нашего списка
        // W25_WritePage (page_addresses[i], page.raw, W25Q_PAGE_SIZE);

        uint32_t target_addr;
        uint8_t retry = 0;

        while (retry < 3) {  // Лимит попыток

            // Ищем свободное место (1 штуку)
            if (W25_GetFreeAddresses (&target_addr, 1) != 0) {
                return 2;
            }

            // Пытаемся записать с проверкой
            if (W25_WritePage_Verified (target_addr, temp_page.raw, W25Q_PAGE_SIZE) == 0) {
                break;  // Успех, переходим к следующему сегменту файла
            }

            retry++;
        }

        if (retry == 3) {
            return 3;
        }  // Железная проблема

        // Если попали сюда — страница оказалась битой.
        // W25_WritePage_Verified уже пометила её как BAD.
        // while(1) заставит нас найти НОВЫЙ адрес и попробовать снова.
    }

    return 0;
}
// ----------------------------------------------------------------------------
/**
 * Читает файл целиком в предоставленный буфер.
 * @param file_id - ID (имя) файла
 * @param destination_buffer - куда копировать данные
 * @param max_buffer_size - размер приемного буфера (защита от переполнения)
 * @return Количество реально прочитанных байт или 0 при ошибке
 */
uint32_t W25_ReadFileByID (uint16_t file_id, uint8_t *destination_buffer, uint32_t max_buffer_size) {

    const uint16_t MAX_SEGS = 1024;  // Максимально допустимое кол-во сегментов (около 250КБ)
    uint32_t file_map[MAX_SEGS];
    uint32_t total_read_bytes = 0;

    // 1. Получаем карту адресов за один проход по флеш
    uint16_t found_count = W25_BuildFileMap_SinglePass (file_id, file_map, MAX_SEGS);

    if (found_count == 0)
        return 0;  // Файл не найден

    // 2. Идем по карте сегментов строго по порядку (0, 1, 2...)
    for (uint16_t i = 0; i < MAX_SEGS; i++) {
        uint32_t addr = file_map[i];

        // Если встретили пустоту в карте до того, как нашли все сегменты — файл "дырявый"
        if (addr == W25_PAGE_ZERO_ID) {
            // Если мы уже что-то прочитали, но цепочка прервалась — это ошибка целостности
            if (total_read_bytes > 0)
                break;
            continue;
        }

        // 3. Читаем страницу целиком (или заголовок + данные)
        W25_ReadData (addr, temp_page.raw, W25Q_PAGE_SIZE);

        // 4. Проверка: не переполним ли мы буфер назначения?
        if (total_read_bytes + temp_page.LENGTH > max_buffer_size) {
            // Можно либо вернуть ошибку, либо прочитать сколько влезет
            break;
        }

        // 5. Копируем полезные данные (начиная с 7-го байта) в буфер
        memcpy (&destination_buffer[total_read_bytes], &temp_page.data, temp_page.LENGTH);

        uint8_t crc_temp = crc8_calculate (temp_page.data, temp_page.LENGTH);
        if (crc_temp != temp_page.CRC8) {

            // Внимание! Мы прочитали битый сектор!
        }

        total_read_bytes += temp_page.LENGTH;

        // Здесь будет вызов обработчика приема данных

        // 6. Если статус "Последний чанк", прекращаем чтение
        if (temp_page.STATUS == 0xFD) {
            break;
        }
    }

    return total_read_bytes;
}
// -----------------------------------------------------------------------------
/**
 * Читает файл частями (посекторно с вызовом обработчика)
 * @param file_id - ID (имя) файла
 * @return Количество реально прочитанных байт или 0 при ошибке
 */
uint32_t W25_ReadFileByIDEx (uint16_t file_id, flash_callback_data_read OnDataRead) {

    uint32_t total_read_bytes = 0;
    uint32_t current_offset = 0;  // С какого логического индекса части начинаем поиск
    uint8_t file_ended = 0;

    while (file_ended == 0) {

        Params16 search_params = {
            .param1 = file_id,
            .param2 = W25Q_CHUNK_SIZE,
            .param3 = (uint16_t)current_offset};  // Стартовый индекс окна (0, 256, 512...)

        memset (tmp_page_map, 0, sizeof (tmp_page_map));

        // 1. Ищем физические адреса для частей внутри текущего окна [offset ... offset + 255]
        uint16_t found_in_window = W25_BuildFileMapEx (search_params, tmp_page_map);

        if (found_in_window == 0)
            break;  // В данном диапазоне индексов больше ничего не существует

        // 2. Обрабатываем только то количество, которое реально нашли
        for (uint16_t i = 0; i < found_in_window; i++) {

            if (tmp_page_map[i] == W25_FILE_ZERO_ID)
                continue;

            uint32_t addr = (uint32_t)tmp_page_map[i] * W25Q_PAGE_SIZE;

            W25_ReadData (addr, temp_page.raw, W25Q_PAGE_SIZE);

            if (crc8_calculate (temp_page.data, temp_page.LENGTH) == temp_page.CRC8) {
                total_read_bytes += temp_page.LENGTH;

                if (OnDataRead (addr, temp_page.SEGMENT, temp_page.data, temp_page.LENGTH) != 0) {

                    file_ended = 1;
                    break;
                }
            }

            if (temp_page.STATUS == 0xFD) {
                file_ended = 1;
                break;
            }
        }

        // Если вышли по флагу (ошибка callback или маркер конца 0xFD) — завершаем работу
        if (file_ended) {
            break;
        }

        // Сдвигаем СТАРТОВЫЙ ИНДЕКС окна для следующей итерации поиска
        // Если искали 0..255, теперь будем искать 256..511
        current_offset += W25Q_CHUNK_SIZE;
    }

    return total_read_bytes;
}
// ----------------------------------------------------------------------------
/**
 * Логически помечает страницу как мусор (TRASH)
 */
void W25_SectorMarkAsTrash (uint32_t addr) {

    uint8_t trash_status = SECTOR_STATUS_TRASH;  // 0xFC или 0xFD

    // Прямая запись 1 байта (превращаем 1 в 0)
    W25_WritePage (addr, &trash_status, 1);
}
// ----------------------------------------------------------------------------
/**
 * Безопасно сохраняет данные под указанным ID.
 * Если файл с таким ID уже был, он будет аннулирован после успешной записи нового.
 */
uint16_t W25_HideFile (uint16_t file_id) {

    //uint16_t page_map[W25Q_CHUNK_SIZE];  // Массив индексов страниц
    uint32_t total_erased = 0;       // Количество удаленных страниц
    uint16_t current_offset = 0;     // С какого сегмента начинаем чтение
    uint8_t file_ended = 0;

    while (file_ended == 0) {

        // Подготавливаем параметры для поиска порции сегментов
        Params16 search_params = {
            .param1 = file_id,
            .param2 = W25Q_CHUNK_SIZE,
            .param3 = current_offset};

        memset (tmp_page_map, 0, sizeof (tmp_page_map));

        // 1. Ищем адреса страниц только для текущего "окна"
        uint16_t found_in_window = W25_BuildFileMapEx (search_params, tmp_page_map);

        if (found_in_window == 0)
            break;  // Больше ничего не нашли

        // 2. Обрабатываем найденные сегменты внутри окна
        for (uint16_t i = 0; i < found_in_window; i++) {

            if (tmp_page_map[i] == W25_FILE_ZERO_ID)
                continue;  // Пропуск, если сегмент не найден

            uint32_t addr = (uint32_t)tmp_page_map[i] * W25Q_PAGE_SIZE;

            // Читаем только заголовок (7 байт)
            W25_ReadData (addr, temp_page.raw, 7);

            switch (temp_page.STATUS) {

            case SECTOR_STATUS_LAST_USABLE:
            case SECTOR_STATUS_USABLE:

                break;

            default: continue;
            }

            // Если это наш файл
            if (temp_page.ID == file_id) {

                uint8_t bad_status = SECTOR_STATUS_TRASH;
                W25_WritePage (addr, &bad_status, 1);

                total_erased += 1;
            }

            // 4. Если это финальный сегмент — выходим совсем
            if (temp_page.STATUS == 0xFD) {
                file_ended = 1;
                break;
            }
        }

        // Сдвигаем окно для следующей итерации
        current_offset += W25Q_CHUNK_SIZE;

        // Если за проход нашли меньше, чем размер окна, и это не конец файла по статусу,
        // значит, дальше искать смысла может и не быть (зависит от логики вашей ФС)
        if (found_in_window < W25Q_CHUNK_SIZE && !file_ended) {

            // Можно добавить проверку на разрывы в нумерации
        }
    }

    return (total_erased == 0) ? 2 : 0;
}

// ----------------------------------------------------------------------------
// Поиск максимального НОМЕРА существующего файла (не ID!) внутри своего типа
// Чтоб получить из этого номера ID, используйте макрос MAKE_FILE_ID
uint16_t W25_FindMaxFileNUM(uint8_t file_type) {

    uint16_t result_num = 0;

    for (uint32_t page_idx = 0; page_idx < flash_hw->totalx_pages; page_idx++) {

        // Читаем только заголовок
        W25_ReadPage (page_idx, temp_page.raw, 7);
        if (temp_page.STATUS == SECTOR_STATUS_USABLE || temp_page.STATUS == SECTOR_STATUS_LAST_USABLE) {

            if (temp_page.SEGMENT == 0) {

                uint8_t f_type = GET_FILE_TYPE(temp_page.ID);
                uint16_t f_num = GET_FILE_NUM(temp_page.ID);

                if (file_type == f_type && f_num > result_num) {

                    result_num = f_num;
                }
            }
        }
    }

    return result_num;
}

// ----------------------------------------------------------------------------
// Методы проверки и обслуживания массива памяти
// ----------------------------------------------------------------------------
// На входе: номер твоей логической страницы (W25Q_TOTAL_PAGES - [0..16383])
// На выходе: адрес начала физического сектора для команды 0x20
uint32_t GetPhysicalSectorAddr (uint16_t page_number) {

    // 1. Находим номер физического сектора (от 0 до 1023)
    uint16_t sector_idx = page_number / 16;

    // 2. Переводим в абсолютный адрес
    return (uint32_t)sector_idx * W25Q_SECTOR_SIZE;
}
// ----------------------------------------------------------------------------
/**
 * @brief Расчет границ физического сектора (4кБ)
 * @param page_idx Номер вашей логической страницы (0..16383)
 * @param first_page Указатель для возврата индекса первой страницы в блоке стирания
 * @param last_page Указатель для возврата индекса последней страницы в блоке стирания
 */
void W25Q_GetEraseBoundaries (uint16_t page_idx, uint16_t *first_page, uint16_t *last_page) {
    // Вычисляем индекс физического сектора (0..1023)
    // Деление целых чисел автоматически отсекает остаток (аналог floor)
    uint16_t physical_sector_num = page_idx / 16;

    // Первая страница в этом физическом секторе
    *first_page = physical_sector_num * 16;

    // Последняя страница в этом физическом секторе
    *last_page = *first_page + 15;
}
// ----------------------------------------------------------------------------
/**
 * @param page_idx Номер страницы, которую хотим затронуть
 * @param start_page [out] Индекс первой страницы физического сектора 4кБ
 * @param end_page [out] Индекс последней страницы физического сектора 4кБ
 */
void W25Q_GetPhysicalBoundaries (uint16_t page_idx, uint16_t *start_page, uint16_t *end_page) {
    // 16 страниц в физическом секторе (4096 / 256 = 16)
    *start_page = page_idx & ~0xF;  // Сброс 4 младших бит (выравнивание по 16)
    *end_page = page_idx | 0xF;     // Установка 4 младших бит в 1
}
// ----------------------------------------------------------------------------
/**
 * @brief Собирает индексы занятых страниц в конкретном физическом секторе
 * @param sector_idx Номер физического сектора (0..1023)
 * @param found_pages Массив, куда запишем индексы (размер должен быть минимум 16)
 * @return uint8_t Количество найденных занятых страниц
 */
uint8_t W25_GetUsedPagesInSector (uint16_t sector_idx, uint16_t *found_pages) {

    uint8_t found_count = 0;
    uint8_t status_byte = 0;

    // 1. Находим индекс первой страницы в этом секторе (сектор 0 -> стр 0, сектор 1 -> стр 16)
    uint16_t start_page_idx = sector_idx * 16;

    // 2. Проходим по всем 16 страницам физического сектора
    for (uint8_t i = 0; i < 16; i++) {
        uint16_t current_page_idx = start_page_idx + i;

        // Вычисляем физический адрес начала текущей страницы
        uint32_t page_addr = (uint32_t)current_page_idx * W25Q_PAGE_SIZE;

        // Читаем только 1 байт (статус) по адресу начала страницы
        W25_ReadData (page_addr, &status_byte, 1);

        // Проверяем твои флаги: занята ли страница данными
        if (status_byte == SECTOR_STATUS_USABLE || status_byte == SECTOR_STATUS_LAST_USABLE) {

            found_pages[found_count] = current_page_idx;
            found_count++;
        }
    }

    return found_count;  // Сколько страниц в итоге "живые"
}
// ----------------------------------------------------------------------------
/**
 * @brief "Расселяет" (копирует) важные страницы из сектора перед его стиранием
 * @param target_sector_addr Адрес начала физического сектора (4кБ), который готовим к стиранию
 * @return 0 - успех, 1 - ошибка (нет места для переселения)
 */
uint8_t W25_EvacuateAndErase (uint16_t sector_idx) {

    uint32_t sector_addr = (uint32_t)sector_idx * W25Q_SECTOR_SIZE;
    uint32_t new_page_addr;

    uint16_t bad_pages_mask = 0;  // Битовая маска для 16 страниц (экономим RAM!)

    // 1. Сканируем все 16 страниц в физическом секторе
    for (uint8_t i = 0; i < 16; i++) {

        uint32_t current_page_addr = sector_addr + (i * W25Q_PAGE_SIZE);
        uint8_t status = 0;

        // Читаем только статус (первый байт)
        W25_ReadData (current_page_addr, &status, 1);

        if (status == SECTOR_STATUS_BAD) {
            bad_pages_mask |= (1 << i);  // Запоминаем, что i-я страница битая

            continue;
        }

        // 2. Если страница содержит живые данные (0xFE или 0xFD)
        if (status == SECTOR_STATUS_USABLE || status == SECTOR_STATUS_LAST_USABLE) {

            // Ищем ОДНО новое свободное место, исключая текущий сектор из поиска!
            if (W25_GetFreeAddressesEx (&new_page_addr, 1, sector_addr) != 0) {
                return 1;  // Ошибка: флешка переполнена, расселять некуда
            }

            // Читаем всю страницу целиком
            W25_ReadData(current_page_addr, temp_page.raw, W25Q_PAGE_SIZE);

            // Записываем её на новое найденное место
            W25_WritePage(new_page_addr, temp_page.raw, W25Q_PAGE_SIZE);
        }
    }

    // 3. Когда все важные страницы скопированы, стираем физический сектор целиком
    W25_EraseSector4K (sector_addr);

    // --- ШАГ 3: Восстанавливаем метки BAD ---
    for (uint8_t i = 0; i < 16; i++) {

        if (bad_pages_mask & (1 << i)) {

            uint32_t page_addr = sector_addr + (i * W25Q_PAGE_SIZE);
            uint8_t bad_status = SECTOR_STATUS_BAD;
            // Снова ставим черную метку на чистую страницу
            W25_WritePage (page_addr, &bad_status, 1);
        }
    }

    return 0;
}
// ----------------------------------------------------------------------------
/**
 * @brief Ищет ОДИН наиболее захламленный сектор и очищает его (Расселяет + Стирает)
 * @return 0 - мусора нет, 1 - чистых секторов не найдено
 */
uint8_t W25_CollectGarbage (void) {

    // Вычисляем максимальный номер сектора для сканирования
    // Если W25Q_NONX_AREA_BEGIN задан (> 0), то берем границу секторов, иначе сканируем весь чип
    uint16_t max_sectors = (flash_hw->nonx_area_begin > 0) ? (flash_hw->nonx_area_begin / 16) : flash_hw->total_4K_sectors;

    // Можно ограничить диапазон, если часть флешки занята системными данными
    for (uint16_t s = 0; s < max_sectors; s++) {

        uint16_t start_page = s * 16;
        uint8_t trash_in_this_sector = 0;

        // Сканируем статусы 16 страниц внутри сектора
        for (uint8_t p = 0; p < 16; p++) {

            uint32_t addr = (uint32_t)(start_page + p) * W25Q_PAGE_SIZE;
            uint8_t status;
            W25_ReadData (addr, &status, 1);

            switch (status) {

            case SECTOR_STATUS_NEW:
            case SECTOR_STATUS_BAD:

                break;

            case SECTOR_STATUS_USABLE:
            case SECTOR_STATUS_LAST_USABLE:

                break;

            default:

                trash_in_this_sector++;
                break;
            }
        }

        // КРИТЕРИЙ ВЫБОРА:
        // Нам нужен сектор, где много мусора И мало живых данных (меньше переписывать)
        if (trash_in_this_sector > 0) {

            if (W25_EvacuateAndErase (s) != 0) {

                return 1;
            }
        }
    }

    return 0;
}
// ----------------------------------------------------------------------------
// Получение информации о массиве памяти
sectorPage_t *W25_MemoryInfo (void) {

    memoryInfo_t mem_result;
    memset (&mem_result, 0, sizeof (mem_result));

    mem_result.chip_size = flash_hw->chip_size;
    mem_result.total_pages = flash_hw->totalx_pages;
    mem_result.cont_area_begin = flash_hw->nonx_area_begin;

    mem_result.cont_area_free = 0;

    // Линейный скан всей флеш
    for (uint32_t page_idx = 0; page_idx < flash_hw->chip_size; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;
        W25_ReadData (current_addr, temp_page.raw, 7);

        switch (temp_page.STATUS) {

        case SECTOR_STATUS_NEW:

            mem_result.free_pages += 1;
            break;

        case SECTOR_STATUS_USABLE:
        case SECTOR_STATUS_LAST_USABLE:

            mem_result.used_pages += 1;
            break;

        case SECTOR_STATUS_TRASH:

            mem_result.trash_pages += 1;
            break;

        case SECTOR_STATUS_BAD:

            mem_result.bad_pages += 1;
            break;
        }
    }

    if (flash_hw->nonx_area_begin > 0) {

        uint8_t byte1 = 0;
        uint8_t byte2 = 0;
        uint8_t byte3 = 0;

        for (uint32_t page_idx = flash_hw->nonx_area_begin; page_idx < flash_hw->chip_size; page_idx++) {

            uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;

            W25_ReadData (current_addr, &byte1, 1);
            W25_ReadData (current_addr + 127, &byte2, 1);
            W25_ReadData (current_addr + 255, &byte3, 1);

            if (byte1 == 0xFF && byte2 == 0xFF && byte3 == 0xFF) {

                mem_result.cont_area_free += 1;
            }
        }
    }

    temp_page.LENGTH = sizeof(mem_result);
    memcpy (temp_page.data, &mem_result, sizeof (mem_result));

    return &temp_page;
}

// ----------------------------------------------------------------------------
// Получение информации о массиве памяти
uint8_t W25_MemoryInfo2 (memoryInfo_t *mem_result) {

    //memset (&mem_result, 0, sizeof (mem_result));

    mem_result->chip_size = flash_hw->chip_size;
    mem_result->total_pages = flash_hw->totalx_pages;
    mem_result->cont_area_begin = flash_hw->nonx_area_begin;

    mem_result->cont_area_free = 0;

    // Линейный скан всей флеш
    for (uint32_t page_idx = 0; page_idx < flash_hw->chip_size; page_idx++) {

        uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;
        W25_ReadData (current_addr, temp_page.raw, 7);

        switch (temp_page.STATUS) {

        case SECTOR_STATUS_NEW:

            mem_result->free_pages += 1;
            break;

        case SECTOR_STATUS_USABLE:
        case SECTOR_STATUS_LAST_USABLE:

            mem_result->used_pages += 1;
            break;

        case SECTOR_STATUS_TRASH:

            mem_result->trash_pages += 1;
            break;

        case SECTOR_STATUS_BAD:

            mem_result->bad_pages += 1;
            break;
        }
    }

    if (flash_hw->nonx_area_begin > 0) {

        uint8_t byte1 = 0;
        uint8_t byte2 = 0;
        uint8_t byte3 = 0;

        for (uint32_t page_idx = flash_hw->nonx_area_begin; page_idx < flash_hw->chip_size; page_idx++) {

            uint32_t current_addr = page_idx * W25Q_PAGE_SIZE;

            W25_ReadData (current_addr, &byte1, 1);
            W25_ReadData (current_addr + 127, &byte2, 1);
            W25_ReadData (current_addr + 255, &byte3, 1);

            if (byte1 == 0xFF && byte2 == 0xFF && byte3 == 0xFF) {

                mem_result->cont_area_free += 1;
            }
        }
    }

    return 1;
}
// ----------------------------------------------------------------------------
// -- Непрерывная область (обслуживание)
// ----------------------------------------------------------------------------
// Чтение непрерывной области данных (индекс должен быть больше W25Q_TOTAL_PAGES) учитывается размер файла!
void W25_ReadContArea (uint32_t first_page_idx, uint16_t pages_read_count, uint32_t length, flash_callback_data_read OnDataRead) {

    uint32_t total_readed = 0;
    uint16_t current_segment = 0;

    for (uint32_t page_idx = 0; page_idx < pages_read_count; page_idx++) {

        if (page_idx + first_page_idx < flash_hw->chip_size) {

            uint32_t addr = (page_idx + first_page_idx) * W25Q_PAGE_SIZE;

            // Читаем физический сектор/страницу целиком
            W25_ReadData (addr, temp_page.raw, W25Q_PAGE_SIZE);

            // По умолчанию считаем, что полезных данных — вся страница
            uint16_t real_length = W25Q_PAGE_SIZE;

            // Если задана общая длина файла, проверяем остаток
            if (length > 0) {
                if (total_readed >= length) {
                    break;  // Все полезные данные уже прочитаны
                }

                uint32_t remaining = length - total_readed;
                if (remaining < W25Q_PAGE_SIZE) {
                    real_length = (uint16_t)remaining;  // Последний неполный чанк
                }
            }

            // Суммируем количество обработанных полезных байт
            total_readed += real_length;

            if (OnDataRead (addr, current_segment, temp_page.raw, real_length) != 0) {
                break;
            }

            current_segment += 1;

        } else {
            return;
        }
    }
}
// ----------------------------------------------------------------------------
// Поиск непрерывной области указанного размера
uint32_t W25_FindContArea (uint16_t length) {

    if (flash_hw->nonx_area_begin == 0) {
        return 0;  // Ошибка конфигурации или неверный размер
    }

    uint32_t area_begin = 0;     // Индекс первой пустой страницы в текущей цепочке
    uint32_t founded_pages = 0;  // Сколько пустых страниц подряд мы уже нашли

    // Настройка адреса начала поиска
    // 1. Берем адрес начала непрерывной области
    uint32_t free_area_begin = flash_hw->nonx_area_begin;

    // 2. Читаем системный файл...
    systemRecord_t *sys_rec = W25_ReadSystemFile();
    if (sys_rec != NULL && sys_rec->data.ContFreeBegin >= sys_rec->data.ContAreaBegin) {

        // И, если системный файл есть, берем значение начала чистой области из него
        free_area_begin = sys_rec->data.ContFreeBegin;
    }

    // Если запрос = 0, значит возвращаем начало пустой области
    if (length == 0) {

        return free_area_begin;
    }

    // Сканируем непрерывную зону от начала поиска и до конца физического чипа
    for (uint32_t page_idx = free_area_begin; page_idx < flash_hw->chip_size; page_idx++) {

        // Считаем честный физический адрес в байтах
        uint32_t addr = page_idx * W25Q_PAGE_SIZE;

        // Читаем физическую страницу напрямую из чипа в наш проверенный temp_page
        W25_ReadData (addr, temp_page.raw, W25Q_PAGE_SIZE);

        // Выводим в лог для проверки
        // printf ("Page: %d, Calc_CRC: %d, Data: %02X %02X\r\n",
        //        page_idx, crc, temp_page.raw[0], temp_page.raw[1]);

        // Если CRC равен 36, значит страница РЕАЛЬНО пустая на физическом чипе
        // НЕ ВЕРЬТЕ ЭТОМУ! Лучше проверить каждый байт!
        if (W25_DataIsEqu (temp_page.raw, W25Q_PAGE_SIZE, 0xFF)) {

            if (founded_pages == 0) {
                area_begin = page_idx;
            }

            founded_pages++;

            if (founded_pages == length) {
                return area_begin;
            }
        } else {
            founded_pages = 0;
        }
    }

    return 0;  // На всем чипе не нашлось непрерывного свободного места такого размера
}
// ----------------------------------------------------------------------------
// Получение первой страницы, выделенной под непрерывную область
uint32_t W25_GetContAreaBeginPage() {

    return flash_hw->nonx_area_begin;
}
// ----------------------------------------------------------------------------
// Запись страницы в непрерывную область
uint8_t W25_WriteContPage(uint32_t page_idx, uint8_t *buffer, uint16_t length) {

    if (page_idx < flash_hw->totalx_pages) { return 2; }

    if (page_idx < flash_hw->chip_size) {

        uint32_t addr = page_idx * W25Q_PAGE_SIZE;
        W25_WritePage(addr, buffer, length);

        return 0;
    }

    // Ошибка
    return 1;
}
// ----------------------------------------------------------------------------
// Чтение файла из непрерывной области по его ID
uint32_t W25_ReadContFileByIDEx (uint16_t file_id, flash_callback_data_read OnDataRead) {

    if (flash_hw->nonx_area_begin == 0) { return 1; }

    // Внимание! ID должен принадлежать существующему файлу протокола X,
    // который содержит внутри себя структуру fileContRecord_t
    uint32_t page_idx_tmp = 0;

    //printf ("W25_ReadContFileByIDEx\r\n");
    sectorPage_t *tmp_sec_data = W25_ReadFileSegment (file_id, 0, &page_idx_tmp);

    if (!tmp_sec_data) {

        //printf ("W25_ReadContFileByIDEx IS NULL\r\n");
        return 2; 
    }

    //W25_PrintPageDump(0, tmp_sec_data->raw, 256);

    fileRecord_t fileInfo = {0};

    // Копируем ровно столько байт, сколько нужно структуре (12 байт)
    memcpy (fileInfo.raw, tmp_sec_data->data, sizeof (fileInfo));

    //printf ("id: %u\r\n", (unsigned int)fileInfo.data.ID);

    if (fileInfo.data.ID != file_id) { return 3; }

    //printf ("address: %u\r\n", (unsigned int)fileInfo.data.Address);

    if (fileInfo.data.Address < flash_hw->totalx_pages) { return 4; }
    if (fileInfo.data.Address >= flash_hw->chip_size) { return 5; }

    //printf ("DEBUG: Address = %u, Blocks = %u\r\n", (unsigned int)fileInfo.data.Address, fileInfo.data.Blocks);
    W25_ReadContArea (fileInfo.data.Address, fileInfo.data.Blocks, fileInfo.data.Size, OnDataRead);

    return 0;
}
// ----------------------------------------------------------------------------
// Полная очистка непрерывной области данных одной командой
uint8_t W25_ClearContArea (void) {

    if (flash_hw->nonx_area_begin == 0) { return 1; }

    // 1. Вычисляем физический номер стартового сектора (10912 / 16 = 682)
    uint32_t start_sector_idx = flash_hw->nonx_area_begin / 16;

    // 2. Цикл идет строго по порядковым номерам секторов до конца чипа (до 1024)
    for (uint32_t sector_num = start_sector_idx; sector_num < flash_hw->chip_size; sector_num++) {

        // Превращаем номер сектора в байтовый адрес для вызова вашей функции
        // (номер сектора * 4096 байт)
        uint32_t sector_addr = sector_num * W25Q_SECTOR_SIZE;

        // Вызываем штатную функцию стирания
        W25_EraseSector4K (sector_addr);
    }

    return 0;
}
// ----------------------------------------------------------------------------
// -- Системные функции
// ----------------------------------------------------------------------------
void W25_Format (void) {
#ifdef USE_DEBUG_PRINT
    printf ("\r\n--- FULL FORMAT STARTED ---\r\n");
#endif
    for (uint16_t s = 0; s < flash_hw->total_4K_sectors; s++) {
        uint8_t sector_has_live_data = 0;
        uint32_t sector_addr = s * W25Q_SECTOR_SIZE;

        // Проверяем 16 страниц внутри сектора
        for (uint8_t p = 0; p < 16; p++) {
            uint8_t status;
            W25_ReadData (sector_addr + (p * W25Q_PAGE_SIZE), &status, 1);

            // Если это НАШ живой файл - сектор неприкосновенен
            if (status == SECTOR_STATUS_USABLE || status == SECTOR_STATUS_LAST_USABLE) {
                sector_has_live_data = 1;
                break;
            }
        }

        // Если в секторе только NEW (0xFF), TRASH (0xFC) или "чужой мусор" (0x20 и т.д.)
        if (!sector_has_live_data) {
            // Проверим, не пустой ли он уже (чтобы не тратить ресурс)
            uint8_t first_byte;
            W25_ReadData (sector_addr, &first_byte, 1);

            if (first_byte != SECTOR_STATUS_NEW) {
#ifdef USE_DEBUG_PRINT
                printf ("Cleaning junk at Sector %d [Addr: 0x%08X]...\r\n", s, (unsigned int)sector_addr);
#endif
                W25_EraseSector4K (sector_addr);
            }
        }
    }

    // Сбрасываем курсор поиска свободного места в начало
    last_free_page_idx = 0;
#ifdef USE_DEBUG_PRINT
    printf ("--- FORMAT COMPLETE ---\r\n");
#endif
}
// ----------------------------------------------------------------------------
#ifdef USE_DEBUG_PRINT
// ----------------------------------------------------------------------------
/**
 * @brief Выводит в UART карту занятости секторов 4кБ
 * Легенда:
 * '.' - Пустой (0xFF)
 * 'U' - Есть живые данные (USABLE/LAST)
 * 'T' - Только мусор (TRASH), пора тереть
 * 'M' - Смешанный (Живые + Мусор)
 */
void W25_PrintMemoryMap (void) {

    printf ("\r\n--- W25Q32 Physical Sector Map (1 char = 4KB) ---\r\n");

    for (uint16_t s = 0; s < flash_hw->total_4K_sectors; s++) {
        uint8_t live = 0, trash = 0, empty = 0;
        uint32_t start_addr = (uint32_t)s * W25Q_SECTOR_SIZE;

        // Сканируем статусы 16 страниц в секторе
        for (uint8_t p = 0; p < 16; p++) {
            uint8_t status;
            W25_ReadData (start_addr + (p * W25Q_PAGE_SIZE), &status, 1);

            if (status == SECTOR_STATUS_NEW)
                empty++;
            else if (status == SECTOR_STATUS_TRASH)
                trash++;
            else if (status == SECTOR_STATUS_USABLE || status == SECTOR_STATUS_LAST_USABLE)
                live++;
        }

        // Выбираем символ для сектора
        char c = '?';
        if (empty == 16)
            c = '.';  // Девственно чист
        else if (live > 0 && trash > 0)
            c = 'M';  // Mixed (нужно расселение)
        else if (live > 0)
            c = 'U';  // Только полезные данные
        else if (trash > 0)
            c = 'T';  // Сектор-зомби (только мусор)

        printf ("%c", c);

        // Перенос строки каждые 32 сектора (для красоты 32x32)
        if ((s + 1) % 32 == 0)
            printf (" | %d\r\n", s + 1);
    }
    printf ("--------------------------------------------------\r\n");
}
// ----------------------------------------------------------------------------
/**
 * @brief Печать буфера 256 байт в формате Hex Dump
 * @param start_addr - физический адрес во Flash (для красоты вывода)
 * @param buffer - указатель на данные (256 байт)
 */
void W25_PrintPageDump (uint32_t start_addr, uint8_t *buffer, uint16_t length) {

    if (length == 0 || length > 1024) { return; }

    printf ("\r\n--- PAGE DUMP [Addr: 0x%08X] (%u) ---\r\n", (unsigned int)start_addr, (unsigned int)length);
    printf ("        00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
    printf ("-------------------------------------------------------\r\n");

    for (uint16_t i = 0; i < length; i++) {
        Delay_Us (100);

        // Каждые 16 байт печатаем адрес начала строки
        if (i % 16 == 0) {
            printf ("0x%04X: ", i);
        }

        // Печатаем сам байт в HEX
        printf ("%02X ", buffer[i]);

        // В конце каждой строки (16 байт) делаем перенос
        if ((i + 1) % 16 == 0) {
            printf ("\r\n");
        }
    }
    printf ("-------------------------------------------------------\r\n");
}
// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------