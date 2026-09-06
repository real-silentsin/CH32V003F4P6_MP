/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/07/14
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
// ----------------------------------------------------------------------------
/* Демка без связи с компьютером - навигация по файлам флеш-памяти
 * • Демонстрирует изображения
 * • Проигрывает звуковые фрагменты
 * • Позволяет удалять файлы
 * • Управление через многослойное меню
 * • Навигация по меню энкодером EC11
 */
// ----------------------------------------------------------------------------
#include "app_config.h"
#include "Classes/sss_filetypes.h"
#include "SSS_Common_Lib_V1/sss_stringList.h"

#ifdef USE_TRANSPORT_UART
#include "SSS_UART_Lib_V1/uart_lib.h"
#include "SSS_XProto_Lib3/sss_xproto_lib1.h"
#include "SSS_XProto_Lib3/sss_xproto_targets.h"
#endif

#ifdef USE_FLASH_W25Q
#include "SSS_W25Qxx_Lib_1/w25qxx.h"
#include "SSS_W25Qxx_Lib_1/hardware.h"
#ifdef USE_TRANSPORT_UART
#include "sss_flash_const.h"
#endif
#endif

#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_Common_Lib_V1/sss_math.h"
#include "SSS_Common_Lib_V1/sss_byteList.h"

#ifdef USE_SOUND_GEN
#include "SSS_SoundLib2/sss_soundLib.h"
#include "SSS_SoundLib2/test_ui_snd1.h"
#endif

#ifdef USE_TFT_ST7789
#include "SSS_ST7789_GrLib_V1/sss_st7789_grlib1.h"
#include "SSS_ST7789_GrLib_V1/matrix_engine.h"
#include "SSS_ST7789_GrLib_V1/matrix_menu.h"
#endif

#ifdef USE_OLED_SSD1306
#include "SSS_SSD1306_TXT_V5/SSD1306.h"
#include "SSS_SSD1306_TXT_V5/FONTS.h"
#endif

#ifdef USE_ENCODER_EC11
#include "SSS_EC11Lib_V1/EC11Lib.h"
#endif

#include "debug.h"
#include <string.h>
// ----------------------------------------------------------------------------
extern void view_oled_promo();
extern void view_promo();
// ----------------------------------------------------------------------------
// Проект оптимизирован -Os, -flto, GCC15
// ----------------------------------------------------------------------------
// Версии компонентов оборудования
#ifdef USE_TRANSPORT_UART
#define CONST_HARDWARE_NAME "W25-UART-V003_1"
#define CONST_HARDWARE_VERSION "H1.3"
#define CONST_SOFTWARE_VERSION "S1.5"
#endif
// ----------------------------------------------------------------------------
// Количество тактов чтобы считать нажатие кнопки энкодера долгим
#define ENC_BUT_LONG_VALUE          10
// ----------------------------------------------------------------------------
// Системные команды
#define CMD_FILE_NONE MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 0)         // Пропуск (не поддерживается)
#define CMD_FILE_BACK MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 1)         // Назад (Упакуется в 0xF001)
#define CMD_FILE_SHOW MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 2)         // Показать картинку
#define CMD_FILE_PLAY MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 3)         // Воспроизвести звук
#define CMD_FILE_DELETE MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 4)       // Удалить файл

#define CMD_OPTIONS_VOLUME MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 5)    // Настройка громкости
#define CMD_SETVOL_0 MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 6)          // Громкость 0%
#define CMD_SETVOL_25 MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 7)         // Громкость 25%
#define CMD_SETVOL_50 MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 8)         // Громкость 50%
#define CMD_SETVOL_75 MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 9)         // Громкость 75%
#define CMD_SETVOL_100 MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 10)       // Громкость 75%

#define CMD_FILE_CREATE MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 11)      // Создать файл

#define CMD_MEM_INFO MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 12)         // Информация о памяти
#define CMD_GO_GCC MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 20)           // Уборка мусора

#define CMD_COMMON_OPTIONS MAKE_FILE_ID (FILE_TYPE_SYS_ACTION, 100)  // Настройки
// ----------------------------------------------------------------------------
// Буфер под строки пунктов меню
static uint8_t menu_labels[MENU_MAX_ITEMS][5];
// ----------------------------------------------------------------------------
// Флаг остановки
static uint8_t is_stop_screen = 0;
// ----------------------------------------------------------------------------
// Список строк для динамических меню
static DynamicStringList my_log;
//-----------------------------------------------------------------------------
// Озвучивание интерфейса (БИП)
void do_action_snd (void) {

#ifdef USE_SOUND_GEN
    Play_SoundFromBuffer (SOUND_CLICK_DATA, sizeof (SOUND_CLICK_DATA));
#endif
}
// ----------------------------------------------------------------------------
// Метод обратного вызова при поиске файлов!
uint8_t find_files_callback (uint16_t file_id) {

    TFT_Menu *menu = TFT_matrix_menu_mainmenu();

    if (menu->MenuID == 0x100) {

        // Защита от переполнения
        if (menu->Count >= MENU_MAX_ITEMS) {
            return 0;
        }

        // Получаем индекс текущего свободного места в меню (0...15)
        uint8_t idx = menu->Count;

        // 1. Очищаем целевую строчку нулями (гарантируем '\0' в конце)
        reset_buffer (menu_labels[idx], 0, 5);

        // 2. Ваша функция пишет 4 символа HEX прямо в ячейку menu_labels[idx]
        fill_buffer_hex16 (file_id, menu_labels[idx]);

        // 3. Передаем указатель на эту строку и упакованный file_id
        TFT_matrix_menu_create_menuitem (menu_labels[idx], file_id);

        return 1;  // Продолжаем поиск файлов на W25
    }

    return 0;
}
// ----------------------------------------------------------------------------
// Получение карты тома (файлы) последовательное
void flash_create_fileList() {

    W25_createFilesMap_SinglePass2 (find_files_callback);
}
// ----------------------------------------------------------------------------
void flash_erase_file (uint16_t file_idx) {

    if (W25_HideFile (file_idx) == 0) {

        TFT_matrix_menu_show_modal_windows ((uint8_t *)"Файл удалён", CL_YELLOW, CL_GREEN);
    } else {

        TFT_matrix_menu_show_modal_windows ((uint8_t *)"ОШИБКА удаления!", CL_YELLOW, CL_RED);
    }
}
// ----------------------------------------------------------------------------
// Уборка мусора
void flash_go_gcc() {

    (void)W25_CollectGarbage();

    // 0 - успешное расселение одного сектора
    // 1 - сектор расселить не удалось (кончилось место?)
    // 2 - критическая ошибка
    // 3 - все хорошо, расселять некого
    // xproto_createStatusPacket (_header, result, FLASH_STATE_GCC_COMPLETE);
}
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

        view_oled_promo();
        return;
    }

    SSD1306_BASE_ClearScreen (1);
    SSD1306_BASE_SetCursor (6, 1);
    SSD1306_BASE_WriteStr ((uint8_t *)"NO", SSD1306_SYMSTYLE_WIDE | SSD1306_SYMSTYLE_INV);
    SSD1306_BASE_SetCursor (0, 4);
    SSD1306_BASE_WriteStr ((uint8_t *)"Graphics", SSD1306_SYMSTYLE_WIDE);
#endif
}
// ----------------------------------------------------------------------------
#ifdef USE_TFT_ST7789
// Графическая демка для TFT экранов
void view_promo() {

    TFT_matrix_frame_init();

    ST7789_setForeColor (CL_WHITE);
    TFT_matrix_client_rect();

    Rect heap_area;

    heap_area.Location = (Point){5, 5};
    heap_area.ClientSize = (Size){TFT_matrix_get_clientrect().ClientSize.Width - 10, 40};

    ST7789_setForeColor (CL_BLUE);
    TFT_matrix_fillRect (heap_area, ForeGround);

    ST7789_setForeColor (CL_YELLOW);
    TFT_matrix_set_textContext_prop (TX_Space, 3);

    TFT_matrix_setCaret (10);
    TFT_matrix_setMargin (12);

    TFT_matrix_writeString ((uint8_t *)"Тест CH32V003");

    ST7789_drawIconFF (1, 10, 50, 2);
    ST7789_drawIconFF (2, 80, 50, 2);
    ST7789_drawIconFF (3, 150, 50, 2);

    // Рисуется примерно 1 секунду
    // ST7789_drawIconFF (0x00CD, 0, 0, 1);
}
#endif
// ----------------------------------------------------------------------------
#ifdef USE_OLED_SSD1306

uint8_t led_str2[] = {0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE, 0xD0, 0xD2, 0x00};

void view_oled_promo() {
    // SSD1306_BASE_OFF();
    SSD1306_BASE_ClearScreen (1);

    SSD1306_BASE_SetWrapMode (NOWRAP);

    for (int row_index = 0; row_index < 8; row_index++) {

        // Счетчик строк инверсный
        SSD1306_BASE_SetCursor (0, row_index);
        SSD1306_BASE_PutSymbol (0x30 + row_index, SSD1306_SYMSTYLE_NOALIGN | SSD1306_SYMSTYLE_INV);
    }

    // Wide LED строка
    SSD1306_BASE_SetCursor (2, 4);
    SSD1306_BASE_WriteStr ((uint8_t *)led_str2, SSD1306_SYMSTYLE_WIDTHLED | SSD1306_SYMSTYLE_NOCYR);

    // Широкие символы
    SSD1306_BASE_SetCursor (2, 0);
    SSD1306_BASE_WriteStr ((uint8_t *)"МЕГА", SSD1306_SYMSTYLE_WIDE);

    SSD1306_BASE_SetCursor (2, 2);
    SSD1306_SetStr_Limit (9);
    SSD1306_BASE_WriteStr ((uint8_t *)led_str2, SSD1306_SYMSTYLE_LED | SSD1306_SYMSTYLE_NOCYR);

    // Обычные символы
    SSD1306_BASE_SetCursor (11, 1);
    SSD1306_BASE_WriteStr ((uint8_t *)"Вверх", SSD1306_SYMSTYLE_NORM);

    // Обычные символы
    SSD1306_BASE_SetCursor (11, 2);
    SSD1306_BASE_WriteStr ((uint8_t *)"Вниз-", SSD1306_SYMSTYLE_NORM);

    // SSD1306_BASE_ON();
}
#endif
// ----------------------------------------------------------------------------
// Меню верхнего уровня - главное (Список файлов)
void start_mainmenu(uint8_t select_zero) {
    //  -------------------------------------------------------------------------
    // Инициализация нового меню
    // Это меню - главное!
    if (TFT_matrix_menu_create_new_root_menu (select_zero) != 1) {
        return;
    }
    TFT_matrix_menu_default_config (TFT_matrix_get_clientrect(), 40);
    //  -------------------------------------------------------------------------
    // Локальная копия указателя
    TFT_Menu *menu = TFT_matrix_menu_mainmenu();

    menu->MenuID = 0x100;
    menu->Tag = 0;
    menu->DefaultActionIndex = 8;
    menu->ViewItemNumber = 1;
    menu->ItemNumberWidth = 30;
    menu->MenuItemHeight = 40;
    menu->TextScale = 2;
    menu->MenuHeapWidth = 30;
    menu->TextSpace = 3;
    menu->TextMenuSpace = 5;

    // Наполнение элементами
    TFT_matrix_menu_create_menuitem2 ((uint8_t *)"[Настройки]", CMD_COMMON_OPTIONS, CL_GREEN);
    flash_create_fileList();

    TFT_matrix_menu_set_name ((uint8_t *)"ФАЙЛЫ");
    TFT_matrix_menuname_set_margin (0);

    // Обязательно! Завершение создания этого меню
    TFT_matrix_menu_create_menu_complete();
}
// ----------------------------------------------------------------------------
// Меню второго уровня - действия с файлами
void start_menu2 (uint16_t file_id) {
    // -------------------------------------------------------------------------
    // Обязательная часть обработки!
    // -------------------------------------------------------------------------
    // Здесь еще доступны все данные вызвавшего нас окна!
    //  -------------------------------------------------------------------------
    // Закрытие предыдущего меню
    TFT_matrix_menu_close_menu();

    // Инициализация нового меню
    if (TFT_matrix_menu_create_new_menu() != 1) {
        return;
    }
    TFT_matrix_menu_default_config (TFT_matrix_get_clientrect(), 40);
    //  -------------------------------------------------------------------------
    // Конец обязательной части
    //  -------------------------------------------------------------------------
    // Локальная копия указателя (ссылка)
    TFT_Menu *menu = TFT_matrix_menu_mainmenu();

    // menu->MenuItemHeight = (menu->MenuRect.ClientSize.Height / 2);

    // Теперь работаем быстро и коротко
    menu->MenuID = 0x200;
    menu->Tag = file_id;

    // Запоминаем индекс вызвавшего нас пункта меню
    // menu->BackRetIndex = 0;

    menu->DefaultActionIndex = 0;
    menu->TextScale = 2;
    menu->MenuHeapWidth = 30;

    menu->TextSpace = 3;
    menu->TextMenuSpace = 5;
    //  -------------------------------------------------------------------------
    // --- ДИНАМИЧЕСКОЕ НАПОЛНЕНИЕ ПО ТИПУ ФАЙЛА ---
    uint8_t type = GET_FILE_TYPE (file_id);

    switch (type) {

    case FILE_TYPE_IMAGECONT:
    case FILE_TYPE_IMAGEPROTO:
    case FILE_TYPE_OLED_BIN_IMAGE:

        TFT_matrix_menu_create_menuitem2 ((uint8_t *)"Показать", CMD_FILE_SHOW, CL_GREEN);

        break;

    case FILE_TYPE_SOUNDPROTO:
    case FILE_TYPE_SOUNDCONT:

        TFT_matrix_menu_create_menuitem2 ((uint8_t *)"Проиграть", CMD_FILE_PLAY, CL_GOLD);

        break;

    default:

        TFT_matrix_menu_create_menuitem2 ((uint8_t *)"[не поддерживается]", CMD_FILE_NONE, CL_RED);

        break;
    }
    //  -------------------------------------------------------------------------
    // Кнопки удаления и возврата подходят для всех
    TFT_matrix_menu_create_menuitem ((uint8_t *)"Удалить", CMD_FILE_DELETE);
    TFT_matrix_menu_create_menuitem ((uint8_t *)"[Назад]", CMD_FILE_BACK);
    //  -------------------------------------------------------------------------
    TFT_matrix_menu_set_name ((uint8_t *)"ВЫБОР");
    TFT_matrix_menuname_set_margin (0);
    //  -------------------------------------------------------------------------
    // Обязательно! Завершение создания этого меню
    TFT_matrix_menu_create_menu_complete();
    //  -------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Меню 2 уровня - настройки
void start_menu3() {
    // -------------------------------------------------------------------------
    // Обязательная часть обработки!
    // -------------------------------------------------------------------------
    // Здесь еще доступны все данные вызвавшего нас окна!
    //  -------------------------------------------------------------------------
    // Закрытие предыдущего меню
    TFT_matrix_menu_close_menu();

    // Инициализация нового меню
    if (TFT_matrix_menu_create_new_menu() != 1) {
        return;
    }
    TFT_matrix_menu_default_config (TFT_matrix_get_clientrect(), 40);
    //  -------------------------------------------------------------------------
    // Локальная копия указателя (ссылка)
    TFT_Menu *menu = TFT_matrix_menu_mainmenu();

    menu->MenuID = 0x300;
    menu->Tag = 0;
    menu->DefaultActionIndex = 8;
    menu->ViewItemNumber = 1;
    menu->ItemNumberWidth = 30;
    menu->MenuItemHeight = 40;
    menu->TextScale = 2;
    menu->MenuHeapWidth = 30;
    menu->TextSpace = 3;
    menu->TextMenuSpace = 5;

    // Наполнение пунктами меню
#ifdef USE_SOUND_GEN
    TFT_matrix_menu_create_menuitem ((uint8_t *)"[Инф. о флеш-памяти]", CMD_MEM_INFO);
    TFT_matrix_menu_create_menuitem ((uint8_t *)"[Уборка мусора]", CMD_GO_GCC);
    TFT_matrix_menu_create_menuitem ((uint8_t *)"[Громкость]", CMD_OPTIONS_VOLUME);
#endif
    TFT_matrix_menu_create_menuitem ((uint8_t *)"[Тестовый файл]", CMD_FILE_CREATE);

    TFT_matrix_menu_create_menuitem ((uint8_t *)"[Назад]", CMD_FILE_BACK);

    // Создаем список файлов (если он нужен в настройках)
    flash_create_fileList();

    TFT_matrix_menu_set_name ((uint8_t *)"НАСТРОЙКИ");
    TFT_matrix_menuname_set_margin (0);
    //  -------------------------------------------------------------------------
    // Обязательно! Завершение создания этого меню
    TFT_matrix_menu_create_menu_complete();
    //  -------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Меню 2 уровня - Громкость
void start_menu4() {
    // -------------------------------------------------------------------------
    // Обязательная часть обработки!
    // -------------------------------------------------------------------------
    // Здесь еще доступны все данные вызвавшего нас окна!
    //  -------------------------------------------------------------------------
    // Закрытие предыдущего меню
    TFT_matrix_menu_close_menu();

    // Инициализация нового меню
    if (TFT_matrix_menu_create_new_menu() != 1) {
        return;
    }
    TFT_matrix_menu_default_config (TFT_matrix_get_clientrect(), 40);
    //  -------------------------------------------------------------------------
    // Локальная копия указателя (ссылка)
    TFT_Menu *menu = TFT_matrix_menu_mainmenu();

    // Теперь работаем быстро и коротко
    menu->MenuID = 0x400;
    menu->Tag = 0;

    // menu->BackRetIndex = matrix_menu_get_selected();

    menu->DefaultActionIndex = 8;
    menu->ViewItemNumber = 1;

    menu->ItemNumberWidth = 30;
    menu->MenuItemHeight = 40;
    menu->TextScale = 2;
    menu->MenuHeapWidth = 30;
    menu->TextSpace = 3;
    menu->TextMenuSpace = 5;
    //  -------------------------------------------------------------------------
    TFT_matrix_menu_create_menuitem ((uint8_t *)"Уровень 0%", CMD_SETVOL_0);
    TFT_matrix_menu_create_menuitem ((uint8_t *)"Уровень 25%", CMD_SETVOL_25);
    TFT_matrix_menu_create_menuitem ((uint8_t *)"Уровень 50%", CMD_SETVOL_50);
    TFT_matrix_menu_create_menuitem ((uint8_t *)"Уровень 75%", CMD_SETVOL_75);
    TFT_matrix_menu_create_menuitem ((uint8_t *)"Уровень 100%", CMD_SETVOL_100);
    TFT_matrix_menu_create_menuitem ((uint8_t *)"[Назад]", CMD_FILE_BACK);
    //  -------------------------------------------------------------------------
    // Создаем список файлов в меню
    flash_create_fileList();
    //  -------------------------------------------------------------------------
    TFT_matrix_menu_set_name ((uint8_t *)"ГРОМКОСТЬ");
    TFT_matrix_menuname_set_margin (0);
    //  -------------------------------------------------------------------------
    // Обязательно! Завершение создания этого меню
    TFT_matrix_menu_create_menu_complete();
    //  -------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Меню 2 уровня - Информация о чипе флеш-памяти
void start_menu5() {
    // -------------------------------------------------------------------------
    // Обязательная часть обработки!
    // -------------------------------------------------------------------------
    // Здесь еще доступны все данные вызвавшего нас окна!
    //  -------------------------------------------------------------------------
    TFT_matrix_menu_show_modal_windows ((uint8_t *)"Ждите...", CL_YELLOW, CL_BLUE);
    //  -------------------------------------------------------------------------
    // Закрытие предыдущего меню
    TFT_matrix_menu_close_menu();

    // Инициализация нового меню
    if (TFT_matrix_menu_create_new_menu() != 1) {
        return;
    }
    TFT_matrix_menu_default_config (TFT_matrix_get_clientrect(), 40);
    //  -------------------------------------------------------------------------
    // Локальная копия указателя (ссылка)
    TFT_Menu *menu = TFT_matrix_menu_mainmenu();

    // Теперь работаем быстро и коротко
    menu->MenuID = 0x500;
    menu->Tag = 0;

    menu->DefaultActionIndex = 8;
    menu->ViewItemNumber = 0;

    menu->ItemNumberWidth = 30;
    menu->MenuItemHeight = 40;
    menu->TextScale = 2;
    menu->MenuHeapWidth = 30;
    menu->TextSpace = 3;
    menu->TextMenuSpace = 5;
    //  -------------------------------------------------------------------------
    // Получение информации о памяти
    memoryInfo_t mem_tmp_res = {0};
    W25_MemoryInfo2 (&mem_tmp_res);
    //  -------------------------------------------------------------------------
    // Очищаем старые строки из пула перед новым заходом
    string_list_clear (&my_log);
    string_list_init (&my_log);
    //  -------------------------------------------------------------------------
    // Создаем динамический набор данных
    //  -------------------------------------------------------------------------
    uint8_t idx0 = 0;
    //  -------------------------------------------------------------------------
    switch (flash_hw->chip_id) {

    case HW_FLASH_W25Q32_ID:

        idx0 = string_list_add (&my_log, "Чип: W25Q32");
        break;

    case HW_FLASH_W25Q64_ID:

        idx0 = string_list_add (&my_log, "Чип: W25Q64");
        break;

    case HW_FLASH_W25Q128_ID:

        idx0 = string_list_add (&my_log, "Чип: W25Q128");
        break;
    }
    //  -------------------------------------------------------------------------
    TFT_matrix_menu_create_menuitem2 ((uint8_t *)string_list_get_addr (&my_log, idx0), 0, CL_GREEN);
    //  -------------------------------------------------------------------------
    idx0 = string_list_add (&my_log, "Емкость чипа: ");
    string_list_append_int (&my_log, idx0, mem_tmp_res.chip_size);
    TFT_matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
    //  -------------------------------------------------------------------------
    idx0 = string_list_add (&my_log, "Сбойных страниц: ");
    string_list_append_int (&my_log, idx0, mem_tmp_res.bad_pages);
    TFT_matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
    //  -------------------------------------------------------------------------
    idx0 = string_list_add (&my_log, "Всего (прото): ");
    string_list_append_int (&my_log, idx0, mem_tmp_res.total_pages);
    TFT_matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
    //  -------------------------------------------------------------------------
    idx0 = string_list_add (&my_log, "Использовано: ");
    string_list_append_int (&my_log, idx0, mem_tmp_res.used_pages);
    TFT_matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
    //  -------------------------------------------------------------------------
    idx0 = string_list_add (&my_log, "Свободно: ");
    string_list_append_int (&my_log, idx0, mem_tmp_res.free_pages);
    TFT_matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
    //  -------------------------------------------------------------------------
    idx0 = string_list_add (&my_log, "Корзина: ");
    string_list_append_int (&my_log, idx0, mem_tmp_res.trash_pages);
    TFT_matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
    //  -------------------------------------------------------------------------
    idx0 = string_list_add (&my_log, "Лента начало: ");
    string_list_append_int (&my_log, idx0, mem_tmp_res.cont_area_begin);
    TFT_matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
    //  -------------------------------------------------------------------------
    idx0 = string_list_add (&my_log, "Свободно: ");
    string_list_append_int (&my_log, idx0, mem_tmp_res.cont_area_free);
    TFT_matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
    //  -------------------------------------------------------------------------
    TFT_matrix_menu_create_menuitem ((uint8_t *)"[Назад]", CMD_FILE_BACK);
    //  -------------------------------------------------------------------------
    TFT_matrix_menu_set_name ((uint8_t *)"ПАМЯТЬ");
    TFT_matrix_menuname_set_margin (0);
    //  -------------------------------------------------------------------------
    // Обязательно! Завершение создания этого меню
    TFT_matrix_menu_create_menu_complete();
    //  -------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Действие с выбранным файлом (для некоторых меню)
void on_actions_menu_click (uint16_t action_idx, uint16_t file_id) {

    uint8_t type = GET_FILE_TYPE (file_id);
    do_action_snd();

    switch (action_idx) {

    case CMD_FILE_SHOW:  // Кликнули по кнопке "Показать"
#ifdef USE_TFT_ST7789
        if (type == FILE_TYPE_IMAGEPROTO) {

            ST7789_DisplayOff();
            ST7789_drawIconFF (file_id, 0, 0, 1);
            ST7789_DisplayOn();
            is_stop_screen = 1;

            return;

        } else if (type == FILE_TYPE_IMAGECONT) {

            ST7789_DisplayOff();
            ST7789_drawContIconFF (file_id, 0, 0, 1);
            ST7789_DisplayOn();
            is_stop_screen = 1;

            return;
        }
#endif
        if (type == FILE_TYPE_OLED_BIN_IMAGE) {

            TFT_matrix_menu_show_modal_windows ((uint8_t *)"Не поддерживается!", CL_YELLOW, CL_RED);
            is_stop_screen = 1;

            return;
        }

        // Чтобы меню2 (0x200) снова появилось
        // на экране для этого же файла, принудительно перерисовываем его:
        start_menu2 (file_id);

        break;

    case CMD_FILE_PLAY:  // Кликнули по кнопке "Проиграть"
#ifdef USE_SOUND_GEN
        TFT_matrix_menu_show_modal_windows ((uint8_t *)"Ждите...", CL_YELLOW, CL_BLUE);
        if (type == FILE_TYPE_SOUNDPROTO) {

            Play_SoundFromFlash (file_id);
        } else if (type == FILE_TYPE_SOUNDCONT) {

            Play_SoundContFromFlash (file_id);
        }
#endif
        TFT_matrix_menu_refresh_now();
        is_stop_screen = 0;

        break;

    case CMD_FILE_DELETE:  // Кликнули по кнопке "Удалить"

        flash_erase_file (file_id);
        Delay_Ms (3000);

        // Возвращаем в главное меню файлов (экран 0x100)
        start_mainmenu(0);

        break;

    case CMD_FILE_BACK:  // Кликнули по кнопке "Назад"
        // Поскольку список файлов в RAM (menu_labels) и file_info_count не пострадали,
        // заново сканировать флешку не нужно — просто активируем главное меню.
        start_mainmenu(0);

        break;
    }
}
//-----------------------------------------------------------------------------
// Обработчик клика по пункту всех меню приложения
void menu_do_click_action (int16_t menu_item_idx) {
    // ---------------------------------------------------------------------
    do_action_snd();
    // ---------------------------------------------------------------------
    TFT_Menu *menu = TFT_matrix_menu_mainmenu();
    // ---------------------------------------------------------------------
    // Обработчик главного меню
    if (menu->MenuID == 0x100 && menu->State == Running) {

        uint16_t actionID = TFT_matrix_menu_get_selected_item()->ActionID;

        uint8_t type = GET_FILE_TYPE (actionID);

        if (type == FILE_TYPE_SYS_ACTION) {

            switch (actionID) {

            case CMD_COMMON_OPTIONS:

                start_menu3();
                break;
            }
        } else {

            start_menu2 (actionID);
        }

        return;
    }
    // ---------------------------------------------------------------------
    // Обработчик меню2
    if (menu->MenuID == 0x200 && menu->State == Running) {

        // 1. Извлекаем ActionID выбранного пункта (например, 0xF002)
        uint16_t actionID = TFT_matrix_menu_get_selected_item()->ActionID;
        uint16_t file_id = menu->Tag;

        on_actions_menu_click (actionID, file_id);

        return;
    }
    // ---------------------------------------------------------------------
    // Обработчик меню3
    if (menu->MenuID == 0x300 && menu->State == Running) {

        // 1. Извлекаем ActionID выбранного пункта (например, 0xF002)
        uint16_t actionID = TFT_matrix_menu_get_selected_item()->ActionID;
        uint16_t file_id = menu->Tag;

        uint8_t type = GET_FILE_TYPE (actionID);

        if (type == FILE_TYPE_SYS_ACTION) {

            switch (actionID) {
#ifdef USE_SOUND_GEN
            case CMD_OPTIONS_VOLUME:

                start_menu4();
                return;
#endif
            case CMD_FILE_CREATE:

                TFT_matrix_menu_show_modal_windows ((uint8_t *)"В разработке...", CL_YELLOW, CL_RED);
                is_stop_screen = 1;
                return;

            case CMD_MEM_INFO:

                start_menu5();
                return;

            case CMD_GO_GCC:

                TFT_matrix_menu_show_modal_windows ((uint8_t *)"Очистка мусора...", CL_YELLOW, CL_RED);
                flash_go_gcc();

                TFT_matrix_menu_refresh_now();
                is_stop_screen = 0;
                return;
            }
        }

        on_actions_menu_click (actionID, file_id);

        return;
    }
    // ---------------------------------------------------------------------
    // Обработчик меню4
    if (menu->MenuID == 0x400 && menu->State == Running) {

        // 1. Извлекаем ActionID выбранного пункта (например, 0xF002)
        uint16_t actionID = TFT_matrix_menu_get_selected_item()->ActionID;
        // uint16_t file_id = menu->Tag;

        uint8_t type = GET_FILE_TYPE (actionID);

        if (type == FILE_TYPE_SYS_ACTION) {

            switch (actionID) {
#ifdef USE_SOUND_GEN
            case CMD_SETVOL_0:

                Play_SetVolume (0);
                break;

            case CMD_SETVOL_25:

                Play_SetVolume (25);
                break;

            case CMD_SETVOL_50:

                Play_SetVolume (50);
                break;

            case CMD_SETVOL_75:

                Play_SetVolume (75);
                break;

            case CMD_SETVOL_100:

                Play_SetVolume (100);
                break;
#endif
            }
        }

        start_menu3();

        return;
    }
    // ---------------------------------------------------------------------
    // Обработчик меню5
    if (menu->MenuID == 0x500 && menu->State == Running) {

        // 1. Извлекаем ActionID выбранного пункта (например, 0xF002)
        uint16_t actionID = TFT_matrix_menu_get_selected_item()->ActionID;
        // uint16_t file_id = menu->Tag;

        if (actionID == CMD_FILE_BACK) {
            start_menu3();
        }

        return;
    }
    // ---------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// Главный метод приложения
int main (void) {
    //-------------------------------------------------------------------------
    // Запуск процесса
    NVIC_PriorityGroupConfig (NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    //-------------------------------------------------------------------------
#ifdef USE_W25Q_DEBUG
    USART_Printf_Init (115200);
    printf ("SystemClk:%u\r\n", (unsigned int)SystemCoreClock);
    printf ("ChipID:%08x\r\n", (unsigned int)DBGMCU_GetCHIPID());
#endif
    //-------------------------------------------------------------------------
    Delay_Ms (500);
    //-------------------------------------------------------------------------
    // Инициализация механизма SPI/Flash
    W25_Init();
    uint32_t _FlashID = W25_ReadID();
    //-------------------------------------------------------------------------
#ifdef USE_W25Q_DEBUG
    printf ("FlashID:%08x\r\n", (unsigned int)_FlashID);
#endif
    //-------------------------------------------------------------------------
    if (W25_InitHardware (_FlashID, 0) == 0) {
        // Блокирование работы программы
#ifdef USE_W25Q_DEBUG
        printf ("ERROR: Unknown FlashID:%08x\r\n", (unsigned int)_FlashID);
        printf ("Stopped...");
#endif
        // Ошибка 404 - нет флеш памяти!
        // xproto_resetBufferData();
        // xproto_sendStr ((uint8_t *)"ERRXP:404", 9, 1);

        while (1) { Delay_Ms (1000); }
    }
    // ------------------------------------------------------------------------
#ifdef USE_TFT_ST7789
    // Инициализация TFT дисплея
    ST7789_SPI_Init();
    ST7789_Init_Landscape();
    // Вывод тестовой графики
    // view_promo();
    // ------------------------------------------------------------------------
#endif
    //-------------------------------------------------------------------------
#ifdef USE_OLED_SSD1306
    SSD1306_BASE_Init (0xCF);
    view_oled_promo();
#endif
    //-------------------------------------------------------------------------
#ifdef USE_TRANSPORT_UART
    // UART
    UART_UNIT_Init (115200);
    //-------------------------------------------------------------------------
    // xproto_setReadyToReceive();
#endif
#ifdef USE_W25Q_DEBUG
    printf ("Running (v_CH32V003)...\r\n");
#endif
    //-------------------------------------------------------------------------
#ifdef USE_ENCODER_EC11
    // Инициализация энкодера
    EC11_Init();
    uint8_t enc_button_value = 0;
#endif
    //-------------------------------------------------------------------------
#ifdef USE_SOUND_GEN
    // Инициализация звука
    Play_SoundInit();
    Play_SetVolume (75);
#endif
    //-------------------------------------------------------------------------
    // Запуск корневого меню приложения
    start_mainmenu(0);
    //-------------------------------------------------------------------------
    while (1) {
#ifdef USE_TRANSPORT_UART
        // Выполняем обработку очереди сообщений
        xproto_processingCommand();
#endif
        // ---------------------------------------------------------------------
        // Получение прямой ссылки на активное в данный момент меню
        TFT_Menu *menu = TFT_matrix_menu_mainmenu();
        // ---------------------------------------------------------------------
#ifdef USE_ENCODER_EC11

        int16_t selected_item_idx = TFT_matrix_menu_get_selected();

        switch (EC11_GetEncoderRotationState()) {
        
        // Энкодер вращается по часовой стрелке (один тик)
        case ENC_EC11_TICK_CWISE:

            if (selected_item_idx + 1 < menu->Count) {

                TFT_matrix_menu_set_selected (selected_item_idx + 1);

            } else {

                TFT_matrix_menu_set_selected (0);
            }

            do_action_snd();

            break;

        // Эндодер вращается против часовой стрелки (один тик)
        case ENC_EC11_TICK_CCWISE:

            if (selected_item_idx > 0) {

                TFT_matrix_menu_set_selected (selected_item_idx - 1);
            } else {

                TFT_matrix_menu_set_selected (menu->Count - 1);
            }

            do_action_snd();

            break;

        default:

            break;
        }
        // ---------------------------------------------------------------------
        // Кнопка энкодера нажата?
        if (EC11_GetEncoderButtonState() == 0) {
            // Кнопка энкодера нажата!
            enc_button_value++;

            // Уведомляем пользователя о достижении "долгого нажатия"
            if (enc_button_value == ENC_BUT_LONG_VALUE) {

                do_action_snd();
            }
        } else {
            // Кнопка энкодера не нажата или отпущена в прошлом цикле...
            if (enc_button_value > 0) {
                // Долгое нажатие кнопки энкодера
                if (enc_button_value > ENC_BUT_LONG_VALUE) {

                    // Возврат в главное меню
                    uint8_t start_zero = (menu->MenuID == 0x100) ? 1 : 0;
                    start_mainmenu(start_zero);
                } else {

                    // Короткое нажатие кнопки энкодера
                    if (is_stop_screen == 0) {

                        // Выполнение действия
                        menu_do_click_action (selected_item_idx);
                    } else {

                        // Разблокирование модального сообщения
                        is_stop_screen = 0;
                        TFT_matrix_menu_refresh_now();
                    }
                }
            }

            // Сброс счетчика нажатия кнопки
            enc_button_value = 0;
        }
        // ---------------------------------------------------------------------
#endif
        // ---------------------------------------------------------------------
        // Если выведено модальное сообщение, меню не перерисовывается
        if (is_stop_screen == 0) {

            TFT_matrix_menu_draw();

            if (menu->State == Newed) {

                menu->State = Running;
            }
        }
        // ---------------------------------------------------------------------
        // Задержка формирователя цикла обслуживания
        Delay_Ms (100);
        // ---------------------------------------------------------------------
    }
    //-------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
