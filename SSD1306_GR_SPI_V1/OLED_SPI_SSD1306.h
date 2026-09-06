/********************************** (C) COPYRIGHT ******************************
 * File Name          : SSD1306.h
 * Author             : vantr
 * Version            : V1.1.0
 * Date               : 2025/07/01
 * Description        : Библиотека драйвера дисплея SSD1306 SPI (7pin)
 *                    : Аппаратный SPI, часть библиотеки MEGAPACK
 *******************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef XC_HEADER_SSD1306LL_H
#define	XC_HEADER_SSD1306LL_H
// -----------------------------------------------------------------------------
// ВНИМАНИЕ!!! Физический размер матрицы, оставьте только один!
// Правится пользователем!
//#define SSD1306_DISPLAY_DIMENSIONS_128X32
#define SSD1306_DISPLAY_DIMENSIONS_128X64
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
//  -----------------------------------------------------------------------------
//  Варианты переноса при переполнении
typedef enum _SSD1306_WRAP_MODE {
    NOWRAP = 0,      // Переноса нет
    NORMALWRAP = 1,  // Перенос до конца поля без возврата в начало экрана
    FULLWRAP = 2     // Полный перенос с возвратом в начало экрана
} SSD1306_WRAP_MODE;
// -----------------------------------------------------------------------------
// Экспортируемые методы
// -----------------------------------------------------------------------------
void OLED_SPI_SSD1306_BASE_Init(uint8_t init_contrast);

void OLED_SPI_SSD1306_BASE_ON(void);
void OLED_SPI_SSD1306_BASE_OFF(void);

void OLED_SPI_SSD1306_BASE_ClearScreen(uint8_t on_by_complete);
void OLED_SPI_SSD1306_BASE_CONTRAST(uint8_t value);

void OLED_SPI_SSD1306_BASE_SendBuffer(uint8_t *buffer, uint16_t count, uint8_t clear_screen);
uint8_t OLED_SPI_SSD1306_BASE_DrawProtoIcon (uint16_t file_id, uint8_t x, uint8_t y, uint8_t scale_factor);
// -----------------------------------------------------------------------------
#endif	/* XC_HEADER_SSD1306LL_H */
// -----------------------------------------------------------------------------

