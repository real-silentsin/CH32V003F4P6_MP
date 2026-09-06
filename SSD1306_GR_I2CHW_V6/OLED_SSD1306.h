/********************************** (C) COPYRIGHT ******************************
 * File Name          : SSD1306.h
 * Author             : vantr
 * Version            : V1.0.0
 * Date               : 2025/12/22
 * Description        : Библиотека текстового вывода на дисплеи SSD1306 (ПОЛНАЯ)
 *                    : Аппаратный I2C!  
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе. Допускается к использованию.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef XC_HEADER_SSD1306LW_H
#define	XC_HEADER_SSD1306LW_H
// -----------------------------------------------------------------------------
#include "app_config.h"
// -----------------------------------------------------------------------------
// Физические размеры устройства (всегда в основе мелкие символы!)
// НЕ МЕНЯТЬ ВРУЧНУЮ!
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
#define DISPLAY_HEIGHT      32
#define DISPLAY_WIDTH       128  
#endif

#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
#define DISPLAY_HEIGHT      64
#define DISPLAY_WIDTH       128
#endif

#define DISPLAY_HARDWARE_BUFFER_SIZE ((DISPLAY_HEIGHT * DISPLAY_WIDTH) / 8)
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
// -----------------------------------------------------------------------------
//  Варианты переноса при переполнении
typedef enum {
    NOWRAP2 = 0,      // Переноса нет
    NORMALWRAP2 = 1,  // Перенос до конца поля без возврата в начало экрана
    FULLWRAP2 = 2     // Полный перенос с возвратом в начало экрана
} OLED_SSD1306_WRAP_MODE;
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

    0xB0, // Стартовая страница GDDRAM [0..7]

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

    //OLED_SSD1306_BASE_ClearScreen(1);

    0xAF //--turn on SSD1306 panel
};
#endif
// -----------------------------------------------------------------------------
// Экспортируемые методы
// -----------------------------------------------------------------------------
void OLED_SSD1306_BASE_Init(uint8_t init_contrast);

//uint8_t SSD1306_BASE_WRITECMD(uint8_t cmd);
//uint8_t SSD1306_BASE_WRITEDATA(uint8_t data);

void OLED_SSD1306_BASE_ON(void);
void OLED_SSD1306_BASE_OFF(void);

void OLED_SSD1306_BASE_ClearScreen(uint8_t on_by_complete);
//void SSD1306_BASE_SetCursor(uint8_t X, uint8_t Y);
void OLED_SSD1306_BASE_CONTRAST(uint8_t value);

void OLED_SSD1306_BASE_SendBuffer(uint8_t *buffer, uint16_t count, uint8_t clear_screen);

#ifdef SSD1306_USE_EEPROM_FONTS
uint8_t OLED_SSD1306_LoadSymbol(uint16_t sym_index);
#endif

/* void SSD1306_BASE_SetWrapMode(OLED_SSD1306_WRAP_MODE NewMode);
void SSD1306_BASE_CarretReturnThisLine(void);
void SSD1306_BASE_CarretReturnThisScreen(void);
void SSD1306_BASE_IncrementCarret(void);
// -----------------------------------------------------------------------------
// Одиночные символы
void SSD1306_BASE_PutSymbol(uint8_t symbol_code, uint8_t style);
void SSD1306_BASE_PutDoubleSymbol(uint8_t symbol_code, uint8_t style);
void SSD1306_BASE_PutWidthSymbol(uint8_t symbol_code, uint8_t style);
void SSD1306_BASE_PutWideDoubleSymbol(uint8_t symbol_code, uint8_t style);

// Общий метод вывода строк
void SSD1306_BASE_WriteStr(char *str, uint8_t count, uint8_t style); */
// -----------------------------------------------------------------------------
#endif	/* XC_HEADER_SSD1306LL_H */
// -----------------------------------------------------------------------------

