/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : vantr
 * Version            : V1.3.0
 * Date               : 2026/07/28
 * Description        : Главная программа (UART)
 *********************************************************************************
 * (c) 2021 Vantr Universal Co. Ltd
 *******************************************************************************/
// ----------------------------------------------------------------------------
// Полный доступ к файлам с ББ (чтение/запись/удаление/показ картинок/проигрывание звука)...
// ----------------------------------------------------------------------------
#include "app_config.h"

#ifdef USE_TRANSPORT_UART
#include "SSS_UART_Lib_V1/uart_lib.h"

#include "SSS_XProto_Lib4/sss_xproto_lib1.h"
#include "SSS_XProto_Lib4/sss_xproto_targets.h"
#endif

#include "SSS_CRC8_Lib_V1/crc8_Lib_V1.h"

#include "SSS_W25Qxx_Lib_1/w25qxx.h"
#include "Classes/sss_flash_const.h"
#include "SSS_W25Qxx_Lib_1/hardware.h"

#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_Common_Lib_V1/sss_math.h"
// #include "SSS_Common_Lib_V1/sss_byteList.h"

#ifdef USE_TFT_ST7789
#include "SSS_ST7789_GrLib_V1/sss_st7789_grlib1.h"
#include "SSS_ST7789_GrLib_V1/matrix_engine.h"
#endif

#ifdef USE_OLED_SSD1306
#include "SSD1306_GR_SPI_V1/OLED_SPI_SSD1306.h"
#include "SSD1306_GR_SPI_V1/FONTS.h"
#include "SSD1306_GR_SPI_V1/oled_spi_matrix_engine.h"
#endif

#ifdef USE_I2C
#include "SSS_I2CHW_MLib_V4.4/i2c.h"
#endif

#ifdef USE_OLED_SSD1306_TXT_I2C
#include "SSS_I2CHW_MLib_V4.4/i2c.h"
#include "SSD1306_TXT_V5/SSD1306.h"
#endif

#ifdef USE_OLED_SSD1306_GR6_I2C
#include "SSD1306_GR_I2CHW_V6/OLED_SSD1306.h"
#include "SSD1306_GR_I2CHW_V6/oled_matrix_engine.h"
#endif

#ifdef USE_IR_NEC
#include "SSS_IRNEC_Lib.V1/sss_irnec_lib.h"
#include "SSS_IRNEC_Lib.V1/KEYES.h"
Int32x deb_val = {0};
#endif

#ifdef USE_SOUND_GEN
#include "SSS_SoundLib2/sss_soundLib.h"
#include "SSS_SoundLib2/test_ui_snd1.h"
#endif

#ifdef USE_LCD1602_I2C
#include "SSS_LCD1602_V2/LCD1602.h"
#endif

#ifdef USE_DS18B20
#include "SSS_DS18B20_Lib_V1/DS18B20.h"
#endif

#include "debug.h"
#include <string.h>
// ----------------------------------------------------------------------------
extern void view_oled_promo();
extern void view_promo();
// ----------------------------------------------------------------------------
// Максимальное количество обрабатываемых за один раз файловых записей
#define CONST_FILES_INFO_MAX_COUNT              18         // 18 * 14 = 256
#define CONST_FILES_MAP_MAX_COUNT               128         // 128 * 2 = 256
#define FI_STRUCT_SIZE sizeof (fileRecord_t)  // Размер структуры
// ----------------------------------------------------------------------------
// Глобальные переменные
#ifdef USE_TRANSPORT_UART
static uint8_t decoded_data[XPROTO_MAX_DATA_SIZE + 4];
ProtoHeader _header = {0};
ProtoTarget _target = {0};

static fileRecord_t file_info;
static uint16_t files_info_page_idx = 0;
static uint32_t address_map_ptr;
#endif
// ----------------------------------------------------------------------------
// Текущий номер страницы флеш
uint32_t _current_page = 0;
// ----------------------------------------------------------------------------
// Счетчик адреса непрерывной записи
uint32_t _current_page_writecont = 0;
// ----------------------------------------------------------------------------
// Отправка полный данных (свойств) указанного файла
#ifdef USE_TRANSPORT_UART
void flash_send_fileinfo (uint16_t file_id) {

    // printf ("prop_id:%u\r\n", (unsigned int)file_id);

    fileRecord_t f_info;

    f_info.data.ID = file_id;
    f_info.data.Blocks = 0;
    f_info.data.Size = 0;

    uint8_t pack_len = sizeof (fileRecord_t);

    uint8_t res_fnd = W25_getFilesProperty (&f_info);

    if (res_fnd == 0) {

        memset (decoded_data, 0, sizeof (decoded_data));
        memcpy (decoded_data, f_info.raw, pack_len);

        _target.PackIdent = XP_BINDATA_PACKET;
        xproto_createBINDataPacket (decoded_data, pack_len, FLASH_PACK_SINGLEFILEINFO, _target);
    } else {

        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_FILEINFO_FAILED);
    }
}

// ----------------------------------------------------------------------------
// Получение сегмента указанного файла по номеру
void flash_get_filesegment_byID (uint16_t fileID, uint16_t seg_idx) {

    // printf ("id:%u, seg_idx:%u\r\n", (unsigned int)fileID, (unsigned int)seg_idx);

    volatile uint32_t found_page_idx = 0;
    sectorPage_t *tmp_data = W25_ReadFileSegment (fileID, seg_idx, &found_page_idx);

    // printf ("found:%u\r\n", (unsigned int)found_page_idx);
    // W25_PrintPageDump (0, tmp_data->raw, 16);

    if (W25_GetLastError() == LAST_ERROR_NO_ERROR) {

        // printf("try send packet...\r\n");
        _target.PackIdent = XP_FLASHPAGE_READ_PACKET;
        xproto_createBINDataPacket (tmp_data->raw, 256, found_page_idx, _target);
    } else {

        // Больше данных нет!
        xproto_createStatusPacket (_header, fileID, FLASH_STATE_RDPAGE_FAILED);
        return;
    }
}

// ----------------------------------------------------------------------------
// Получение информации о занимаемой памяти
void flash_get_memoryInfo() {

    sectorPage_t *tmp_data = W25_MemoryInfo();

    if (tmp_data) {

        _target.PackIdent = XP_BINDATA_PACKET;
        xproto_createBINDataPacket (tmp_data->data, tmp_data->LENGTH, FLASH_PACK_MEMINFO, _target);

        if (xproto_getProtoLastState() != XPROTO_PROTO_NO_ERROR) {

            xproto_createStatusPacket (_header, FLASH_PACK_MEMINFO, FLASH_STATE_MEMINFO_FAILED);
        }
    }
}

// ----------------------------------------------------------------------------
// Поиск пустого места
void flash_find_freewindows_contarea (uint16_t ID, uint32_t total) {

    // printf ("id: %u, total: %u\r\n", (unsigned int)ID, (unsigned int)total);

    if (flash_hw->nonx_area_begin == 0) {

        xproto_createStatusPacket (_header, FLASH_PACK_FILECONTINFO, FLASH_STATE_CONTAREA_FULL);
        return;
    }

    uint32_t fnd_begin = W25_FindContArea (total);

    if (fnd_begin < flash_hw->nonx_area_begin) {

        xproto_createStatusPacket (_header, FLASH_PACK_FILECONTINFO, FLASH_STATE_CONTAREA_FULL);
        return;
    }

    fileRecord_t result = {0};

    result.data.Address = fnd_begin;
    result.data.Blocks = total;
    result.data.ID = ID;
    result.data.Type = 3;

    uint16_t length = sizeof (fileRecord_t);

    memcpy (&decoded_data, &result.raw, length);

    _target.PackIdent = XP_FLASHPAGE_READ_PACKET;
    xproto_createBINDataPacket (decoded_data, length, FLASH_PACK_FILECONTINFO, _target);
    
    if (xproto_getProtoLastState() != XPROTO_PROTO_NO_ERROR) {

        xproto_createStatusPacket (_header, FLASH_PACK_FILECONTINFO, FLASH_STATE_CONTAREA_FULL);
    }
}

// ----------------------------------------------------------------------------
// Установка адреса пустого пространства непрерывной области
void flash_set_new_freeaddr_contarea (uint32_t new_page_addr) {

    if (flash_hw->nonx_area_begin > 0 && new_page_addr >= flash_hw->nonx_area_begin) {

        systemRecord_t n_sys_rec = {0};
        
        n_sys_rec.data.FlashID = flash_hw->chip_id;
        n_sys_rec.data.ContFreeBegin = new_page_addr;
        n_sys_rec.data.ContAreaBegin = flash_hw->nonx_area_begin;

        if (W25_WriteSystemFile (&n_sys_rec) == 0) {
            // успешная запись файла
            xproto_createStatusPacket (_header, 0, FLASH_STATE_SET_FREECONTAREA_OK);
            return;
        }
    }

    xproto_createStatusPacket (_header, 0, FLASH_STATE_SET_FREECONTAREA_FAILED);
}

// ----------------------------------------------------------------------------
// Уборка мусора
void flash_go_gcc() {

    uint8_t result = W25_CollectGarbage();

    // 0 - успешное расселение одного сектора
    // 1 - сектор расселить не удалось (кончилось место?)
    // 2 - критическая ошибка
    // 3 - все хорошо, расселять некого
    xproto_createStatusPacket (_header, result, FLASH_STATE_GCC_COMPLETE);
}

// ----------------------------------------------------------------------------
// Отправка ББ считанной страницы памяти по адресу
void flash_send_page (uint16_t pagenum) {

    // printf("pagenum:%u", (unsigned int)pagenum);

    if (flash_hw == NULL || pagenum >= flash_hw->chip_size) {

        return;
    }

    if (W25_ReadPage (pagenum, decoded_data, 256) == 0) {

        // printf (" OK\r\n");
        _target.PackIdent = XP_FLASHPAGE_READ_PACKET;
        xproto_createBINDataPacket (decoded_data, 256, pagenum, _target);
    } else {

        // printf (" FAIL\r\n");
        //  Больше данных нет!
        xproto_createStatusPacket (_header, pagenum, FLASH_STATE_RDPAGE_FAILED);
        return;
    }
}

// ----------------------------------------------------------------------------
// Стирание указанного сектора
void flash_erase_sector (uint32_t sector_num) {

    // printf ("erasing sector: %u\r\n", (unsigned int)sector_num);

    if (sector_num < flash_hw->total_4K_sectors) {

        W25_EraseSector4K (sector_num);

        xproto_createStatusPacket (_header, FLASH_CMD_ERASE_SECTOR, 200);
    }

    // Иначе - статус 106!
    xproto_createStatusPacket (_header, FLASH_CMD_ERASE_SECTOR, 106);
}

// ----------------------------------------------------------------------------
// Получение карты тома (файлы)
void flash_create_fileList() {

    uint8_t total_founded = 0;
    uint16_t data_ptr = 0;

    while (total_founded < CONST_FILES_INFO_MAX_COUNT) {

        //  Сохраняем индекс найденного файла, как указатель для следующего поиска
        files_info_page_idx = W25_createFilesMap_SinglePassEx (files_info_page_idx, &file_info);

        // printf ("total_founded: %u\r\n", (unsigned int)total_founded);
        // printf ("files_info_page_idx: %u\r\n", (unsigned int)files_info_page_idx);

        if (files_info_page_idx < flash_hw->totalx_pages) {

            if (file_info.data.ID > 0) {

                // Копируем одну структуру
                memcpy (&decoded_data[data_ptr], &file_info, FI_STRUCT_SIZE);

                data_ptr += FI_STRUCT_SIZE;
                total_founded += 1;
            }
        } else {

            break;
        }
    }


    // Отправка пакета
    if (total_founded > 0) {

        // W25_PrintPageDump (0, decoded_data, total_founded * 8);

        _target.PackIdent = XP_BINDATA_PACKET;
        xproto_createBINDataPacket (decoded_data, total_founded * FI_STRUCT_SIZE, FLASH_PACK_FILEINFO, _target);

        if (xproto_getProtoLastState() != XPROTO_PROTO_NO_ERROR) {

            // Ошибка!
            files_info_page_idx = 0;
            xproto_createStatusPacket (_header, FLASH_PACK_FILEINFO, FLASH_STATE_FILEINFO_ERR);
        }
    } else {

        // Данных больше нет
        files_info_page_idx = 0;
        xproto_createStatusPacket (_header, FLASH_PACK_FILEINFO, FLASH_STATE_FILEINFO_DONE);
    }
}

// ----------------------------------------------------------------------------
// Создание карты указанного файла
void flash_create_filemap (uint16_t file_id) {

    // printf ("file_id: %u\r\n", (unsigned int)file_id);
    Params16 param_tmp;

    param_tmp.param1 = file_id;
    param_tmp.param2 = CONST_FILES_MAP_MAX_COUNT;
    param_tmp.param3 = address_map_ptr;
    param_tmp.param4 = 0;

    // printf ("start_idx: %u\r\n", (unsigned int)address_map_ptr);
    // printf ("param1: %u\r\n", (unsigned int)param_tmp.param1);
    // printf ("param2: %u\r\n", (unsigned int)param_tmp.param2);
    // printf ("param3: %u\r\n", (unsigned int)param_tmp.param3);
    // printf ("param4: %u\r\n", (unsigned int)param_tmp.param4);

    uint16_t founded_pages_cnt = W25_BuildFileMapEx2 (param_tmp, decoded_data);

    // W25_PrintPageDump(0, decoded_data, founded_pages_cnt * 2);

    // printf ("page_addr_total: %u\r\n", (unsigned int)founded_pages_cnt);

    if (founded_pages_cnt == 0) {

        // printf ("send_status_233...\r\n");
        address_map_ptr = 0;
        xproto_createStatusPacket (_header, FLASH_PACK_MAPDATA, FLASH_STATE_MAPDATA_DONE);

        return;
    } else {

        // printf ("send_pack_mapdata...\r\n");
        _target.PackIdent = XP_BINDATA_PACKET;
        xproto_createBINDataPacket (decoded_data, founded_pages_cnt * 2, FLASH_PACK_MAPDATA, _target);

        if (xproto_getProtoLastState() == XPROTO_PROTO_NO_ERROR) {

            address_map_ptr += founded_pages_cnt;

            return;
        }
    }

    // Закончено
    address_map_ptr = 0;
    xproto_createStatusPacket (_header, FLASH_PACK_MAPDATA, FLASH_STATE_MAPDATA_DONE);
}

// ----------------------------------------------------------------------------
// Обработка бинарных пакетов (с адресом)
void execute_bindata_pack (uint8_t *pack_data, uint16_t len, ProtoTarget target) {
    // ------------------------------------------------------------------------
    if (len < 2) {
        return;
    }
    // ------------------------------------------------------------------------
    // xproto_PrintData (pack_data, len);
    // ------------------------------------------------------------------------
    // Пересборка пакета для отладки
    packet_t *packet = xproto_decodeBINPacked (pack_data, len);

    uint8_t real_target = xproto_createTarget2 (target);
    ProtoHeader header = xproto_createProtoHeader (packet->full_buffer, len, real_target);
    // ------------------------------------------------------------------------
    // Сборка окончательного пакета
    xproto_createProtoPacket (packet->full_buffer, header);
    // ------------------------------------------------------------------------
    // Если нет ошибок, отправляем его обратно
    if (xproto_getProtoLastState() == XPROTO_PROTO_NO_ERROR) {

        xproto_sendTXBufferNow();

    } else {

        // Иначе - статус 120!
        xproto_createStatusPacket (header, XP_STATUS_PACKET, 120);
    }
    // -----------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
// Удаление файла по индексу
void flash_erase_file (uint16_t file_idx) {

    if (W25_HideFile (file_idx) == 0) {

        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_DELFILE_OK);
    } else {

        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_DELFILE_FAILED);
    }
}
#endif
// ----------------------------------------------------------------------------
// Рисование картинки по file_id
void flash_show_bitmap (uint16_t file_idx, uint8_t scale) {
#ifdef USE_TFT_ST7789
    if (file_idx == 0xFFFF) {

        view_promo();
        return;
    }

    ST7789_DisplayOff();  // Большие картинки (на весь экран) нужно прятать во время вывода
    TFT_matrix_clearScreen();
    ST7789_drawContIconFF (file_idx, 0, 0, scale);
    ST7789_DisplayOn();
#endif
#ifdef USE_OLED_SSD1306
    if (file_idx == 0xFFFF) {

        // Просмотр заставки!
        view_oled_promo();

        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_CMD_OK);
        return;
    }

    OLED_SPI_matrix_clearScreen();
    if (OLED_SPI_SSD1306_BASE_DrawProtoIcon(file_idx, 32, 0, scale)) {

        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_CMD_OK);
    } else {

        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_BAD_COMMAND);
    }
#endif
}

// ----------------------------------------------------------------------------
#ifdef USE_TRANSPORT_UART
// Выполнение входящих команд от внешнего приложения
void execute_incoming_cmd (cmd_packex_t cmd_pack) {

    // printf("cmd: %u\r\n", (unsigned int)cmd_pack.cmd);

    switch (cmd_pack.cmd) {

    // Установка текущей страницы памяти
    case FLASH_CMD_SET_CURPAGE:

        _current_page = cmd_pack.param;
        flash_send_page (_current_page);

        break;

    // Получение указанного сегмента указанного файла
    case FLASH_CMD_GET_FDBYID:

        flash_get_filesegment_byID (cmd_pack.param, cmd_pack.ext_param);
        break;

    // Проверка на существование файла с указанным ID
    // STATUS 237 - файл существует
    // STATUS 137 - файл не существует
    case FLASH_CMD_GET_FILEEXISTSID:

        if (W25_FileExists (cmd_pack.param) == 1) {

            xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_FILE_EXISTS);
        } else {

            xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_FILE_NOFOUND);
        }

        break;

    // Получение списка файлов флеш-памяти
    // Возвращает первые записи о файлах
    // STATUS 230 - список исчерпан
    case FLASH_CMD_GET_FILES:

        flash_create_fileList();

        break;

    // Возвращает остальные записи до STATUS 230
    case FLASH_CMD_GET_FILESINFO:

        flash_create_fileList();

        break;

    // Получение свойств указанного файла
    case FLASH_CMD_FILE_PROP:

        flash_send_fileinfo (cmd_pack.param);
        break;

    // Запрос на удаление указанного сегмента файла
    case FLASH_CMD_HIDE_FILESEGMENT:

        if (W25_EraseFileSegment (cmd_pack.param, cmd_pack.ext_param) == 1) {

            xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_HFS_OK);
        } else {

            xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_HFS_FAILED);
        }

        break;

    // Запрос на поиск свободного места в непрерывной области
    case FLASH_CMD_FINDCONTPLACE:

        //  File ID         нужно страниц
        flash_find_freewindows_contarea (cmd_pack.param, cmd_pack.ext_param);

        break;

    // Запрос на установку нового адреса начала пустого пространства непрерывной области
    case FLASH_CMD_SETNEW_FREECONTAREA:

        flash_set_new_freeaddr_contarea(cmd_pack.param);
        break;

#ifdef USE_SOUND_GEN
        // Установка громкости звука
        case FLASH_CMD_PLAY_VOLUME:

        Play_SetVolume (cmd_pack.param);
        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_CMD_OK);

        break;

    // Проиграть файл по его ID
    case FLASH_CMD_PLAY_SND:

        Play_SoundFromFlash (cmd_pack.param);
        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_PLAYSND_OK);

        break;

    case FLASH_CMD_PLAY_CONTSND:

        Play_SoundContFromFlash (cmd_pack.param);
        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_PLAYSND_OK);

        break;

    case FLASH_CMD_PLAY_TEST_SND:

        Play_SoundFromBuffer (SOUND_CLICK_DATA, sizeof (SOUND_CLICK_DATA));
        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_PLAYSND_OK);

        break;
#endif
    // Последовательный поиск адресов страниц указанного файла
    case FLASH_CMD_FINDFILEPAGES:

        flash_create_filemap (cmd_pack.param);

        break;

    // Запрос на установку счетчика адреса непрерывной записи файла
    case FLASH_CMD_SETWRITEADDR:

        if (flash_hw->nonx_area_begin > 0 && flash_hw->nonx_area_begin < flash_hw->chip_size && cmd_pack.param >= flash_hw->nonx_area_begin) {

            _current_page_writecont = cmd_pack.param;
            xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_SETWRITEADDR_OK);
        } else {

            xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_SETWRITEADDR_FAILED);
        }

        break;

    case FLASH_CMD_WIPE_CONTAREA:

        W25_ClearContArea();
        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_ERASE_CONTMEM_OK);

        break;

    // Удаление файла по ID
    case FLASH_CMD_ERASE_FILE:

        flash_erase_file (cmd_pack.param);

        break;

    // Стирание сектора
    case FLASH_CMD_ERASE_SECTOR:

        flash_erase_sector (cmd_pack.param);

        break;

    // Показать файл как картинку
    case FLASH_CMD_SHOW_BITMAP:

        //printf ("param:%u\r\n", (unsigned int)cmd_pack.param);
        flash_show_bitmap (cmd_pack.param, cmd_pack.ext_param);

        break;

    // Полное стирание флешки
    case FLASH_CMD_WIPE_ALL:

        W25_WipeAll_Async (1);
        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_WIPEALL_DONE);

        break;

    // Получение информации о занимаемой памяти
    case FLASH_CMD_GET_MEMINFO:

        flash_get_memoryInfo();
        break;

    case FLASH_CMD_GO_GCC:

        flash_go_gcc();
        break;

    case FLASH_CMD_SYS_REBOOT:

        if (cmd_pack.param == 0x01AD1B72) {

            xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_GO_REBOOT_NOW);
            Delay_Ms (1000);

            NVIC_SystemReset();
        }

        break;

    default:

        xproto_createStatusPacket (_header, XP_STATUS_PACKET, FLASH_STATE_BAD_COMMAND);

        break;
    }
}

// ----------------------------------------------------------------------------
// Обработка данных (не протокол)
// Функцию можно определить в любом внешнем модуле (C файле)
void execute_cmd_noproto() {
    // -------------------------------------------------------------------------
    // Команда "Get hardware name"
    if (xproto_Compare_Str ((uint8_t *)"AW?", 3)) {

        xproto_resetBufferData();
        xproto_sendStr ((uint8_t *)CONST_HARDWARE_NAME, sizeof (CONST_HARDWARE_NAME), 1);
        return;
    }
    // -------------------------------------------------------------------------
    // Команда "Get hardware version"
    if (xproto_Compare_Str ((uint8_t *)"AWHW?", 5)) {

        xproto_resetBufferData();
        xproto_sendStr ((uint8_t *)CONST_HARDWARE_VERSION, sizeof (CONST_HARDWARE_VERSION), 1);
        return;
    }
    // -------------------------------------------------------------------------
    // Команда "Get software version"
    if (xproto_Compare_Str ((uint8_t *)"AWSW?", 5)) {

        xproto_resetBufferData();
        xproto_sendStr ((uint8_t *)CONST_SOFTWARE_VERSION, sizeof (CONST_SOFTWARE_VERSION), 1);
        return;
    }
    // -------------------------------------------------------------------------
    // Команда "Get equipment" (список оборудования)
    if (xproto_Compare_Str ((uint8_t *)"AWEQ?", 5)) {

        uint8_t eq_tmp[10] = "AWEQ=?????";

#ifdef USE_FLASH_W25Q
        // Флеш память подключена
        eq_tmp[5] = 'F';

        switch (flash_hw->chip_id) {
            
            case HW_FLASH_W25Q32_ID:

                eq_tmp[6] = '1';
                break;

            case HW_FLASH_W25Q64_ID:

                eq_tmp[6] = '2';
                break;

            case HW_FLASH_W25Q128_ID:

                eq_tmp[6] = '3';
                break;

            default:

                eq_tmp[6] = '0';
                break;
        }
#endif

#ifdef USE_OLED_SSD1306
        // OLED дисплей SSD1306
        eq_tmp[7] = 'L';
#endif

#ifdef USE_TFT_ST7789
        // TFT дисплей ST7789
        eq_tmp[2] = 'T';
#endif

#ifdef USE_SOUND_GEN
        // Звук включен
        eq_tmp[8] = 'S';
#endif

#ifdef USE_SPI_HARDWARE
        // Используется аппаратный SPI
        eq_tmp[9] = 'H';
#else
        // Используется программный SPI
        eq_tmp[9] = 'P';
#endif

        xproto_resetBufferData();
        xproto_sendStr ((uint8_t *)eq_tmp, 10, 1);
        return;
    }
    // -------------------------------------------------------------------------
    // Команда "Get flash type"
    if (xproto_Compare_Str ((uint8_t *)"AWFT?", 5)) {

        xproto_resetBufferData();

        if (flash_hw == NULL) {

            xproto_sendStr ((uint8_t *)"FLHW_W25Q_UNKNOWN", 17, 1);
            return;
        }

        switch (flash_hw->chip_id) {

        case HW_FLASH_W25Q32_ID:

            xproto_sendStr ((uint8_t *)"FLHW_W25Q32", 11, 1);
            break;

        case HW_FLASH_W25Q64_ID:

            xproto_sendStr ((uint8_t *)"FLHW_W25Q64", 11, 1);
            break;

        case HW_FLASH_W25Q128_ID:

            xproto_sendStr ((uint8_t *)"FLHW_W25Q128", 12, 1);
            break;

        default:

            xproto_sendStr ((uint8_t *)"FLHW_W25QUNKNOWN", 17, 1);
            break;
        }

        return;
    }
    // -------------------------------------------------------------------------
    // Если принята неизвестная команда
    // -------------------------------------------------------------------------
    // Если нет совпадений по командам
    xproto_sendStr ((uint8_t *)"ERR:100", 7, 1);
    // -------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
// Выполнение команд X-Proto
// Функцию можно определить в любом внешнем модуле (C файле)
void execute_xproto_cmd() {
    // ------------------------------------------------------------------------
    _header = xproto_decodeProtoPacket (decoded_data);
    _target = xproto_decodeTarget (_header.target);

    // printf ("CRC Rx: 0x%02X | CRC Cal: 0x%02X\r\n", _header.crc8, cal_crc);

    if (xproto_getProtoLastState() != XPROTO_PROTO_NO_ERROR) {

        xproto_resetBufferData();
        xproto_sendStr ((uint8_t *)"ERRXP:101", 9, 1);

        return;
    }
    // ------------------------------------------------------------------------
    // Тестовый пакет
    // ------------------------------------------------------------------------
    if (_target.PackIdent == XP_DATA_TEST_PACKET) {

        // Тестовые пакеты не поддерживаются
        xproto_createStatusPacket (_header, XP_DATA_TEST_PACKET, 101);

        return;
    }
    // ------------------------------------------------------------------------
    // Пакет XP_CMD_PACKET
    // ------------------------------------------------------------------------
    if (_target.PackIdent == XP_CMD_PACKET) {

        cmd_packex_t received;

        // Распаковываем то, что пришло от ББ (Большого Брата / ПК)
        // Используем указатель на данные внутри входящего пакета протокола X
        xproto_decodeCMDPacket (decoded_data, _header.length.val, &received);

        execute_incoming_cmd (received);

        return;
    }
    // ------------------------------------------------------------------------
    // Пакет XP_BINDATA_PACKET
    // ------------------------------------------------------------------------
    if (_target.PackIdent == XP_BINDATA_PACKET) {

        // execute_bindata_pack (decoded_data, _header.length.val, _target);
        //  Чистые BINData пакеты не поддерживаются
        xproto_createStatusPacket (_header, XP_BINDATA_PACKET, 102);

        return;
    }
    // ------------------------------------------------------------------------
    // Пакет XP_BINDATA_READ_PACKET (запрос на чтение)
    // ------------------------------------------------------------------------
    if (_target.PackIdent == XP_BINDATA_READ_PACKET) {

        // Чистые BINData пакеты не поддерживаются
        xproto_createStatusPacket (_header, XP_BINDATA_READ_PACKET, 103);

        return;
    }
    // ------------------------------------------------------------------------
    // Пакет XP_BINDATA_WRITE_PACKET
    // ------------------------------------------------------------------------
    if (_target.PackIdent == XP_BINDATA_WRITE_PACKET) {

        packet_t *buffer = xproto_decodeBINPacked (decoded_data, _header.length.val);

        if (buffer) {

            // W25_PrintPageDump(buffer->header_u32, buffer->buffer);

            uint32_t real_addr = buffer->header_u32 * W25Q_PAGE_SIZE;

            if (W25_WritePage_Verified (real_addr, buffer->buffer, _header.length.val - 4) == 0) {
                // Запись успешно завершена
                xproto_createStatusPacket (_header, XP_BINDATA_WRITE_PACKET, 201);

                return;
            }
        }

        // Что-то пошло не так
        xproto_createStatusPacket (_header, XP_BINDATA_WRITE_PACKET, 104);

        return;
    }
    // ------------------------------------------------------------------------
    if (_target.PackIdent == XP_FLASHPAGE_WRITE_PACKET) {

        packet_t *buffer = xproto_decodeBINPacked (decoded_data, _header.length.val);

        // W25_PrintPageDump(0, buffer->buffer);
        // printf ("page_addr: %u\r\n", (unsigned int)buffer->header_u32);
        // printf ("page_leng: %u\r\n", (unsigned int)_header.length.val);

        // Внешнее приложение должно прислать полностью сформированную
        // страницу памяти по протоколу X! 256 байт, включая заголовок!

        // Запись страницы
        if (W25_StoreBinDataPart (buffer->buffer, _header.length.val - 4) == 0) {

            // Запись успешно завершена
            xproto_createStatusPacket (_header, XP_FLASHPAGE_WRITE_PACKET, FLASH_STATE_WRPAGE_OK);

            return;
        }

        // Что-то пошло не так
        xproto_createStatusPacket (_header, XP_FLASHPAGE_WRITE_PACKET, FLASH_STATE_WRPAGE_FAILED);

        return;
    }
    // ------------------------------------------------------------------------
    // Пакет записи в непрерывную область памяти
    if (_target.PackIdent == XP_FLASHPAGE_CONTWRITE_PACKET) {

        if (flash_hw->nonx_area_begin > 0 && flash_hw->nonx_area_begin < flash_hw->chip_size && _current_page_writecont >= flash_hw->nonx_area_begin) {

            // printf ("XP_FLASHPAGE_CONTWRITE_PACKET 2\r\n");
            //  Здесь можно производить запись
            packet_t *buffer = xproto_decodeBINPacked (decoded_data, _header.length.val);

            // W25_PrintPageDump (_current_page_writecont, buffer->buffer, _header.length.val);
            // printf ("page_addr: %u\r\n", (unsigned int)_current_page_writecont);
            // printf ("page_leng: %u\r\n", (unsigned int)_header.length.val);

            // Внешнее приложение должно прислать полностью сформированную
            // страницу памяти по протоколу X! 256 байт, включая заголовок!

            // Запись страницы
            if (W25_WriteContPage (_current_page_writecont, buffer->buffer, _header.length.val - 4) == 0) {

                // Инкремент адреса записи
                _current_page_writecont += 1;

                // Запись успешно завершена
                xproto_createStatusPacket (_header, XP_FLASHPAGE_CONTWRITE_PACKET, FLASH_STATE_WRITECONTADDR_OK);

                return;
            }

        } else {

            // Что-то пошло не так
            xproto_createStatusPacket (_header, XP_FLASHPAGE_CONTWRITE_PACKET, FLASH_STATE_WRITECONTADDR_FAILED);

            return;
        }
    }
    // ------------------------------------------------------------------------
    // Если ничего не сработало, значит - ошибка
    xproto_createStatusPacket (_header, 0, 100);
    // ------------------------------------------------------------------------
}
#endif
// ----------------------------------------------------------------------------
#ifdef USE_TFT_ST7789
void view_promo() {

    TFT_matrix_frame_init();

    ST7789_setForeColor (CL_WHITE);
    TFT_matrix_client_rect();

    Rect heap_area;

    heap_area.Location = (Point){5, 5};
    heap_area.ClientSize = (Size){ TFT_matrix_get_clientrect().ClientSize.Width - 10, 40 };

    ST7789_setForeColor (CL_BLUE);
    TFT_matrix_fillRect (heap_area, ForeGround);

    ST7789_setForeColor (CL_YELLOW);
    TFT_matrix_set_textContext_prop (TX_Space, 3);

    TFT_matrix_setCaret (10);
    TFT_matrix_setMargin (12);

    TFT_matrix_set_textContext_prop (TX_Scale, 3);
    TFT_matrix_writeString ((uint8_t *)"Тест CH32V003");

    ST7789_drawIconFF (0x5001, 10, 50, 2);
    ST7789_drawIconFF (0x5002, 80, 50, 2);
    ST7789_drawIconFF (0x5003, 150, 50, 2);
}
#endif
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
#if defined(USE_OLED_SSD1306)

#include "Fonts/new_7seg_21x37.h"
#include "Fonts/new_7seg_9x37.h"
#include "Fonts/misc32x16.h"

#include "Classes/radio_name.h"

void print_radio_freq (uint16_t value) {

    Params val_tmp = decompose_uint16 (value);

    if (val_tmp.sign > 0) {

        OLED_SPI_matrix_writeCharBin (0x20 + val_tmp.sign, &SEG7_MID_21X37, 1.0);
    } else {

        OLED_SPI_matrix_writeCharBin (0x2A, &SEG7_MID_21X37, 1.0);
    }

    OLED_SPI_matrix_writeCharBin (0x20 + val_tmp.extend, &SEG7_MID_21X37, 1.0);
    OLED_SPI_matrix_writeCharBin (0x20 + val_tmp.value2, &SEG7_MID_21X37, 1.0);
    OLED_SPI_matrix_writeCharBin (0x20, &SEG7_MID_9X37, 1.0);
    OLED_SPI_matrix_writeCharBin (0x20 + val_tmp.value1, &SEG7_MID_21X37, 1.0);
}

void view_oled_promo() {

    // Интерфейс FM приемника!

    uint16_t radio_freq = 1035;

    OLED_SPI_matrix_clearScreen();

    OLED_SPI_matrix_setCoord (0, 1);
    OLED_SPI_matrix_set_textContext_prop (TX_Space3, 0);
    OLED_SPI_matrix_set_textContext_prop (TX_Inverse3, 0);
    OLED_SPI_matrix_set_textContext_prop (TX_Scale3, 1);

    OLED_SPI_matrix_writeCharBin (0x27, &MiscF, 1.5);
    OLED_SPI_matrix_writeCharBin (0x2F, &MiscF, 1.5);

    OLED_SPI_matrix_writeCharBin (0x28, &MiscF, 1.5);

    OLED_SPI_matrix_writeCharBin (0x22, &MiscF, 1.5);

    OLED_SPI_matrix_setCoord (2, 18);
    print_radio_freq (radio_freq);

    OLED_SPI_matrix_writeCharBin (0x30, &MiscF, 1.5);  // MHz

    Rect radio_name_area = {
        {0,             DISPLAY_HEIGHT - 10},
        {DISPLAY_WIDTH, 10                 }
    };

    OLED_SPI_matrix_set_textContext_prop (TX_Space3, 2);
    OLED_SPI_matrix_drawStringInscribed_Smart ((uint8_t *)get_station_name (radio_freq), radio_name_area);

    OLED_SPI_matrix_refresh(); 
}
#endif
// ----------------------------------------------------------------------------
#ifdef USE_OLED_SSD1306_GR6_I2C
// ----------------------------------------------------------------------------
#include "Fonts/new_7seg_21x37.h"
#include "Fonts/new_7seg_9x37.h"
#include "Fonts/misc32x16.h"

#include "Classes/radio_name.h"

void print_radio_freq2 (uint16_t value) {

    Params val_tmp = decompose_uint16 (value);

    if (val_tmp.sign > 0) {

        oled_matrix_writeCharBin (0x20 + val_tmp.sign, &SEG7_MID_21X37, 1.0);
    } else {

        oled_matrix_writeCharBin (0x2A, &SEG7_MID_21X37, 1.0);
    }

    oled_matrix_writeCharBin (0x20 + val_tmp.extend, &SEG7_MID_21X37, 1.0);
    oled_matrix_writeCharBin (0x20 + val_tmp.value2, &SEG7_MID_21X37, 1.0);
    oled_matrix_writeCharBin (0x20, &SEG7_MID_9X37, 1.0);
    oled_matrix_writeCharBin (0x20 + val_tmp.value1, &SEG7_MID_21X37, 1.0);
}

void view_oled_promo2() {

    // Интерфейс FM приемника!

    uint16_t radio_freq = 1035;

    oled_matrix_clearScreen();

    oled_matrix_setCoord (0, 1);
    oled_matrix_set_textContext_prop (TX_Space2, 0);
    oled_matrix_set_textContext_prop (TX_Inverse2, 0);
    oled_matrix_set_textContext_prop (TX_Scale2, 1);

    oled_matrix_writeCharBin (0x27, &MiscF, 1.5);
    oled_matrix_writeCharBin (0x2F, &MiscF, 1.5);

    oled_matrix_writeCharBin (0x28, &MiscF, 1.5);

    oled_matrix_writeCharBin (0x22, &MiscF, 1.5);

    oled_matrix_setCoord (2, 18);
    print_radio_freq2 (radio_freq);

    oled_matrix_writeCharBin (0x30, &MiscF, 1.5);  // MHz

    Rect radio_name_area = {
        {0,             DISPLAY_HEIGHT - 10},
        {DISPLAY_WIDTH, 10                 }
    };

    oled_matrix_set_textContext_prop (TX_Space2, 2);
    oled_matrix_drawStringInscribed_Smart ((uint8_t *)get_station_name (radio_freq), radio_name_area);

    oled_matrix_refresh();
}
#endif
// ----------------------------------------------------------------------------

#ifdef USE_OLED_SSD1306_TXT_I2C
void OLED_TXT_Promo() {

    uint8_t led_str2[] = {0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE, 0xD0, 0xD2, 0x00};

    SSD1306_TXT_Init (0xE0);

    SSD1306_TXT_ClearScreen (1);
    //  -------------------------------------------------------------------------
    // i2c_scan();
    //  -------------------------------------------------------------------------
    SSD1306_TXT_SetWrapMode (NOWRAP4);

    for (int row_index = 0; row_index < 8; row_index++) {

        // Счетчик строк инверсный
        SSD1306_TXT_SetCursor (0, row_index);
        SSD1306_TXT_PutSymbol (0x30 + row_index, SYMSTYLE_NOALIGN | SYMSTYLE_INV);
    }

    // Wide LED строка
    SSD1306_TXT_SetCursor (2, 4);
    SSD1306_TXT_WriteStr ((uint8_t *)led_str2, SYMSTYLE_WIDTHLED | SYMSTYLE_NOCYR);

    // Широкие символы
    SSD1306_TXT_SetCursor (2, 0);
    SSD1306_TXT_WriteStr ((uint8_t *)"МЕГА", SYMSTYLE_WIDE);

    SSD1306_TXT_SetCursor (2, 2);
    SSD1306_TXT_SetStr_Limit (9);
    SSD1306_TXT_WriteStr ((uint8_t *)led_str2, SYMSTYLE_LED | SYMSTYLE_NOCYR);

    // Обычные символы
    SSD1306_TXT_SetCursor (11, 1);
    SSD1306_TXT_WriteStr ((uint8_t *)"Вверх", SYMSTYLE_NORM);

    // Обычные символы
    SSD1306_TXT_SetCursor (11, 2);
    SSD1306_TXT_WriteStr ((uint8_t *)"Вниз-", SYMSTYLE_NORM);
}
#endif

// ----------------------------------------------------------------------------
#if defined (USE_IR_NEC) && defined (USE_OLED_SSD1306_TXT_I2C)

Params my_int_to_hex (uint8_t value) {

    Params result;

    result.value1 = HEX_STR[(value >> 4) & 0x0F];
    result.value2 = HEX_STR[value & 0x0F];

    return result;
}

void print_hex_value (uint8_t value) {

    Params tmp_data = my_int_to_hex (value);
    SSD1306_TXT_PutSymbol (tmp_data.value1, 0);
    SSD1306_TXT_PutSymbol (tmp_data.value2, 0);
}

void IR_processing() {

    SSD1306_TXT_SetCursor (0, 4);

    switch (IR_GetStatus()) {

    case IRDR_READY:

        deb_val.cvalue[0] = IR_GetData (0);
        deb_val.cvalue[1] = IR_GetData (1);
        deb_val.cvalue[2] = IR_GetData (2);
        deb_val.cvalue[3] = IR_GetData (3);
        SSD1306_TXT_WriteStr ((uint8_t *)"NORMAL", SYMSTYLE_NORM);

        break;

    case IRDR_REPEAT:

        deb_val.cvalue[0] = IR_GetLastData (0);
        deb_val.cvalue[1] = IR_GetLastData (1);
        deb_val.cvalue[2] = IR_GetLastData (2);
        deb_val.cvalue[3] = IR_GetLastData (3);
        SSD1306_TXT_WriteStr ((uint8_t *)"REPEAT", SYMSTYLE_NORM);

        break;

    default:

        deb_val.cvalue[0] = 0;
        deb_val.cvalue[1] = 0;
        deb_val.cvalue[2] = 0;
        deb_val.cvalue[3] = 0;
        SSD1306_TXT_WriteStr ((uint8_t *)"<NONE>", SYMSTYLE_NORM);

        break;
    }

    SSD1306_TXT_SetCursor (0, 0);
    print_hex_value (deb_val.cvalue[3]);
    SSD1306_TXT_SetCursor (3, 0);
    print_hex_value (deb_val.cvalue[2]);
    SSD1306_TXT_SetCursor (6, 0);
    print_hex_value (deb_val.cvalue[1]);
    SSD1306_TXT_SetCursor (9, 0);
    print_hex_value (deb_val.cvalue[0]);

#ifdef USE_SOUND_GEN
    switch (deb_val.cvalue[3]) {

    case KEY_1:

        Play_SoundContFromFlash (0x3001);
        break;

    case KEY_2:

        Play_SoundContFromFlash (0x3002);
        break;

    case KEY_3:

        Play_SoundContFromFlash (0x3003);
        break;

    case KEY_4:

        Play_SoundContFromFlash (0x3004);
        break;

    case KEY_5:

        Play_SoundContFromFlash (0x3005);
        break;

    case KEY_7:

        flash_show_bitmap(0x1001, 1);
        break;

    case KEY_8:

        flash_show_bitmap(0x1002, 1);
        break;

    case KEY_9:

        flash_show_bitmap (0x1003, 1);
        break;

    case KEY_0:

        flash_show_bitmap (0x1004, 1);
        break;

    case KEY_OK:

        flash_show_bitmap(0xFFFF, 1);
        break;
    }

    IR_ResetStatus();
#endif

}
#endif
// ----------------------------------------------------------------------------

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main (void) {
    //-------------------------------------------------------------------------
    NVIC_PriorityGroupConfig (NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    //-------------------------------------------------------------------------
#ifdef USE_W25Q_DEBUG
    USART_Printf_Init (UART_WORK_BAUDRATE);
    printf ("SystemClk:%u\r\n", (unsigned int)SystemCoreClock);
    printf ("ChipID:%08x\r\n", (unsigned int)DBGMCU_GetCHIPID());
#endif
    //-------------------------------------------------------------------------
    Delay_Ms (500);
    //-------------------------------------------------------------------------
#ifdef USE_FLASH_W25Q
    // Инициализация механизма SPI/Flash
    W25_Init();
    uint32_t _FlashID = W25_ReadID();
    //-------------------------------------------------------------------------
#ifdef USE_W25Q_DEBUG
    printf ("FlashID:%08x\r\n", (unsigned int)_FlashID);
#endif
    //-------------------------------------------------------------------------
#ifdef USE_FLASH_W25Q
    if (W25_InitHardware (_FlashID, 1) == 0) {
        // Блокирование работы программы
#ifdef USE_W25Q_DEBUG
        printf ("ERROR: Unknown FlashID:%08x\r\n", (unsigned int)_FlashID);
        printf ("Stopped...");
#endif
        // Фатальная ошибка - нет флеш памяти!
        while (1) { Delay_Ms (1000); }
    }
#endif
#endif
    // ------------------------------------------------------------------------
#ifdef USE_W25Q_DEBUG
    if (flash_hw->nonx_area_begin > 0) {
        
        printf ("Cont Area Begin: %u\r\n", (unsigned int)flash_hw->nonx_area_begin);
    }
#endif
    // ------------------------------------------------------------------------
#ifdef USE_TFT_ST7789
    // Инициализация TFT дисплея
    ST7789_SPI_Init();
    ST7789_Init_Landscape();
    // Вывод тестовой графики
    view_promo();
    // ------------------------------------------------------------------------
#endif
    //-------------------------------------------------------------------------
#ifdef USE_TRANSPORT_UART
    // UART
    UART_UNIT_Init (UART_WORK_BAUDRATE);
    //-------------------------------------------------------------------------
#ifdef USE_I2C
    // I2C инициализация. Периферия инициализировать шину не должна!
    IIC_Init (I2C_SELF_ADDRESS);
#endif
    //-------------------------------------------------------------------------
#endif
#ifdef USE_W25Q_DEBUG
    printf ("Running (v_CH32X033 Programmer)...\r\n");
#endif
    //-------------------------------------------------------------------------
#ifdef USE_OLED_SSD1306_TXT_I2C
    OLED_TXT_Promo();
#endif
    //-------------------------------------------------------------------------
#ifdef USE_OLED_SSD1306
    // Для SPI устройств инициализация обязательна!
    OLED_SPI_SSD1306_BASE_Init(0xCF);
    view_oled_promo();
#endif
    //-------------------------------------------------------------------------
#ifdef USE_I2C
    // Инициализация I2C ДО первого обращения к шине!
    IIC_Init (I2C_SELF_ADDRESS);
#endif
    //-------------------------------------------------------------------------
#ifdef USE_OLED_SSD1306_GR6_I2C
        OLED_SSD1306_BASE_Init (0xCF);
    view_oled_promo2();
#endif
    //-------------------------------------------------------------------------
#ifdef USE_IR_NEC
    IR_Init();

    uint8_t tick_counter = 0;
    SSD1306_TXT_ClearScreen (1);
#endif
    //-------------------------------------------------------------------------
#ifdef USE_SOUND_GEN
    Play_SoundInit();
#endif
    //-------------------------------------------------------------------------
#ifdef USE_LCD1602_I2C
    lcd_init(0x07);
    lcd_writeStr((char *)"This is a test!");
    lcd_writeStrPos(1, 0, (char *)"0123456789 ->");
    lcd_blcontrol(0);
    Delay_Ms(1000);
    lcd_blcontrol(1);
#endif
    //-------------------------------------------------------------------------
#ifdef USE_DS18B20
    DS18B20_Init();
#endif
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    while (1) {
#ifdef USE_IR_NEC
    tick_counter += 1;

    if (tick_counter > 10) {
        
        tick_counter = 0;
        // Опрос лучше всего производить раз в секунду
        IR_processing();
    }
#endif
#ifdef USE_TRANSPORT_UART
    // Выполняем обработку очереди сообщений
    xproto_processingCommand();
#endif
    // Обязательная задержка!
    Delay_Ms (100);
    }
    //-------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
