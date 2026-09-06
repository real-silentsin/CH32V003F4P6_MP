/********************************** (C) COPYRIGHT ******************************
 * File Name          : SSD1306.h
 * Author             : vantr
 * Version            : V2.1.0
 * Date               : 2026/07/28
 * Description        : Библиотека текстового вывода на дисплеи SSD1306 (ПОЛНАЯ)
 *                    : Аппаратный I2C! Возможно использование EEPROM для шрифта
 *                    : Версия для CH32X033 MEGAPACK
 *******************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef XC_HEADER_SSD1306_5_H
#define	XC_HEADER_SSD1306_5_H
// -----------------------------------------------------------------------------
#include "app_config.h"
// -----------------------------------------------------------------------------
// Физические размеры устройства (всегда в основе мелкие символы!)
// НЕ МЕНЯТЬ ВРУЧНУЮ!
#define OLED_DISPLAY_WIDTH          128
#define OLED_DISPLAY_WIDTH_SYM      16

#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
#define OLED_DISPLAY_HEIGHT_SYM     32
#define OLED_DISPLAY_HEIGHT_ROW     4
#endif

#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
#define OLED_DISPLAY_HEIGHT_SYM     64
#define OLED_DISPLAY_HEIGHT_ROW     8
#endif
// -----------------------------------------------------------------------------
#include "SSS_Common_Lib_V1/sss_classes.h"
// -----------------------------------------------------------------------------
// Команды SSD1306 (D/C# = 0, R/W# = 0, E(RD#) = 1)
#define SSD1306_CMD_SETCONTRAST         0x81    // Установка контрастности
#define SSD1306_CMD_ENTIREDISPLAY_ON    0xA4    // Возобновление отображения RAM
#define SSD1306_CMD_ENTIREDISPLAY_OFF   0xA5    // Запрет отображения RAM
#define SSD1306_CMD_NORMALVIEW          0xA6    // Нормальное отображение дисплея
#define SSD1306_CMD_INVVIEW             0xA7    // Инверсное отображение дисплея
#define SSD1306_CMD_SETDISPLAY_OFF      0xAE    // Экран выключен
#define SSD1306_CMD_SETDISPLAY_ON       0xAF    // Экран включен
// -----------------------------------------------------------------------------
// Адресные команды SSD1306
#define SSD1306_CMDADDR_SETLCOLPAM      0x00    // Младший ниббл регистра начального адреса столбца
#define SSD1306_CMDADDR_SETHCOLPAM      0x10    // Старший ниббл регистра начального адреса столбца
// -----------------------------------------------------------------------------
// Типы адресации памяти дисплея
#define SSD1306_CMDADDR_SETMODE_1       0x20    // Режим горизонтальной адресации
#define SSD1306_CMDADDR_SETMODE_2       0x21    // Режим вертикальной адресации
#define SSD1306_CMDADDR_SETMODE_3       0x22    // Режим страничной адресации (по умолчанию после сброса)
#define SSD1306_CMDADDR_SETMODE_X       0x23    // Недопустимый режим адресации!
// -----------------------------------------------------------------------------
#define SSD1306_CMDCOLADDR_SETCOL       0x21    // Первый байт команды установки начального и конечного столбцов [0..127] (0 - по умолчанию)
#define SSD1306_CMDPAGEADDR_SETPAGE     0x22    // Первый байт команды установки начальной и конечной строки [0..7] (0 - по умолчанию)
#define SSD1306_CMDPAGEADDR_SETSTART    0xB0    // Первый байт команды установки адреса начальной страницы GDDRAM 
// -----------------------------------------------------------------------------
// Заголовок записи
#define SSD1306_CMD_WRITECMD            0x00    // Производится запись команды
#define SSD1306_CMD_WRITEDATA           0x40    // Производится запись данных
// -----------------------------------------------------------------------------
#define SSD1306_ERR_NOERR               0x00    // Нет ошибок при обмене
#define SSD1306_ERR_NOCHIP              0x01    // Ошибка при вводе адреса (нет чипа?)
#define SSD1306_ERR_NOACK               0x02    // Нет подтверждения приема ACK от контроллера SSD1306
//  -----------------------------------------------------------------------------
//  Варианты переноса при переполнении
typedef enum {
    NOWRAP4 = 0,      // Переноса нет
    NORMALWRAP4 = 1,  // Перенос до конца поля без возврата в начало экрана
    FULLWRAP4 = 2     // Полный перенос с возвратом в начало экрана
} SSD1306_WRAP_MODE4;
//  -----------------------------------------------------------------------------
//  Варианты ограничения печати целых чисел
typedef enum {
    REST_0 = 0,     // Ограничений нет
    REST_2 = 2,     // Ограничение до 2 символов
    REST_3 = 3,     // Ограничение до 3 символов
    REST_4 = 4,     // Ограничение до 4 символов
    REST_5 = 5,     // Ограничение до 5 символов
    REST_6 = 6,     // Ограничение до 6 символов
    REST_7 = 7,     // Ограничение до 7 символов
    REST_8 = 8,     // Ограничение до 8 символов
    REST_9 = 9      // Ограничение до 9 символов
} SSD1306_PRINTINT_RESTRICT_MODES;
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_NEW_INIT
// Данные инициализации дисплея
// Шаг 9 - контрастность дисплея
// После шага 26 должна быть очистка экрана
static const uint8_t display_init_data[28] = {
    0xAE, // Дисплей выключен

    0x20, // Режим автоматической адресации
    0x00, // 0x00 - по горизонтали с переходом на новую страницу (строку)
    // 0x01 - по вертикали с переходом на новую строку
    // 0x02 - только по выбранной странице без перехода 

    0xB0, // Старновая страница GDDRAM [0..7]

    0xC8, // Режим сканирования озу дисплея для изменения системы координат
    // С0 - снизу/верх (начало нижний левый угол)
    // С8 - сверху/вниз (начало верний левый угол)

    0x00, // Установка начального адреса колонки
    0x10, // Установка конечного адреса колонки

    0x40, // Установка начального адреса строки [0..63]

    0x81, // Установка яркости (контрастности) дисплея
    0xCF, // ! SSD1306_BASE_WRITECMD(init_contrast);

    0xA1, // set segment re-map 0 to 127

    0xA6, //--set normal display
    0xA8, //--set multiplex ratio(1 to 64)
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
    0x1F,
#endif
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
    0x3F,
#endif

    0xA4, //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    0xD3, //-set display offset
    0x00, //-not offset

    0xD5, //--set display clock divide ratio/oscillator frequency
    0xF0, //--set divide ratio

    0xD9, //--set pre-charge period
    0x22,

    0xDA, //--set com pins hardware configuration
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
    0x02,
#endif
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
    0x12,
#endif    

    0xDB, //--set vcomh
    0x20, //0x20,0.77xVcc

    0x8D, //--set DC-DC enable
    0x14,

    //SSD1306_TXT_ClearScreen(1);

    0xAF //--turn on SSD1306 panel
};
#endif
// -----------------------------------------------------------------------------
// Экспортируемые методы
// -----------------------------------------------------------------------------
void SSD1306_TXT_Init(uint8_t init_contrast);

void SSD1306_TXT_ON(void);
void SSD1306_TXT_OFF(void);

void SSD1306_TXT_ClearScreen(uint8_t on_by_complete);
void SSD1306_TXT_SetCursor(uint8_t X, uint8_t Y);
void SSD1306_TXT_CONTRAST(uint8_t value);

void SSD1306_TXT_SetWrapMode(SSD1306_WRAP_MODE4 NewMode);
void SSD1306_TXT_CarretReturnThisLine(void);
void SSD1306_TXT_CarretReturnThisScreen(void);
void SSD1306_TXT_IncrementCarret(void);
// -----------------------------------------------------------------------------
// Одиночные символы
void SSD1306_TXT_PutSymbol(uint8_t symbol_code, uint16_t style);
void SSD1306_TXT_PutSymbolEx(uint8_t symbol_code, uint16_t style);
void SSD1306_TXT_PutDoubleSymbol(uint8_t symbol_code, uint16_t style);
void SSD1306_TXT_PutWidthSymbol(uint8_t symbol_code, uint16_t style);
void SSD1306_TXT_PutWideDoubleSymbol(uint8_t symbol_code, uint16_t style);
// -----------------------------------------------------------------------------
void SSD1306_TXT_PrintLongInt(uint32_t value, uint8_t format, uint8_t point, uint16_t style);
void SSD1306_TXT_SetPrIntRestrict(SSD1306_PRINTINT_RESTRICT_MODES mode);
void SSD1306_TXT_Print8_hex(uint8_t value, uint16_t style);
void SSD1306_TXT_Print16_hex(uint16_t value, uint16_t style);
void SSD1306_TXT_Print32_hex(uint32_t value, uint16_t style);
// -----------------------------------------------------------------------------
// Общий метод вывода строк
void SSD1306_TXT_SetStr_Limit (uint8_t limit_value);
void SSD1306_TXT_WriteStr (uint8_t *str, uint16_t style);
// -----------------------------------------------------------------------------
#endif	/* XC_HEADER_SSD1306_H */
// -----------------------------------------------------------------------------

