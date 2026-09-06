/********************************** (C) COPYRIGHT *******************************
 * File Name          : w25qxx.h
 * Author             : vantr
 * Version            : V1.5.0
 * Date               : 2026/07/28
 * Description        : Драйвер флеш памяти Winbond W25Qxx (протокол X)
 * Info               : Включает обслуживание "непрерывной области"
 * Target             : CH32X033 
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// ----------------------------------------------------------------------------
/* Драйвер поддерживает посекторное чтение больших файлов. Определите в месте,
 * в котором будут обрабатываться данные такой метод:
 * void W25_OnFileRead(const uint32_t address, const uint8_t *data_chunk,
 * uint16_t len) { } Метод будет вызываться каждый раз, когда драйвер считал <=
 * 256 байт указанного файла. Инициировать начало последовательного чтения
 * нужнотак: W25_ReadFileByIDEx(your_file_id); Чтение происходит линейно от
 * первого до последнего сектора!
 */
// ----------------------------------------------------------------------------
#ifndef W25Q32_H
#define W25Q32_H
// ----------------------------------------------------------------------------
#include "app_config.h"
#include "SSS_Common_Lib_V1/sss_classes.h"
// ----------------------------------------------------------------------------
// Тип метода обратного вызова для чанковых, порционных чтений файлов
// Создайте в любом месте своего кода метод по этому прототипу и вызывайте
// чтение данных со ссылкой на свой метод, принимающий эти данные!
typedef uint8_t (flash_callback_data_read) (const uint32_t address,
                                            const uint16_t segment_num,
                                            const uint8_t *data_chunk,
                                            const uint16_t len);
// ----------------------------------------------------------------------------
// Этот параметр изменять нельзя!
#define W25Q_PAGE_MAX_DATA_SIZE     249     // Доступно для записи по протоколу в странице
// ----------------------------------------------------------------------------
// Размер буфера для файловых операций (в словах)
// От этого параметра зависит занимаемый объем RAM и производительность!
// Определяет количество сегментов (по странице флеш), которые могут быть
// найдены за один проход при последовательном чтении файлов, чем больше
// это число, тем быстрее будет работать ваша схема. Операции поиска очень
// длительные и тем дольше, чем больше объем памяти флеш чипа!
// При значении 512 обеспечивается чтение 63744 байтов без повторного поиска.
#ifndef W25Q_CHUNK_SIZE
#define W25Q_CHUNK_SIZE             512     // 1kB
#endif
// ----------------------------------------------------------------------------
// Параметры флеш-чипа
// ----------------------------------------------------------------------------
#define W25Q_PAGE_SIZE              256     // Размер страницы
// ----------------------------------------------------------------------------
// --- О параметре "nonx_area_begin" ------------------------------------------
// Этот параметр напрямую зависит от предыдущих настроек и указывает на
// начало области непрерывных данных, находящихся всегда после области
// X протокола и до конца массива памяти флеш.
// Каждый файл в непрерывной области имеет заголовок в виде протокольного
// файла, который содержит в себе структуру fileContRecord_t
// 
// Логически область делится на 4кБ сектора и её начало должно быть
// выровнено с учетом этого требования! Оставьте этот параметр равным 0,
// если данная возможность не используется.
// ----------------------------------------------------------------------------
// Эти параметры изменять не стоит, они логические и не зависят от чипа
#define W25Q_SECTOR_SIZE            4096    // 16 страниц
#define W25Q_BLOCK_SIZE             65536   // 16 секторов
// ----------------------------------------------------------------------------
// Команды W25Q32
#define W25_CMD_READ_DATA           0x03
#define W25_CMD_JEDEC_ID            0x9F
#define W25_CMD_WRITE_ENABLE        0x06
#define W25_CMD_PAGE_PROGRAM        0x02
#define W25_CMD_SECTOR_ERASE4K      0x20    // Стирает 4 КБ
#define W25_CMD_SECTOR_ERASE32K     0x52    // Стирает 32 КБ
#define W25_CMD_SECTOR_ERASE64K     0xD8    // Стирает 64 КБ
#define W25_CMD_ERASE_ALL           0xC7    // Стирает весь чип
#define W25_CMD_READ_STATUS_1       0x05
// ----------------------------------------------------------------------------
// Пустой идентификатор файла
#define W25_FILE_ZERO_ID            0xFFFF
#define W25_PAGE_ZERO_ID            0xFFFFFFFF
// ----------------------------------------------------------------------------
/* Для чипов до 128 Мбит (24-битная адресация) используются следующие команды:

    0x20 — Sector Erase (4 КБ). Самая частая команда.
    0x52 — Block Erase (32 КБ).
    0xD8 — Block Erase (64 КБ).
    0xC7 или 0x60 — Chip Erase (Стереть всё вообще).

    адрес всегда стоит выравнивать вручную по границе сектора (4 КБ)
    uint32_t sector_addr = address & 0xFFFFF000; // Обнуляем младшие 12 бит
    W25_EraseSector(sector_addr);
*/
// ----------------------------------------------------------------------------
#define W25_STATUS_BUSY             0x01
// ----------------------------------------------------------------------------
// Флаги статуса сектора (протокол X)
#define SECTOR_STATUS_NEW           0xFF    // Сектор не использовался
#define SECTOR_STATUS_USABLE        0xFE    // Сектор используется
#define SECTOR_STATUS_LAST_USABLE   0xFD    // Сектор используется (последний чанк)
#define SECTOR_STATUS_TRASH         0xFC    // Сектор брошен (не используется)
#define SECTOR_STATUS_BAD           0xF0    // Сектор с BAD блоками
// ----------------------------------------------------------------------------
#define LAST_ERROR_NO_ERROR         0x00    // Нет ошибок
#define LAST_ERROR_NOT_FOUND        0x01    // Не найден
// ----------------------------------------------------------------------------
// Структура заголовка сектора (256 байт) (протокол X)
#pragma pack(push, 1)
typedef union {
    uint8_t raw[256];  // Полный доступ к сырым байтам (индекс 0..255)

    struct {
        // --- ЗАГОЛОВОК (7 байт) ---
        uint8_t STATUS;  // 0xFE (Usable), 0xFD (Last), 0xFF (New)

        union {
            uint16_t ID;  // ИМЯ ФАЙЛА (2 байта)
            uint8_t ID_bytes[2];
        };

        union {
            uint16_t SEGMENT;  // НОМЕР СЕГМЕНТА (теперь до 65535!)
            uint8_t SEG_bytes[2];
        };

        uint8_t LENGTH;  // Длина полезных данных в этом секторе (0-249)
        uint8_t CRC8;    // Контрольная сумма заголовка

        // --- ДАННЫЕ (249 байт) ---
        uint8_t data[249];
    };
} sectorPage_t;
#pragma pack(pop)
// ----------------------------------------------------------------------------
// Структура информации о файле в непрерывной области
#pragma pack(push, 1)
typedef union {
    struct {
        uint16_t Type;
        uint16_t ID;
        uint32_t Address;
        uint16_t Blocks;
        uint32_t Size;
        uint8_t CRC;
    } data;  // В C99 имя обязательно

    uint8_t raw[15];
} __attribute__ ((packed)) fileRecord_t;
#pragma pack(pop)
// ----------------------------------------------------------------------------
// Структура системного файла с ID = 0
#pragma pack(push, 1)
typedef union {
    struct {
        uint32_t FlashID;
        uint32_t ContAreaBegin;
        uint32_t ContFreeBegin;
    } data;  // В C99 имя обязательно

    uint8_t raw[12];
} systemRecord_t;
#pragma pack(pop)
// ----------------------------------------------------------------------------
// Структура информации о массиве памяти
#pragma pack(push, 1)
typedef struct {
    uint32_t total_pages;
    uint32_t used_pages;
    uint32_t trash_pages;
    uint32_t bad_pages;
    uint32_t free_pages;
    uint32_t chip_size;
    uint32_t cont_area_begin;
    uint32_t cont_area_free;
} memoryInfo_t;
#pragma pack(pop)
// ----------------------------------------------------------------------------
typedef enum {
    FLASH_AREA_INVALID = 0,  // Неверный адрес (выход за пределы чипа)
    FLASH_AREA_X = 1,        // Страница находится в зоне X
    FLASH_AREA_NONX = 2      // Страница находится в зоне nonX
} flash_area_t;
// ----------------------------------------------------------------------------
// Тип метода обратного вызова для пошагового поиска файлов
// Создайте в любом месте своего кода метод по этому прототипу и вызывайте
// чтение данных со ссылкой на свой метод, принимающий эти данные!
typedef uint8_t (callback_fileinfo_found) (uint16_t file_id);
// ----------------------------------------------------------------------------
// Проверка, находятся ли два адреса в одном физическом секторе (4кБ)
#define W25Q_IS_SAME_SECTOR(addr1, addr2) (((addr1) ^ (addr2)) < W25Q_SECTOR_SIZE)
// ----------------------------------------------------------------------------
// Инициализация механизма работы с флеш-памятью
void W25_Init(void);
// ----------------------------------------------------------------------------
// Специальные методы
uint8_t W25_InitHardware (uint32_t chipID, uint8_t ignore_sysfile);
flash_area_t W25_CheckPageArea (uint32_t page_num);
uint8_t W25_GetLastError();
uint32_t W25_ReadID (void);
uint8_t W25_WaitBusy (void);
void W25_EraseSector4K (uint32_t addr);
void W25_Format (void);
void W25_WipeAll_Async (uint8_t wait);

uint32_t W25_GetChipSize();
uint32_t W25_GetProtoVolumeSize();

uint32_t W25_GetSectorAddr (uint32_t addr);
uint32_t W25_GetBlockAddr (uint32_t addr);
uint16_t W25_GetPageIndex (uint32_t addr);

uint8_t W25_DataIsEqu (uint8_t *buffer, uint16_t length, uint8_t value);
// ----------------------------------------------------------------------------
// Методы непрерывной области
uint8_t W25_ClearContArea (void);
uint32_t W25_FindContArea (uint16_t length);
uint32_t W25_GetContAreaBeginPage();
uint32_t W25_ReadContFileByIDEx (uint16_t file_id, flash_callback_data_read OnDataRead);
uint8_t W25_WriteContPage (uint32_t page_idx, uint8_t *buffer, uint16_t length);
void W25_ReadContArea (uint32_t first_page_idx, uint16_t pages_read_count, uint32_t length, flash_callback_data_read OnDataRead);
// ----------------------------------------------------------------------------
// Вне протокольные методы
void W25_ReadData (uint32_t addr, uint8_t *buffer, uint32_t length);
uint8_t W25_ReadPage (uint32_t page_idx, uint8_t *buffer, uint16_t length);
sectorPage_t *W25_ReadPageEx (uint32_t page_idx, uint16_t len);
void W25_WritePage (uint32_t addr, uint8_t *buffer, uint16_t length);
uint8_t W25_WritePage_Verified (uint32_t addr, uint8_t *buffer, uint16_t length);
// ----------------------------------------------------------------------------
// Протокольные методы
systemRecord_t *W25_ReadSystemFile();
uint8_t W25_WriteSystemFile (systemRecord_t *sys_data);
void W25_HW_ChangeContAreaBegin (uint32_t new_value);
uint16_t W25_FindMaxFileNUM (uint8_t file_type);

uint32_t W25_GetFreeAddress();
uint8_t W25_GetFreeAddresses (uint32_t *address_list, uint16_t required_count);
uint16_t W25_BuildFileMap_SinglePass(uint16_t target_id, uint32_t* out_map, uint16_t max_segs);
uint32_t W25_createFilesMap_SinglePassEx (uint32_t start_page_idx, fileRecord_t *out_map);
void W25_createFilesMap_SinglePass2 (callback_fileinfo_found OnFileFound);
uint16_t W25_createFilesMap_SinglePass (fileRecord_t *out_map, uint16_t max_segs);

uint16_t W25_BuildFileMapEx (const Params16 target_id, uint16_t *out_map);
uint16_t W25_BuildFileMapEx2 (const Params16 target_id, uint8_t *out_map);

uint8_t W25_GetFreeAddressesEx (uint32_t *address_list, uint16_t required_count, uint32_t excluded_sector_addr);
//uint8_t W25_GetFreeAddresses(uint32_t* address_list, uint16_t required_count);
uint8_t W25_getFilesProperty (fileRecord_t *out_fileprops);

uint8_t W25_FileExists (uint16_t file_id);
uint16_t W25_StoreBinData (uint8_t *bin_data, uint32_t total_size, uint16_t data_id);
uint32_t W25_ReadFileByID(uint16_t file_id, uint8_t* destination_buffer, uint32_t max_buffer_size);
uint32_t W25_ReadFileByIDEx (uint16_t file_id, flash_callback_data_read OnDataRead);
sectorPage_t *W25_ReadFileSegment (uint16_t fileID, uint16_t seg_idx, volatile uint32_t *page_idx);

uint8_t W25_EraseFileSegment (uint16_t fileID, uint16_t seg_idx);

void W25_SectorMarkAsTrash (uint32_t addr);
uint16_t W25_HideFile (uint16_t file_id);

uint8_t W25_StoreBinDataPart (uint8_t *bin_data, uint16_t length);

void W25Q_GetPhysicalBoundaries(uint16_t page_idx, uint16_t *start_page, uint16_t *end_page);
void W25Q_GetEraseBoundaries(uint16_t page_idx, uint16_t *first_page, uint16_t *last_page);
uint32_t GetPhysicalSectorAddr(uint16_t page_number);

uint8_t W25_GetUsedPagesInSector(uint16_t sector_idx, uint16_t *found_pages);
uint8_t W25_EvacuateAndErase(uint16_t sector_idx);
uint8_t W25_CollectGarbage(void);

sectorPage_t *W25_MemoryInfo (void);
uint8_t W25_MemoryInfo2 (memoryInfo_t *mem_result);

#ifdef USE_DEBUG_PRINT
void W25_PrintMemoryMap (void);
void W25_PrintPageDump (uint32_t start_addr, uint8_t *buffer, uint16_t length);
#endif
// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------
