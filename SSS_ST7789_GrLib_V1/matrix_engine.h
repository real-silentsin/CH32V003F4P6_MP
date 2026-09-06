/********************************** (C) COPYRIGHT ******************************
 * File Name          : matrix_engine.h
 * Author             : vantr
 * Version            : V1.3.1
 * Date               : 2026/07/10
 * Description        : Библиотека графики для OLED дисплея на ST7789 (CH32X033)
 * Module             : Графическая библиотека MATRIX™
 *******************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef __SSS_MATRIX130__
// -----------------------------------------------------------------------------
#define __SSS_MATRIX130__
// -----------------------------------------------------------------------------
#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_Common_Lib_V1/sss_graphics_common_types.h"
#include "SSS_ST7789_GrLib_V1/sss_st7789_grlib1.h"
// -----------------------------------------------------------------------------
// -- Структуры библиотеки
// -----------------------------------------------------------------------------
// Контекст отрисовки стандартного текста
typedef struct {
    uint8_t scale;
    WriteString_DirectionType direction;
    WriteString_WrapMode wrap_mode;
    WriteString_Align align;
    uint8_t space;
    int16_t x_caret;
    int16_t y_caret;
    uint8_t inverse;
} TextContext2;
// -----------------------------------------------------------------------------
// Для управления контекстом
typedef enum {
    TX_Scale,
    TX_Direction,
    TX_Space,
    TX_Wrap,
    TX_Align,
    TX_X_Caret,
    TX_Y_Caret,
    TX_Inverse,
    TX_ForeColor,
    TX_BackColor
} TextContextTypes2;
// -----------------------------------------------------------------------------
typedef struct {
    Point Location;
    Size ClientSize;
    uint16_t BorderColor;
    uint16_t FillColor;
    uint16_t BackColor;
    uint8_t Value;      // 0...100
    uint8_t PrevValue;  // Для умной перерисовки
} ProgressBar;
// ----------------------------------------------------------------------------
// Выбор поверхности
typedef enum {
    // Цвет переднего плана
    ForeGround,
    // Цвет заднего плана
    BackGround
} TargetPlanes;
// -----------------------------------------------------------------------------
// -- Экспортируемые функции библиотеки
// -----------------------------------------------------------------------------
// Управление выводом
void TFT_matrix_frame_init();
void TFT_matrix_clearScreen();

void TFT_matrix_set_clientrect (Rect value);
void TFT_matrix_restoreClientRect();
Rect TFT_matrix_get_clientrect();
void TFT_matrix_client_rect();

void TFT_matrix_returnCaret();
void TFT_matrix_setCaret (int16_t value);
void TFT_matrix_addCaretX (int16_t value);
void TFT_matrix_addCaretY (int16_t value);
void TFT_matrix_setMargin (int16_t value);
void TFT_matrix_setCoord (int16_t X, int16_t Y);
void TFT_matrix_setCoord2 (Point data);
void TFT_matrix_centerCaret (Rect area, uint8_t *str);
void TFT_matrix_pushCaret();
void TFT_matrix_popCaret();

void TFT_matrix_setBrush (uint16_t color);
uint16_t TFT_matrix_getBrush();
uint16_t TFT_matrix_getBackColor();
void TFT_matrix_setBackColor(uint16_t color);
void TFT_matrix_putPixelClip (uint16_t x, uint16_t y);
// -----------------------------------------------------------------------------
// Примитивы и области
Point TFT_matrix_create_point (uint16_t X, uint16_t Y);
Size TFT_matrix_create_size (uint16_t width, uint16_t height);
Rect TFT_matrix_create_rect (Point coords, Size client_size);
Rect TFT_matrix_create_rect2 (uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height);

uint8_t TFT_matrix_point_in_rect (Point data, Rect rect);
uint8_t TFT_matrix_point_in_cliprect (uint16_t X, uint16_t Y);

Point TFT_matrix_get_corner (Rect area, Corner subject);
Point TFT_matrix_get_rect_topright (Rect rect);
Point TFT_matrix_get_rect_bottomright (Rect rect);
Point TFT_matrix_get_rect_bottomleft (Rect rect);

uint8_t TFT_matrix_matrix_rect_isEmpty (Rect area);
Rect TFT_matrix_intersect_rects (Rect canvas, Rect window);
Rect TFT_matrix_rect_intersect (Rect r1, Rect r2);
Point TFT_matrix_shift_coords (Point data, int16_t X, int16_t Y);

Rect TFT_matrix_reduce_rect (Rect rect, uint8_t value);
Rect TFT_matrix_reduce_rect2 (Rect rect);
Rect TFT_matrix_increase_rect (Rect rect, uint8_t value);
Rect TFT_matrix_center_rect (Rect owner, Rect area);
Rect TFT_matrix_reduce_rect_height (Rect area, uint16_t delta);
Rect TFT_matrix_inscribeArea (Rect area_owner, Rect area_slave);

void TFT_matrix_draw_rect (Point loc, Size sz);
void TFT_matrix_fillRect (const Rect area, TargetPlanes target);
void TFT_matrix_fillRect2 (uint16_t x, uint16_t y, uint16_t w, uint16_t h, TargetPlanes target);
void TFT_matrix_fillRect3 (Point data, Size size, TargetPlanes target);
void TFT_matrix_hatching_rect (Point data, Size size);

void TFT_matrix_line (int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void TFT_matrix_line2 (Point A1, Point A2);
void TFT_matrix_vertical_line(Point location, uint16_t width, uint8_t thickness);
void TFT_matrix_horizon_line(Point location, uint16_t width, uint8_t thickness);
void TFT_matrix_drawCircle (int16_t x0, int16_t y0, int16_t radius, uint8_t fill);
void TFT_matrix_polygon (Point *data, uint8_t count, uint8_t closed);
// -----------------------------------------------------------------------------
// Стандартные шрифты (FONTS.H)
void TFT_matrix_set_textContext_prop (TextContextTypes2 prop_name, int16_t value);
void TFT_matrix_set_otherContext_prop (TextContext2 *context, TextContextTypes2 prop_name, int16_t value);
void TFT_matrix_copy_textContext (TextContext2 *source_context);
void TFT_matrix_textcontext_default(TextContext2 *context);
void TFT_matrix_writeString (uint8_t *str);
void TFT_matrix_writeChar_Scaled (uint8_t c);
void TFT_matrix_writeCharV_Scaled (uint8_t c);
void TFT_matrix_drawStringInscribed_Smart (uint8_t *str, Rect area_owner);
Size TFT_matrix_measureString (uint8_t *str);
// -----------------------------------------------------------------------------
// Бинарные и GFX шрифты, иконки
// ВНИМАНИЕ! Нестандартные шрифты не поддерживают клиппинг в client_rect!
void TFT_matrix_drawString_GFX2X (const uint8_t *str, const GFXfont *font, float line_spacing);
void TFT_matrix_writeBitmapEx (const uint8_t *bitmap, uint16_t width, uint16_t height);
Size TFT_matrix_measureStringBin (const uint8_t *str, const BINFont *font);
void TFT_matrix_writeStringBin (const uint8_t *str, const BINFont *font, float line_spacing);
void TFT_matrix_draw_7seg_BIN (uint8_t sym_code, const BINFont *font);
void TFT_matrix_draw_7seg_str (uint8_t *str, const BINFont *font);
void TFT_matrix_draw_bitmap (const uint8_t *source);
void TFT_matrix_draw_bin_data (Rect area, const uint8_t *source);
// -----------------------------------------------------------------------------
// Дополнительная графика
void TFT_matrix_drawProgressBar (ProgressBar *pb, uint8_t newValue);
//void TFT_matrix_draw_led_font (uint8_t sym_code, Point coords, const uint8_t *font_data);
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
