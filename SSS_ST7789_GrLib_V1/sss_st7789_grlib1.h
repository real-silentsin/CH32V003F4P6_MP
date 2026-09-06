/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_st7789_grlib1.h
 * Author             : vantr
 * Version            : V1.2.1
 * Date               : 2026/06/16
 * Description        : Драйвер для TFT дисплея ST7789 (графика)
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef _SSS_ST77891__
// -----------------------------------------------------------------------------
#define _SSS_ST77891__
// ----------------------------------------------------------------------------
#include "app_config.h"
#include "SSS_Common_Lib_V1/sss_classes.h"
// ----------------------------------------------------------------------------
// Разрешить методы расширенной печати
//#define TFT_USE_PRINTEX
// -----------------------------------------------------------------------------
// Физические размеры устройства (всегда в основе мелкие символы!)
// НЕ МЕНЯТЬ ВРУЧНУЮ!
#define TFT_DISPLAY_WIDTH           320
#define TFT_DISPLAY_HEIGHT          170

#define DISPLAY_WIDTH_SYM       39   // 0..39 (40 колонок)
#define DISPLAY_HEIGHT_ROW      20  // 0..20 (21 строка)
// ----------------------------------------------------------------------------
#define OFFSET_Y                35  // Это OFFSET_Y
// ----------------------------------------------------------------------------
// Цветной пиксель
typedef union {
    uint16_t full;  // Цвет целиком (0xRRRRRGGGGGGBBBBB)

    struct {
        uint16_t b : 5;  // Синий (0-31)
        uint16_t g : 6;  // Зеленый (0-63)
        uint16_t r : 5;  // Красный (0-31)
    } ch;                // ch = channels (каналы)

    struct {
        uint8_t low;   // Младший байт
        uint8_t high;  // Старший байт
    } byte;
} RGB565_t;
//  -----------------------------------------------------------------------------
// Структура цвет/фон
typedef struct {
    RGB565_t fore;  // Основной цвет (текст/линии)
    RGB565_t back;  // Цвет фона
} ParamsColor;
//  -----------------------------------------------------------------------------
#ifdef TFT_USE_PRINTEX
//  Варианты ограничения печати целых чисел
typedef enum {
    REST_0 = 0,  // Ограничений нет
    REST_2 = 2,  // Ограничение до 2 символов
    REST_3 = 3,  // Ограничение до 3 символов
    REST_4 = 4,  // Ограничение до 4 символов
    REST_5 = 5,  // Ограничение до 5 символов
    REST_6 = 6,  // Ограничение до 6 символов
    REST_7 = 7,  // Ограничение до 7 символов
    REST_8 = 8,  // Ограничение до 8 символов
    REST_9 = 9   // Ограничение до 9 символов
} TFT_PRINTINT_RESTRICT_MODES;
#endif
// ----------------------------------------------------------------------------
// Базовая палитра (RGB565)
// ----------------------------------------------------------------------------
#define CL_BLACK                0x0000
#define CL_WHITE                0xFFFF
#define CL_GRAY                 0x8410
#define CL_LIGHTGRAY            0xD69A
#define CL_DARKGRAY             0x4208
#define CL_DARKENGRAY           0x2945

#define CL_RED                  0xF800
#define CL_GREEN                0x07E0
#define CL_BLUE                 0x001F
#define CL_YELLOW               0xFFE0
#define CL_CYAN                 0x07FF
#define CL_MAGENTA              0xF81F

#define CL_ORANGE               0xFD20
#define CL_PINK                 0xFE19
#define CL_BROWN                0x9181
#define CL_PURPLE               0x8010
#define CL_OLIVE                0x8400
#define CL_TEAL                 0x0410

// Приятные "интерфейсные" цвета
#define CL_NAVY                 0x000F
#define CL_DARKGREEN            0x03E0
#define CL_DARKENGREEN          0x02A0
#define CL_DARKCYAN             0x03EF
#define CL_MAROON               0x7800
#define CL_GOLD                 0xFEA0
#define CL_SKYBLUE              0x867D
#define CL_LIME                 0x07E0
// -----------------------------------------------------------------------------
// Макрос формирования цвета из компонентов RGB
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
// Пример использования:
// ST7789_Clear(RGB565(100, 200, 50)); // Какой-то свой оттенок
// -----------------------------------------------------------------------------
void ST7789_SPI_Init (void);
void ST7789_Init_Landscape (void);

void ST7789_SetAddressWindow (uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ST7789_OpenSession();
void ST7789_CloseSession();
void ST7789_SendByte (uint8_t data);

void ST7789_SendColor (uint16_t color);
void ST7789_putPixel (uint16_t x, uint16_t y, uint32_t color);
void ST7789_putPixelFC (uint16_t x, uint16_t y);

void ST7789_DisplayOff (void);
void ST7789_DisplayOn (void);
void ST7789_Sleep (void);
void ST7789_Wakeup (void);

void ST7789_setForeColor (uint16_t color);
void ST7789_setBackColor (uint16_t color);
uint16_t ST7789_getForeColor();
uint16_t ST7789_getBackColor();
uint16_t ST7789_DarkenColor (uint16_t color, uint8_t factor);
uint16_t ST7789_MixColors (uint16_t color1, uint16_t color2, uint8_t alpha);

#ifdef TFT_USE_PRINTEX
void ST7789_PrintLongInt (uint32_t value, uint8_t format, uint8_t point, uint16_t style);
void ST7789_SetPrIntRestrict (TFT_PRINTINT_RESTRICT_MODES mode);
void ST7789_Print8_hex (uint8_t value, uint16_t style);
void ST7789_Print16_hex (uint16_t value, uint16_t style);
void ST7789_Print32_hex (uint32_t value, uint16_t style);
#endif

#ifdef USE_FLASH_W25Q
void ST7789_drawIconFF (uint16_t file_id, uint16_t x, uint16_t y, uint8_t scall_factor);
void ST7789_drawContIconFF (uint16_t file_id, uint16_t x, uint16_t y, uint8_t scall_factor);
Params16 ST7789_getImageProps (uint16_t file_id);
#endif
// -----------------------------------------------------------------------------
#endif
    // -----------------------------------------------------------------------------