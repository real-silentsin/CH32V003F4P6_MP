/********************************** (C) COPYRIGHT ******************************
 * File Name          : matrix_engine.c
 * Author             : vantr
 * Version            : V1.1.1
 * Date               : 2026/04/02
 * Description        : Библиотека графики для OLED дисплея на ST7789 (CH32X033)
 * Module             : Графическая библиотека MATRIX™
 *******************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе
 *******************************************************************************/
// ------------------------------------------------------------------------------------------------
#include "SSS_ST7789_GrLib_V1/sss_st7789_grlib1.h"

#include "FONTS.h"

#include "matrix_engine.h"
#include "SSS_Common_Lib_V1/sss_classes.h"

#include "SSS_Common_Lib_V1/matrix_cyrrilic.h"

#include <stdio.h>
#include <stdlib.h>
// ------------------------------------------------------------------------------------------------
// Контекст управления выводом стандартного текста
TextContext2 current_text_context;
Point saved_caret = {0};
// ------------------------------------------------------------------------------------------------
// Область отрисовки
Rect client_rect = {
    { 0, 0 },
    { TFT_DISPLAY_WIDTH, TFT_DISPLAY_HEIGHT }
};
// -----------------------------------------------------------------------------
// Восстановление области отрисовки по физическому размеру экрана
void TFT_matrix_restoreClientRect() {

    client_rect.Location.X = 0;
    client_rect.Location.Y = 0;
    client_rect.ClientSize.Width = TFT_DISPLAY_WIDTH;
    client_rect.ClientSize.Height = TFT_DISPLAY_HEIGHT;
}

// ------------------------------------------------------------------------------------------------
// Установка свойств контекста вывода строк стандартным шрифтом
void matrix_set_context_prop (TextContext2 *context, TextContextTypes2 prop_name, int16_t value) {

    switch (prop_name) {

    case TX_Scale:

        context->scale = (uint8_t)((value > 3) ? 3 : value);
        break;

    case TX_Direction:

        context->direction = (WriteString_DirectionType)value;
        break;

    case TX_Space:

        context->space = value;
        break;

    case TX_Wrap:

        context->wrap_mode = (WriteString_WrapMode)value;
        break;

    case TX_Align:

        context->align = (WriteString_Align)value;
        break;

    case TX_X_Caret:

        context->x_caret = value;
        break;

    case TX_Y_Caret:

        context->y_caret = value;
        break;

    case TX_Inverse:

        context->inverse = value;
        break;

    case TX_ForeColor:

        ST7789_setForeColor (value);
        break;

    case TX_BackColor:

        TFT_matrix_setBackColor (value);
        break;
    }
}
// ------------------------------------------------------------------------------------------------
// Копирование указанного контекста в текущий текстовый контекст
void TFT_matrix_copy_textContext(TextContext2 *source_context) {

    if (source_context == NULL) { return; }

    current_text_context.align = source_context->align;
    current_text_context.direction = source_context->direction;
    current_text_context.inverse = source_context->inverse;
    current_text_context.scale = source_context->scale;
    current_text_context.space = source_context->space;
    current_text_context.wrap_mode = source_context->wrap_mode;
    current_text_context.x_caret = source_context->x_caret;
    current_text_context.y_caret = source_context->y_caret;
}
// ------------------------------------------------------------------------------------------------
// Установка свойств контекста вывода строк стандартным шрифтом стандартного контекста
void TFT_matrix_set_textContext_prop (TextContextTypes2 prop_name, int16_t value) {

    matrix_set_context_prop(&current_text_context, prop_name, value);
}
// ------------------------------------------------------------------------------------------------
// Установка свойств контекста вывода строк стандартным шрифтом внешнего контекста
void TFT_matrix_set_otherContext_prop (TextContext2 *context, TextContextTypes2 prop_name, int16_t value) {

    matrix_set_context_prop (context, prop_name, value);
}
// ------------------------------------------------------------------------------------------------
// Установка контекста печати стандартным шрифтом по умолчанию
void TFT_matrix_textcontext_default (TextContext2 *context) {

    context->align = WA_Center;
    context->direction = WS_Horizonlal;
    context->inverse = 0;
    context->scale = 1;
    context->space = 1;
    context->wrap_mode = WM_Full;
    context->x_caret = 0;
    context->y_caret = 0;
}
// ------------------------------------------------------------------------------------------------
// Сохранение текущих координат каретки
void TFT_matrix_pushCaret() {

    saved_caret.X = current_text_context.x_caret;
    saved_caret.Y = current_text_context.y_caret;
    saved_caret.extend = 1;
}
// ------------------------------------------------------------------------------------------------
// Восстановление сохраненных координат каретки
void TFT_matrix_popCaret() {

    if (saved_caret.extend == 1) {
        
        current_text_context.x_caret = saved_caret.X;
        current_text_context.y_caret = saved_caret.Y;
        saved_caret.extend = 0;
    }
}
// ------------------------------------------------------------------------------------------------
// Возврат каретки в начальную, нулевую позицию
void TFT_matrix_returnCaret() {

    current_text_context.x_caret = 0;
    current_text_context.y_caret = 0;
}
// ------------------------------------------------------------------------------------------------
// Установка каретки в любое указанное положение [-127...127]
void TFT_matrix_setCaret (int16_t value) {

    current_text_context.x_caret = value;
}
// ------------------------------------------------------------------------------------------------
// Установка отступа по вертикали
void TFT_matrix_setMargin (int16_t value) {

    current_text_context.y_caret = value;
}
// ------------------------------------------------------------------------------------------------
// Установка каретки в указанные координаты матрицы
void TFT_matrix_setCoord (int16_t X, int16_t Y) {

    current_text_context.x_caret = X;
    current_text_context.y_caret = Y;
}
// ------------------------------------------------------------------------------------------------
// Установка каретки в указанные координаты матрицы
void TFT_matrix_setCoord2 (Point data) {

    current_text_context.x_caret = data.X;
    current_text_context.y_caret = data.Y;
}
// ------------------------------------------------------------------------------------------------
// Сдвиг каретки на указанное количество пикселей [-127...127]
void TFT_matrix_addCaretX (int16_t value) {

    current_text_context.x_caret += value;
}

// ------------------------------------------------------------------------------------------------
// Сдвиг каретки на указанное количество пикселей [-127...127]
void TFT_matrix_addCaretY (int16_t value) {

    current_text_context.y_caret += value;
}
// ------------------------------------------------------------------------------------------------
// Установить цвет переднего плана
void TFT_matrix_setBrush (uint16_t color) {

    ST7789_setForeColor (color);
}
// ------------------------------------------------------------------------------------------------
// Посмотреть цвет переднего плана
uint16_t TFT_matrix_getBrush() {

    return ST7789_getForeColor();
}
// ------------------------------------------------------------------------------------------------
// Посмотреть цвет заднего плана
uint16_t TFT_matrix_getBackColor() {

    return ST7789_getBackColor();
}
// ------------------------------------------------------------------------------------------------
// Установить цвет заднего плана
void TFT_matrix_setBackColor(uint16_t color) {

    ST7789_setBackColor(color);
}
// ------------------------------------------------------------------------------------------------
// Создание точки из координат
Point TFT_matrix_create_point (uint16_t X, uint16_t Y) {

    Point result;

    result.X = X;
    result.Y = Y;

    return result;
}
// ------------------------------------------------------------------------------------------------
// Создание структуры размера из данных
Size TFT_matrix_create_size (uint16_t width, uint16_t height) {

    Size result;

    result.Width = width;
    result.Height = height;

    return result;
}
// ------------------------------------------------------------------------------------------------
// Создание прямоугольной области
Rect TFT_matrix_create_rect (Point coords, Size client_size) {

    Rect result;

    result.Location = coords;
    result.ClientSize = client_size;

    return result;
}
// ------------------------------------------------------------------------------------------------
// Вычисление пересечения прямоугольников
Rect TFT_matrix_intersect_rects (Rect canvas, Rect window) {

    Rect result;

    // 1. Находим координаты левого верхнего угла пересечения
    int16_t x1 = (canvas.Location.X > window.Location.X) ? canvas.Location.X : window.Location.X;
    int16_t y1 = (canvas.Location.Y > window.Location.Y) ? canvas.Location.Y : window.Location.Y;

    // 2. Находим координаты правого нижнего угла (X + Width, Y + Height)
    int32_t canvas_right = canvas.Location.X + canvas.ClientSize.Width;
    int32_t canvas_bottom = canvas.Location.Y + canvas.ClientSize.Height;
    int32_t window_right = window.Location.X + window.ClientSize.Width;
    int32_t window_bottom = window.Location.Y + window.ClientSize.Height;

    int16_t x2 = (canvas_right < window_right) ? (int16_t)canvas_right : (int16_t)window_right;
    int16_t y2 = (canvas_bottom < window_bottom) ? (int16_t)canvas_bottom : (int16_t)window_bottom;

    // 3. Заполняем результат
    result.Location.X = x1;
    result.Location.Y = y1;
    result.Location.extend = 0;  // Или иное значение по умолчанию

    // Проверяем, есть ли пересечение вообще
    if (x2 > x1 && y2 > y1) {
        result.ClientSize.Width = (uint16_t)(x2 - x1);
        result.ClientSize.Height = (uint16_t)(y2 - y1);
    } else {
        result.ClientSize.Width = 0;
        result.ClientSize.Height = 0;
    }

    return result;
}
// ------------------------------------------------------------------------------------------------
// Создание прямоугольной области
Rect TFT_matrix_create_rect2 (uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height) {

    Rect result;

    result.Location = TFT_matrix_create_point (X, Y);
    result.ClientSize = TFT_matrix_create_size (Width, Height);

    return result;
}
// ------------------------------------------------------------------------------------------------
// Проверка, попадает ли указанная точка в указанную область
uint8_t TFT_matrix_point_in_rect (Point data, Rect rect) {

    if (data.X >= rect.Location.X && data.X <= rect.Location.X + rect.ClientSize.Width) {

        if (data.Y >= rect.Location.Y && data.Y <= rect.Location.Y + rect.ClientSize.Height) {

            return 1;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
// Проверка, попадает ли указанная точка в отрисовываемую область экрана
uint8_t TFT_matrix_point_in_cliprect (uint16_t X, uint16_t Y) {

    // Прибавляем +1 к началу и вычитаем -1 от конца.
    // Теперь область отрисовки для контента СТРОГО внутри рамки.
    if (X > client_rect.Location.X && X < (client_rect.Location.X + client_rect.ClientSize.Width - 1)) {
        if (Y > client_rect.Location.Y && Y < (client_rect.Location.Y + client_rect.ClientSize.Height - 1)) {
            return 1;
        }
    }
    return 0;
}
// ------------------------------------------------------------------------------------------------
// Установка области отрисовки дисплея
void TFT_matrix_set_clientrect (Rect value) {

    // 1. Ограничиваем координаты (точка не может быть за пределами дисплея)
    // Используем DISPLAY_WIDTH как предел, координаты от 0 до WIDTH-1
    uint16_t x = (value.Location.X < TFT_DISPLAY_WIDTH) ? value.Location.X : (TFT_DISPLAY_WIDTH - 1);
    uint16_t y = (value.Location.Y < TFT_DISPLAY_HEIGHT) ? value.Location.Y : (TFT_DISPLAY_HEIGHT - 1);

    // 2. Считаем максимально возможный размер от этой точки
    uint16_t maxWidth = TFT_DISPLAY_WIDTH - x;
    uint16_t maxHeight = TFT_DISPLAY_HEIGHT - y;

    // 3. Ограничиваем размер (не больше запрошенного и не больше доступного)
    uint16_t w = (value.ClientSize.Width < maxWidth) ? value.ClientSize.Width : maxWidth;
    uint16_t h = (value.ClientSize.Height < maxHeight) ? value.ClientSize.Height : maxHeight;

    // Записываем результат
    client_rect.Location.X = x;
    client_rect.Location.Y = y;
    client_rect.ClientSize.Width = w;
    client_rect.ClientSize.Height = h;
}
// ------------------------------------------------------------------------------------------------
// Получение области отрисовки экрана
Rect TFT_matrix_get_clientrect() {

    return client_rect;
}
// ------------------------------------------------------------------------------------------------
// Очистка буфера и инициализация графики без изменения физического экрана
// Используется для начала рисования фрейма
void TFT_matrix_frame_init() {

    TFT_matrix_restoreClientRect();

    ST7789_setForeColor (CL_WHITE);
    ST7789_setBackColor(CL_BLACK);

    TFT_matrix_textcontext_default(&current_text_context);

    TFT_matrix_clearScreen();
}
// ------------------------------------------------------------------------------------------------
// Вывести пиксель, если он попадает внутрь области отрисовки
void TFT_matrix_putPixelClip(uint16_t x, uint16_t y) {
    
    if (TFT_matrix_point_in_cliprect(x, y)) {

        ST7789_putPixelFC(x, y);
    }
}
// ------------------------------------------------------------------------------------------------
// Очистка экрана
void TFT_matrix_clearScreen() {

    uint16_t fc_old = TFT_matrix_getBackColor();
    TFT_matrix_setBackColor(CL_BLACK);
    TFT_matrix_fillRect(client_rect, BackGround);
    TFT_matrix_setBackColor(fc_old);
}
// ------------------------------------------------------------------------------------------------
// Рассчет длины указанной строки в пикселях (стандартные шрифты)
Size TFT_matrix_measureString (uint8_t *str) {

    if (current_text_context.scale == 0) { return (Size){ 0, 0 }; }

    Size result = {0};

    result.Height = (current_text_context.scale * 7);

    for (int i = 0; str[i] != '\0'; i++) {

        uint8_t code = str[i];

        // Обработка кириллицы UTF-8
        if ((code == 0xD0 || code == 0xD1) && (matrix_cyr_Get_NOCyrillic() == 0)) {
            code = matrix_cyr_BASE_UTF2CYR (str[i], str[i + 1]);
            i++;  // Пропускаем второй байт UTF-8
        } else {
            // Если шрифт начинается с пробела (0x20)
            code -= 0x20;
        }

        // Берем ширину из нулевого байта образа и масштабируем
        // +1 если в шрифте есть защитный интервал, как в коде печати
        uint8_t char_w = FONT_IMAGES[code][0];

        result.Width += (char_w * current_text_context.scale) + current_text_context.space;
    }

    return result;
}
// ------------------------------------------------------------------------------------------------
// Рисование залитого цветом прямоугольника
void TFT_matrix_fillRect2(uint16_t x, uint16_t y, uint16_t w, uint16_t h, TargetPlanes target) {

    Rect area_t = {{x, y}, {w, h}};
    TFT_matrix_fillRect(area_t, target);
}
// ------------------------------------------------------------------------------------------------
// Рисование залитого цветом прямоугольника
void TFT_matrix_fillRect3 (Point data, Size size, TargetPlanes target) {

    Rect area_t = { data, size };
    TFT_matrix_fillRect(area_t, target);
}
// ------------------------------------------------------------------------------------------------
// Заливка области, ограниченной областью отрисовки client_rect
void TFT_matrix_fillRect(const Rect area, TargetPlanes target) {

    // 1. Входные данные (как они есть)
    int32_t x0 = (int32_t)area.Location.X;
    int32_t y0 = (int32_t)area.Location.Y;
    int32_t x1 = x0 + (int32_t)area.ClientSize.Width - 1;
    int32_t y1 = y0 + (int32_t)area.ClientSize.Height - 1;

    // 2. Границы холста (client_rect)
    int32_t c_l = (int32_t)client_rect.Location.X;
    int32_t c_t = (int32_t)client_rect.Location.Y;
    int32_t c_r = c_l + (int32_t)client_rect.ClientSize.Width - 1;
    int32_t c_b = c_t + (int32_t)client_rect.ClientSize.Height - 1;

    // 3. ЖЕСТКИЙ КЛИППИНГ (Вместо того чтобы выходить, мы ПОДРЕЗАЕМ края)
    if (x0 < c_l) x0 = c_l;
    if (y0 < c_t) y0 = c_t;
    if (x1 > c_r) x1 = c_r;
    if (y1 > c_b) y1 = c_b;

    // 4. ПРОВЕРКА НА ВИДИМОСТЬ (Если после обрезки объект ушел "в минус")
    if (x1 < x0 || y1 < y0) {
        return; // Объект полностью за пределами видимости — вот тут выходим
    }

    // 5. РАСЧЕТ ДЛЯ ST7789 (Теперь x0, y0, x1, y1 гарантированно внутри окна)
    uint16_t real_w = (uint16_t)(x1 - x0 + 1);
    uint16_t real_h = (uint16_t)(y1 - y0 + 1);
    uint32_t total  = (uint32_t)real_w * real_h;

    uint16_t color = (target == ForeGround) ? TFT_matrix_getBrush() : TFT_matrix_getBackColor();

    // 6. ВЫВОД В ЖЕЛЕЗО
    ST7789_SetAddressWindow((uint16_t)x0, (uint16_t)y0, (uint16_t)x1, (uint16_t)y1);
    ST7789_OpenSession();

    for (uint32_t i = 0; i < total; i++) {

        ST7789_SendColor(color);
    }

    ST7789_CloseSession();
}
// ------------------------------------------------------------------------------------------------
// Рисование толстой вертикальной линии цветом переднего плана
void TFT_matrix_vertical_line(Point location, uint16_t width, uint8_t thickness) {

    uint8_t thickness_t = (thickness > 30) ? 30 : thickness;

    Size param_t = { thickness_t, width };

    TFT_matrix_fillRect3(location, param_t, ForeGround);
}
// ------------------------------------------------------------------------------------------------
// Рисование толстой горизонтальной линии цветом переднего плана
void TFT_matrix_horizon_line(Point location, uint16_t width, uint8_t thickness) {

    uint8_t thickness_t = (thickness > 30) ? 30 : thickness;

    Size param_t = { width, thickness_t };

    TFT_matrix_fillRect3(location, param_t, ForeGround);
}
// ------------------------------------------------------------------------------------------------
// Рисование линии (Алгоритм Брезенхема)
void TFT_matrix_line (int16_t x0, int16_t y0, int16_t x1, int16_t y1) {

    // Вертикаль
    if (x0 == x1) {
        Rect r;
        r.Location.X = x0;
        r.Location.Y = (y0 < y1) ? y0 : y1;
        r.ClientSize.Width = 1;
        r.ClientSize.Height = abs (y1 - y0) + 1;
        TFT_matrix_fillRect(r, ForeGround);
        return;
    }
    // Горизонталь
    if (y0 == y1) {
        Rect r;
        r.Location.X = (x0 < x1) ? x0 : x1;
        r.Location.Y = y0;
        r.ClientSize.Width = abs(x1 - x0) + 1;
        r.ClientSize.Height = 1;
        TFT_matrix_fillRect(r, ForeGround);
        return;
    }

    int16_t dx = abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    uint16_t fcolor = TFT_matrix_getBrush();

    while (1) {
        // Увы, для наклонной линии без буфера ставим окно 1x1 для каждой точки
        if (TFT_matrix_point_in_cliprect(x0, y0)) {
        
            ST7789_SetAddressWindow (x0, y0, x0, y0);
            ST7789_OpenSession();
            ST7789_SendColor (fcolor);
            ST7789_CloseSession();
        }

        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}
// ------------------------------------------------------------------------------------------------
// Рисование линии (Алгоритм Брезенхема)
void TFT_matrix_line2 (Point A1, Point A2) {

    TFT_matrix_line (A1.X, A1.Y, A2.X, A2.Y);
}
// ------------------------------------------------------------------------------------------------
// Рисование окружности (Алгоритм Брезенхема)
void TFT_matrix_drawCircle (int16_t x0, int16_t y0, int16_t radius, uint8_t fill) {

    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y) {
        if (fill) {
            // Используем FillRect для горизонтальных линий (заливка)
            // Рисуем 4 горизонтальных хорды
            TFT_matrix_fillRect ((Rect){
                {x0 - x,    y0 + y},
                {2 * x + 1, 1     }
            }, ForeGround);
            TFT_matrix_fillRect ((Rect){
                {x0 - x,    y0 - y},
                {2 * x + 1, 1     }
            }, ForeGround);
            TFT_matrix_fillRect ((Rect){
                {x0 - y,    y0 + x},
                {2 * y + 1, 1     }
            }, ForeGround);
            TFT_matrix_fillRect ((Rect){
                {x0 - y,    y0 - x},
                {2 * y + 1, 1     }
            }, ForeGround);
        } else {
            // Одиночные пиксели (8-симметрия)
            TFT_matrix_putPixelClip (x0 + x, y0 + y);
            TFT_matrix_putPixelClip (x0 + y, y0 + x);
            TFT_matrix_putPixelClip (x0 - y, y0 + x);
            TFT_matrix_putPixelClip (x0 - x, y0 + y);
            TFT_matrix_putPixelClip (x0 - x, y0 - y);
            TFT_matrix_putPixelClip (x0 - y, y0 - x);
            TFT_matrix_putPixelClip (x0 + y, y0 - x);
            TFT_matrix_putPixelClip (x0 + x, y0 - y);
        }

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}
// ------------------------------------------------------------------------------------------------
// Рисование полигона (использует текущий цвет переднего плана)
void TFT_matrix_polygon (Point *data, uint8_t count, uint8_t closed) {

    // Минимум 2 точки для линии, и указатель не должен быть пустым
    if (data == NULL || count < 2)
        return;

    // Проходим по всем точкам, соединяя текущую со следующей
    for (uint8_t i = 0; i < (count - 1); i++) {
        TFT_matrix_line2 (data[i], data[i + 1]);
    }

    // Если полигон замкнутый — соединяем последнюю точку с первой
    if (closed) {
        TFT_matrix_line2 (data[count - 1], data[0]);
    }
}
// ------------------------------------------------------------------------------------------------
// Масштабируемая печать прозрачных (без фона) символов с выводом горизонтально
void TFT_matrix_writeChar_Scaled(uint8_t c) {

    if (current_text_context.scale == 0) return;

    const uint8_t *glyph = FONT_IMAGES[c - 0x20];
    uint8_t sw = glyph[0]; // Реальная ширина символа в битах

    int32_t clip_x0 = (int32_t)client_rect.Location.X;
    int32_t clip_y0 = (int32_t)client_rect.Location.Y;
    int32_t clip_x1 = clip_x0 + (int32_t)client_rect.ClientSize.Width - 1;
    int32_t clip_y1 = clip_y0 + (int32_t)client_rect.ClientSize.Height - 1;

    // 1. Внешний цикл по СТРОКАМ (их всегда 8 под 0-м байтом)
    for (uint8_t row = 0; row < 8; row++) {
        uint8_t row_data = glyph[row + 1]; 

        // 2. Внутренний цикл по БИТАМ (ограничен шириной sw)
        for (uint8_t bit = 0; bit < sw; bit++) {
            
            // Проверяем биты слева направо. 
            // Судя по данным (0x1F для #), старший бит — слева.
            if (row_data & (1 << (sw - 1 - bit))) {

                int32_t start_x = (int32_t)current_text_context.x_caret + (bit * current_text_context.scale);
                int32_t start_y = (int32_t)current_text_context.y_caret + (row * current_text_context.scale);

                // --- КЛИППИНГ (рассчитывается исходя из новой ориентации) ---
                if (start_x <= clip_x1 && (start_x + current_text_context.scale - 1) >= clip_x0 &&
                    start_y <= clip_y1 && (start_y + current_text_context.scale - 1) >= clip_y0) {
                    int32_t rx0 = (start_x < clip_x0) ? clip_x0 : start_x;
                    int32_t ry0 = (start_y < clip_y0) ? clip_y0 : start_y;
                    int32_t rx1 = (start_x + current_text_context.scale - 1 > clip_x1) ? clip_x1 : (start_x + current_text_context.scale - 1);
                    int32_t ry1 = (start_y + current_text_context.scale - 1 > clip_y1) ? clip_y1 : (start_y + current_text_context.scale - 1);

                    TFT_matrix_fillRect2((uint16_t)rx0, (uint16_t)ry0, 
                                    (uint16_t)(rx1 - rx0 + 1), 
                                    (uint16_t)(ry1 - ry0 + 1), ForeGround);
                }
            }
        }
    }

    // 3. Шаг каретки ВПРАВО на sw
    current_text_context.x_caret += (uint16_t)(sw * current_text_context.scale + current_text_context.space);
}
// ------------------------------------------------------------------------------------------------
// Масштабируемая печать прозрачных (без фона) символов с выводом вертикально вниз, перенос справа налево
void TFT_matrix_writeCharV_Scaled (uint8_t c) {

    if (current_text_context.scale == 0)
        return;

    const uint8_t *glyph = FONT_IMAGES[c - 0x20];
    uint8_t sw = glyph[0];  // Теперь это будет "высотой" символа при печати вниз

    int32_t clip_x0 = (int32_t)client_rect.Location.X;
    int32_t clip_y0 = (int32_t)client_rect.Location.Y;
    int32_t clip_x1 = clip_x0 + (int32_t)client_rect.ClientSize.Width - 1;
    int32_t clip_y1 = clip_y0 + (int32_t)client_rect.ClientSize.Height - 1;

    // 1. Внешний цикл по байтам (теперь это шаги по ВЕРТИКАЛИ Y)
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t row_data = glyph[i + 1];

        // 2. Внутренний цикл по битам (теперь это шаги по ГОРИЗОНТАЛИ X)
        for (uint8_t bit = 0; bit < sw; bit++) {

            // Проверяем бит. Чтобы буква не была зеркальной при повороте:
            if (row_data & (1 << (sw - 1 - bit))) {

                // ОТРАЖЕНИЕ ПО ГОРИЗОНТАЛИ:
                // Вместо x_caret + (i * scale) используем инверсию индекса i
                // i перебирает байты (0..7), поэтому 7 - i развернет их зеркально
                int32_t start_x = (int32_t)current_text_context.x_caret + ((7 - i) * current_text_context.scale);
                int32_t start_y = (int32_t)current_text_context.y_caret + (bit * current_text_context.scale);

                // --- КЛИППИНГ ---
                if (start_x <= clip_x1 && (start_x + current_text_context.scale - 1) >= clip_x0 &&
                    start_y <= clip_y1 && (start_y + current_text_context.scale - 1) >= clip_y0) {
                    int32_t rx0 = (start_x < clip_x0) ? clip_x0 : start_x;
                    int32_t ry0 = (start_y < clip_y0) ? clip_y0 : start_y;
                    int32_t rx1 = (start_x + current_text_context.scale - 1 > clip_x1) ? clip_x1 : (start_x + current_text_context.scale - 1);
                    int32_t ry1 = (start_y + current_text_context.scale - 1 > clip_y1) ? clip_y1 : (start_y + current_text_context.scale - 1);

                    TFT_matrix_fillRect2 ((uint16_t)rx0, (uint16_t)ry0,
                                      (uint16_t)(rx1 - rx0 + 1),
                                      (uint16_t)(ry1 - ry0 + 1), ForeGround);
                }
            }
        }
    }

    // 3. Шаг каретки ВНИЗ (используем sw, так как она стала вертикальным размером)
    current_text_context.y_caret += (uint16_t)(sw * current_text_context.scale + current_text_context.space);
}
// ------------------------------------------------------------------------------------------------
// Универсальная печать строки (используется контекст печати стандартным шрифтом!)
// Поддерживается кириллица UTF8 в строках и флаг "здесь нет кириллицы"
void TFT_matrix_writeString (uint8_t *str) {

    // 1. Расчет границ окна (используем знаковые типы для безопасного сравнения)
    int32_t clip_x_min = (int32_t)client_rect.Location.X;
    int32_t clip_y_min = (int32_t)client_rect.Location.Y;
    int32_t clip_x_max = clip_x_min + (int32_t)client_rect.ClientSize.Width - 1;
    int32_t clip_y_max = clip_y_min + (int32_t)client_rect.ClientSize.Height - 1;

    // 1.5. Принудительная привязка каретки к границам окна (если она снаружи)
    // Шаг строки/колонки
    uint16_t line_step = (8 + current_text_context.space) * current_text_context.scale;

    // --- АВТО-КОРРЕКЦИЯ НАЧАЛЬНОЙ ПОЗИЦИИ ---
    if (current_text_context.direction == WS_Horizonlal) {
        // Если каретка левее или выше окна — сбрасываем в левый верхний угол окна
        if ((int32_t)current_text_context.x_caret < clip_x_min)
            current_text_context.x_caret = (uint16_t)clip_x_min;
        if ((int32_t)current_text_context.y_caret < clip_y_min)
            current_text_context.y_caret = (uint16_t)clip_y_min;
    } else {
        // Вертикальная печать (колонки идут влево):
        // Если каретка левее окна (например, 0), ставим её на ПРАВЫЙ край окна
        if ((int32_t)current_text_context.x_caret < clip_x_min)
            current_text_context.x_caret = (uint16_t)clip_x_max - line_step + 1;

        // Если каретка выше окна — на верхний край
        if ((int32_t)current_text_context.y_caret < clip_y_min)
            current_text_context.y_caret = (uint16_t)clip_y_min;
    }

    for (uint16_t i = 0; str[i] != '\0'; ++i) {
        uint8_t final_code;

        // 2. Декодирование UTF-8 (Кириллица)
        // Это критично сделать ДО расчета ширины sym_width
        if ((str[i] == 0xD0 || str[i] == 0xD1) && (matrix_cyr_Get_NOCyrillic() == 0)) {
            if (str[i + 1] == '\0')
                break;
            final_code = matrix_cyr_BASE_UTF2CYR (str[i], str[i + 1]) + 0x20;
            i++;
        } else {
            final_code = str[i];
        }

        // 3. Получение параметров символа из таблицы шрифта
        // Индекс в массиве (смещение 0x20, так как таблица начинается с пробела)
        int16_t font_idx = (int16_t)final_code - 0x20;

        // Защита от выхода за пределы массива шрифта
        if (font_idx < 0 || font_idx >= (int16_t)(sizeof (FONT_IMAGES) / sizeof (FONT_IMAGES[0]))) {
            continue;
        }

        // Реальная ширина конкретного символа из первого байта строки массива
        uint8_t raw_width = FONT_IMAGES[font_idx][0];
        // Полная ширина с учетом межсимвольного интервала и масштаба
        uint16_t sym_full_width = (raw_width + current_text_context.space) * current_text_context.scale;

        // 4. Обработка спецсимволов
        if (final_code == 0xFF) {  // Сдвиг (пробел-заполнитель)
            if (current_text_context.direction == WS_Horizonlal)
                current_text_context.x_caret += sym_full_width;
            else
                current_text_context.y_caret += sym_full_width;
            continue;
        }

        if (final_code == '\n') {  // Принудительный перенос
            if (current_text_context.direction == WS_Horizonlal) {
                current_text_context.x_caret = (uint16_t)clip_x_min;
                current_text_context.y_caret += line_step;
            } else {
                current_text_context.y_caret = (uint16_t)clip_y_min;
                current_text_context.x_caret -= line_step;
            }
            continue;
        }

        // 5. Логика автоматического переноса и отрисовка
        if (current_text_context.direction == WS_Horizonlal) {

            uint16_t char_ink_width = (raw_width * current_text_context.scale);

            // Проверка: влезет ли символ по ширине?
            if ((int32_t)current_text_context.x_caret + char_ink_width > clip_x_max + 1) {

                // ПРОВЕРКА: Если перенос ВЫКЛЮЧЕН (WM_None), просто прекращаем печать
                if (current_text_context.wrap_mode == WM_None) {
                    break;
                }

                current_text_context.x_caret = (uint16_t)clip_x_min;
                current_text_context.y_caret += line_step;
            }

            // Проверка: не вышли ли за нижнюю границу окна?
            if ((int32_t)current_text_context.y_caret > clip_y_max) {
                break;
            }

            TFT_matrix_writeChar_Scaled (final_code);
            // x_caret увеличится внутри TFT_matrix_writeChar_Scaled
        } else {
            // Вертикальная печать (сверху вниз, колонки уходят влево)

            // 1. Считаем ширину только "чернильной" части (без нижнего отступа space)
            uint16_t char_ink_height = raw_width * current_text_context.scale;

            if ((int32_t)current_text_context.y_caret + char_ink_height > clip_y_max + 1) {

                // ПРОВЕРКА: Если перенос ВЫКЛЮЧЕН (WM_None), просто прекращаем печать
                if (current_text_context.wrap_mode == WM_None) {
                    break;
                }

                current_text_context.y_caret = (uint16_t)clip_y_min;
                current_text_context.x_caret -= line_step;
            }

            // Проверка: не вышли ли за левую границу окна?
            if ((int32_t)current_text_context.x_caret < clip_x_min) {
                break;
            }

            TFT_matrix_writeCharV_Scaled (final_code);
            // y_caret увеличится внутри TFT_matrix_writeCharV_Scaled
        }
    }

    matrix_cyr_RESet_NOCyrillic();
}
// ------------------------------------------------------------------------------------------------
// Печать символа BIN шрифта
void TFT_matrix_writeBitmapEx (const uint8_t *bitmap, uint16_t width, uint16_t height) {

    if (bitmap == NULL)
        return;

    uint8_t size = current_text_context.scale;
    if (size == 0)
        size = 1;

    uint16_t start_x = current_text_context.x_caret;
    uint16_t start_y = current_text_context.y_caret;

    uint16_t full_width = width * size;
    uint16_t full_height = height * size;

    // 1. ПРОВЕРКА: влезает ли символ по ширине?
    if (start_x + full_width > TFT_DISPLAY_WIDTH) {

        // Вариант А: Автоперенос на новую строку
        current_text_context.x_caret = 0;
        start_x = 0;
        // Сдвигаем Y на высоту символа + межстрочный интервал
        current_text_context.y_caret += (full_height + 2);  // 2 - небольшой зазор
        start_y = current_text_context.y_caret;
    }

    // 2. ПРОВЕРКА: не ушли ли мы за низ экрана?
    if (start_y /* + full_height */ > TFT_DISPLAY_HEIGHT) {
        
        //return;  // Рисовать негде, выходим
    }

    // Рассчитываем, сколько байт занимает одна строка в массиве
    uint16_t bytesPerRow = (width + 7) / 8;

    // 1. Устанавливаем окно отрисовки (с учетом масштаба)
    ST7789_SetAddressWindow (
        start_x,
        start_y,
        start_x + full_width - 1,
        start_y + full_height - 1);

    uint16_t fr_c = TFT_matrix_getBrush();
    uint16_t bk_c = TFT_matrix_getBackColor();

    // Если включена инверсия — просто меняем их местами
    if (current_text_context.inverse) {
     
        uint16_t temp = fr_c;
        fr_c = bk_c;
        bk_c = temp;
    }

    ST7789_OpenSession();

    for (uint16_t y = 0; y < height; y++) {
        // Дублируем строку по вертикали 'size' раз
        for (uint8_t sy = 0; sy < size; sy++) {

            for (uint16_t b = 0; b < bytesPerRow; b++) {
                // Берем байт текущей строки
                uint8_t val = bitmap[y * bytesPerRow + b];

                // Разбираем байт на биты (MSB First)
                for (int8_t bit = 7; bit >= 0; bit--) {
                    uint16_t current_bit_x = (b * 8) + (7 - bit);

                    // Рисуем, только если не вышли за полезную ширину
                    if (current_bit_x < width) {
                        uint16_t color = (val & (1 << bit)) ? fr_c : bk_c;

                        // Дублируем пиксель по горизонтали 'size' раз
                        for (uint8_t sx = 0; sx < size; sx++) {
                            ST7789_SendColor (color);
                        }
                    }
                }
            }
        }
    }

    ST7789_CloseSession();

    // Обновляем каретку в контексте
    current_text_context.x_caret = start_x + (width * size) + current_text_context.space;
}
// ------------------------------------------------------------------------------------------------
// Печать строки символами двоичного шрифта
void TFT_matrix_writeStringBin (const uint8_t *str, const BINFont *font, float line_spacing) {

    if (str == NULL || font == NULL)
        return;

    uint8_t size = current_text_context.scale;
    if (size == 0)
        size = 1;

    // Считаем реальную ширину и высоту символа на экране
    uint16_t full_w = (uint16_t)font->width * size;
    uint16_t full_h = (uint16_t)font->height * size;
    uint16_t y_step = (uint16_t)(full_h * line_spacing);

    while (*str) {
        uint8_t raw_c = *str++;

        // 1. Обработка переноса строки \n
        if (raw_c == '\n') {
            current_text_context.x_caret = 0;
            current_text_context.y_caret += y_step;
            continue;
        }

        // Индекс символа (смещение 0x20)
        uint16_t c_idx = (uint16_t)(raw_c - 0x20);
        if (c_idx >= font->total_chars)
            continue;

        // 2. ПРОВЕРКА ПЕРЕНОСА (теперь видит Scale!)
        if (current_text_context.x_caret + full_w > TFT_DISPLAY_WIDTH) {
            current_text_context.x_caret = 0;
            current_text_context.y_caret += y_step;
        }

        // 3. ПРОВЕРКА ВЕРТИКАЛИ
        if (current_text_context.y_caret >= TFT_DISPLAY_HEIGHT)
            break;

        // 4. ВЫЗОВ БЫСТРОГО МЕТОДА (который мы портировали для ST7789)
        // Передаем указатель на данные конкретного символа
        TFT_matrix_writeBitmapEx(&font->data[c_idx * font->glyph_size], font->width, font->height);

        // ВНИМАНИЕ: Здесь НЕ НУЖНО двигать x_caret вручную!
        // matrix_writeBitmapEx_ST7789 сама делает:
        // x_caret = start_x + (width * size) + space;
    }
}
// ------------------------------------------------------------------------------------------------
// Измерение длины BIN строки
Size TFT_matrix_measureStringBin (const uint8_t *str, const BINFont *font) {

    uint16_t current_w = 0;
    uint16_t max_w = 0;
    uint16_t lines = 1;

    while (*str) {
        if (*str == '\n') {
            lines++;
            current_w = 0;
        } else {
            current_w += (font->width + current_text_context.space);

            if (current_w > max_w)
                max_w = current_w;
        }

        str++;
    }

    return (Size){(uint8_t)max_w, (uint8_t)(lines * font->height)};
}
// ------------------------------------------------------------------------------------------------
// Рисование простой рамки указанного размера
void TFT_matrix_draw_rect (Point loc, Size sz) {

    if (sz.Width == 0 || sz.Height == 0) return;

    // Считаем КРАЙНИЕ точки один раз
    uint16_t x0 = loc.X;
    uint16_t y0 = loc.Y;
    uint16_t x1 = x0 + sz.Width - 1;
    uint16_t y1 = y0 + sz.Height - 1;

    // Рисуем 4 линии через fillRect (которая сделает клиппинг)
    TFT_matrix_fillRect((Rect){{x0, y0}, {sz.Width, 1}}, ForeGround); // Верх
    TFT_matrix_fillRect((Rect){{x0, y1}, {sz.Width, 1}}, ForeGround); // Низ
    TFT_matrix_fillRect((Rect){{x0, y0}, {1, sz.Height}}, ForeGround); // Лево
    TFT_matrix_fillRect((Rect){{x1, y0}, {1, sz.Height}}, ForeGround); // Право
}
// ------------------------------------------------------------------------------------------------
// Рамка в один пиксель по периметру области отрисовки
void TFT_matrix_client_rect() {

    Rect cli_r = TFT_matrix_get_clientrect();

    uint16_t x = cli_r.Location.X;
    uint16_t y = cli_r.Location.Y;
    uint16_t w = cli_r.ClientSize.Width;
    uint16_t h = cli_r.ClientSize.Height;

    // Рисуем рамку
    // Тогда fillRect увидит, что x >= clip_left (3 >= 3) и пропустит линию.
    TFT_matrix_fillRect((Rect){{x, y}, {w, 1}}, ForeGround);         // Верх
    TFT_matrix_fillRect((Rect){{x, y}, {1, h}}, ForeGround);         // Лево
    TFT_matrix_fillRect((Rect){{x, y + h - 1}, {w, 1}}, ForeGround); // Низ
    TFT_matrix_fillRect((Rect){{x + w - 1, y}, {1, h}}, ForeGround); // Право
}
// ------------------------------------------------------------------------------------------------
// Построение заштрихованного прямоугольника (горизонтальные 1-пиксельной толщины линии)
void TFT_matrix_hatching_rect (Point data, Size size) {
    // --------------------------------------------------------------------------------------------
    Point data2, data3;

    data2 = data;
    data3.X = data.X + size.Width;
    data3.Y = data.Y;

    uint8_t tackt_f = 0;
    uint8_t step_f = (data.extend > 10 || data.extend < 2) ? 2 : data.extend;

    uint16_t f_color = TFT_matrix_getBrush();
    uint16_t b_color = TFT_matrix_getBackColor();
    // --------------------------------------------------------------------------------------------
    for (int i = 0; i < size.Height; i++) {

        tackt_f += 1;

        if (tackt_f > step_f) {

            tackt_f = 0;
            TFT_matrix_setBrush(b_color);
        } else {

            TFT_matrix_setBrush(f_color);
        }

        TFT_matrix_line2 (data2, data3);

        data2.Y += 1;
        data3.Y += 1;
    }
    // --------------------------------------------------------------------------------------------
    TFT_matrix_setBrush(f_color);
    // --------------------------------------------------------------------------------------------
}
// ------------------------------------------------------------------------------------------------
// Сдвиг указанных координат на указанный диапазон
Point TFT_matrix_shift_coords (Point data, int16_t X, int16_t Y) {

    Point result;

    result.X = data.X + X;
    result.Y = data.Y + Y;

    return result;
}
// ------------------------------------------------------------------------------------------------
// Получение координат верхнего правого угла прямоугольной области
Point TFT_matrix_get_rect_topright (Rect rect) {

    Point result;

    result.X = rect.Location.X + rect.ClientSize.Width - 1;
    result.Y = rect.Location.Y;

    return result;
}
// ------------------------------------------------------------------------------------------------
// Получение координат верхнего правого угла прямоугольной области
Point TFT_matrix_get_rect_bottomright (Rect rect) {

    Point result;

    result.X = rect.Location.X + rect.ClientSize.Width;
    result.Y = rect.Location.Y + rect.ClientSize.Height;

    return result;
}
// ------------------------------------------------------------------------------------------------
// Получение координат верхнего правого угла прямоугольной области
Point TFT_matrix_get_rect_bottomleft (Rect rect) {

    Point result;

    result.X = rect.Location.X;
    result.Y = rect.Location.Y + rect.ClientSize.Height;

    return result;
}
// ------------------------------------------------------------------------------------------------
// Пропорционально уменьшить прямоугольную область
Rect TFT_matrix_reduce_rect (Rect rect, uint8_t value) {

    Rect result;

    result.Location.X = rect.Location.X + value;
    result.Location.Y = rect.Location.Y + value;
    result.ClientSize.Width = rect.ClientSize.Width - (value * 2);
    result.ClientSize.Height = rect.ClientSize.Height - (value * 2);

    return result;
}
// ------------------------------------------------------------------------------------------------
// Пропорционально уменьшить прямоугольную область на один пиксель
Rect TFT_matrix_reduce_rect2 (Rect rect) {

    return TFT_matrix_reduce_rect (rect, 1);
}
// ------------------------------------------------------------------------------------------------
// Пропорционально увеличить прямоугольную область
Rect TFT_matrix_increase_rect (Rect rect, uint8_t value) {

    Rect result;

    result.Location.X = rect.Location.X - value;
    result.Location.Y = rect.Location.Y - value;
    result.ClientSize.Width = rect.ClientSize.Width + (value * 2);
    result.ClientSize.Height = rect.ClientSize.Height + (value * 2);

    return result;
}
// ------------------------------------------------------------------------------------------------
// Вписать прямоугольник area в центр owner
Rect TFT_matrix_center_rect(Rect owner, Rect area) {

    if (area.ClientSize.Width <= owner.ClientSize.Width && area.ClientSize.Height <= owner.ClientSize.Height) {

        uint16_t delta_w = ((owner.ClientSize.Width - area.ClientSize.Width) / 2);
        uint16_t delta_h = ((owner.ClientSize.Height - area.ClientSize.Height) / 2);

        Rect result = {
            {owner.Location.X + delta_w, owner.Location.Y + delta_h},
            {area.ClientSize.Width, area.ClientSize.Height}
        };

        return result;
    }

    return owner;
}
// ------------------------------------------------------------------------------------------------
// Пропорционально уменьшить только высоту указанной области
Rect TFT_matrix_reduce_rect_height(Rect area, uint16_t delta) {

    //if (area.ClientSize.Height < delta) { return area; }

    uint16_t delta_h = ((area.ClientSize.Height - delta) / 2);

    Rect result = {
            {area.Location.X, area.Location.Y + delta_h},
            {area.ClientSize.Width, area.ClientSize.Height - delta_h * 2}
        };

    return result;
}
// ------------------------------------------------------------------------------------------------
// Установка каретки для вывода горизонтальной строки по центру указанной области
void TFT_matrix_centerCaret (Rect area, uint8_t *str) {

    Size str_size = TFT_matrix_measureString(str);

    // 2. Находим координаты левого верхнего угла текста внутри area
    // Формула: Старт + (Свободное место / 2)
    current_text_context.x_caret = area.Location.X + (area.ClientSize.Width - str_size.Width) / 2;
    current_text_context.y_caret = area.Location.Y + (area.ClientSize.Height - str_size.Height) / 2;
}
// ------------------------------------------------------------------------------------------------
// Проверка, является ли указанная область пустой
uint8_t TFT_matrix_matrix_rect_isEmpty (Rect area) {

    return (area.ClientSize.Height == 0 && area.ClientSize.Width == 0);
}
// ------------------------------------------------------------------------------------------------
// Получение координат углов указанной области
Point TFT_matrix_get_corner (Rect area, Corner subject) {

    if (TFT_matrix_matrix_rect_isEmpty (area)) {
        return (Point){0, 0};
    }

    int dx = (subject == TopRight || subject == BottomRight) ? area.ClientSize.Width : 0;
    int dy = (subject == BottomLeft || subject == BottomRight) ? area.ClientSize.Height : 0;

    return (Point){area.Location.X + dx, area.Location.Y + dy};
}
// ------------------------------------------------------------------------------------------------
// Вписать область в область с центровкой, вписываемая область должна быть заведомо меньше
Rect TFT_matrix_inscribeArea (Rect area_owner, Rect area_slave) {

    // 1. Приводим размеры к знаковому типу int32_t, чтобы разность могла быть отрицательной
    // 2. Считаем смещение (padding)
    int32_t diffW = (int32_t)area_owner.ClientSize.Width - (int32_t)area_slave.ClientSize.Width;
    int32_t diffH = (int32_t)area_owner.ClientSize.Height - (int32_t)area_slave.ClientSize.Height;

    // 3. Вычисляем новые координаты.
    // Деление на 2 в C для отрицательных чисел (например, -5 / 2 = -2)
    // работает корректно для центровки "в обе стороны".
    area_slave.Location.X = (int16_t)(area_owner.Location.X + (diffW / 2));
    area_slave.Location.Y = (int16_t)(area_owner.Location.Y + (diffH / 2));

    return area_slave;
}
// ------------------------------------------------------------------------------------------------
// Вычисление области пересечения двух областей
Rect TFT_matrix_rect_intersect (Rect r1, Rect r2) {

    Rect result = {0};

    // Находим границы пересечения
    int32_t x1 = MAX (r1.Location.X, r2.Location.X);
    int32_t y1 = MAX (r1.Location.Y, r2.Location.Y);
    int32_t x2 = MIN (r1.Location.X + r1.ClientSize.Width, r2.Location.X + r2.ClientSize.Width);
    int32_t y2 = MIN (r1.Location.Y + r1.ClientSize.Height, r2.Location.Y + r2.ClientSize.Height);

    // Проверяем, есть ли реальное пересечение
    if (x2 > x1 && y2 > y1) {
        result.Location.X = (uint16_t)x1;
        result.Location.Y = (uint16_t)y1;
        result.ClientSize.Width = (uint16_t)(x2 - x1);
        result.ClientSize.Height = (uint16_t)(y2 - y1);
    }
    // Если пересечения нет, вернется Rect с нулевыми размерами (инициализирован выше)

    return result;
}
// ------------------------------------------------------------------------------------------------
// Метод печати строки стандартным шрифтом в центр указанной области с центровкой и обрезкой (ellipsis) по границам области
void TFT_matrix_drawStringInscribed_Smart(uint8_t *str, Rect area_owner) {

    if (!str || *str == '\0')
        return;

    Size strSize = TFT_matrix_measureString (str);
    Rect old_clip = client_rect;

    // Сохраняем текущий режим переноса, чтобы не испортить глобальные настройки
    uint8_t old_wrap = current_text_context.wrap_mode;
    TFT_matrix_set_textContext_prop (TX_Wrap, WM_None);

    int32_t ownerW = area_owner.ClientSize.Width;
    int32_t ownerH = area_owner.ClientSize.Height;
    int32_t strLen = strSize.Width;
    int32_t strThick = strSize.Height;

    TFT_matrix_set_clientrect (TFT_matrix_rect_intersect (old_clip, area_owner));

    if (current_text_context.direction == WS_Horizonlal) {
        current_text_context.y_caret = area_owner.Location.Y + (int16_t)((ownerH - strThick + 1) / 2);

        if (strLen <= ownerW) {
            // Внедряем выбор X в зависимости от Align
            if (current_text_context.align == WA_Center) {
                current_text_context.x_caret = area_owner.Location.X + (int16_t)((ownerW - strLen + 1) / 2);
            } else if (current_text_context.align == WA_Right) {
                current_text_context.x_caret = area_owner.Location.X + (int16_t)(ownerW - strLen);
            } else {  // WA_Left
                current_text_context.x_caret = area_owner.Location.X;
            }
            TFT_matrix_writeString (str);
        } else {
            // Логика Ellipsis (для длинных строк Right обычно игнорируют, чтобы видеть начало)
            Size dotSize = TFT_matrix_measureString ((uint8_t *)"...");
            current_text_context.x_caret = area_owner.Location.X;
            Rect text_clip = area_owner;
            text_clip.ClientSize.Width -= dotSize.Width;
            TFT_matrix_set_clientrect (TFT_matrix_rect_intersect (old_clip, text_clip));
            TFT_matrix_writeString (str);
            TFT_matrix_set_clientrect (TFT_matrix_rect_intersect (old_clip, area_owner));
            current_text_context.x_caret = area_owner.Location.X + ownerW - dotSize.Width;
            TFT_matrix_writeString ((uint8_t *)"...");
        }
    } else {  // --- ВЕРТИКАЛЬНЫЙ РЕЖИМ (сверху вниз!) ---
        current_text_context.x_caret = area_owner.Location.X + (int16_t)((ownerW - strThick + 1) / 2);

        if (strLen <= ownerH) {
            if (current_text_context.align == WA_Center) {
                current_text_context.y_caret = area_owner.Location.Y + (int16_t)((ownerH - strLen + 1) / 2);
            } else if (current_text_context.align == WA_Right) {
                // Для печати ВВЕРХ "прижать к верху области" — это закончить на Y_start
                current_text_context.y_caret = area_owner.Location.Y + (int16_t)(ownerH - strLen);
            } else {  // WA_Left (прижать к низу)
                current_text_context.y_caret = area_owner.Location.Y;
            }
            TFT_matrix_writeString (str);
        } else {
            // Ellipsis для вертикали
            // --- ВЕТКА ДЛИННОЙ СТРОКИ (Ellipsis) ---
            Size dotSize = TFT_matrix_measureString ((uint8_t *)"...");

            // 1. Начинаем СВЕРХУ области (а не снизу!)
            current_text_context.y_caret = area_owner.Location.Y;

            Rect text_clip = area_owner;
            // 2. Обрезаем область СНИЗУ (минус высота точек)
            // Было: text_clip.Location.Y += dotSize.Width; (это обрезало верх)
            text_clip.ClientSize.Height -= dotSize.Width;

            // 3. Убеждаемся, что перенос выключен, чтобы не уйти в бок
            uint8_t old_wrap = current_text_context.wrap_mode;
            TFT_matrix_set_textContext_prop (TX_Wrap, WM_None);

            TFT_matrix_set_clientrect (TFT_matrix_rect_intersect (old_clip, text_clip));
            TFT_matrix_writeString (str);  // Теперь текст пойдет сверху вниз до обрезки

            // 4. Рисуем точки в самом НИЗУ области
            TFT_matrix_set_clientrect (TFT_matrix_rect_intersect (old_clip, area_owner));
            current_text_context.y_caret = area_owner.Location.Y + ownerH - dotSize.Width;
            TFT_matrix_writeString ((uint8_t *)"...");
            TFT_matrix_set_textContext_prop (TX_Wrap, old_wrap);
        }
    }

    // В конце функции возвращаем режим на место
    TFT_matrix_set_textContext_prop (TX_Wrap, old_wrap);
    TFT_matrix_set_clientrect (old_clip);
}
// ------------------------------------------------------------------------------------------------
// Метод печати GFX шрифтами из библиотеки Adafruit
void TFT_matrix_drawString_GFX2X(const uint8_t *str, const GFXfont *font, float line_spacing) {

    if (str == NULL || font == NULL)
        return;

    uint8_t size = current_text_context.scale;
    if (size == 0)
        size = 1;

    // 1. Считаем фиксированный шаг строки (высота * масштаб * интервал)
    uint16_t line_height = (uint16_t)(font->yAdvance * size * line_spacing);

    // 2. ВАЖНО: Находим "базовое смещение".
    // GFX шрифты рисуются ВВЕРХ от базовой линии.
    // Чтобы y_caret был ВЕРХНИМ краем, нам нужно сдвинуть базовую линию вниз на высоту заглавных букв.
    // Обычно это модуль самого большого отрицательного yOffset (приблизительно font->yAdvance)
    int16_t baseline_offset = (int16_t)(font->yAdvance * size * 0.8f);  // 0.8 - средний коэффициент для GFX

    while (*str) {
        uint8_t c = (uint8_t)(*str++ - 0x20);

        if (c + 0x20 == '\n') {
            current_text_context.x_caret = 0;
            current_text_context.y_caret += line_height;
            continue;
        }

        if (c > (font->last - font->first))
            continue;

        GFXglyph *glyph = &(font->glyph[c]);
        uint8_t *bitmap = font->bitmap;
        uint16_t w = glyph->width, h = glyph->height;
        uint16_t x_advance = glyph->xAdvance * size;

        // Автоперенос
        if (current_text_context.x_caret + x_advance > TFT_DISPLAY_WIDTH) {
            current_text_context.x_caret = 0;
            current_text_context.y_caret += line_height;
        }

        if (w == 0 || h == 0) {
            current_text_context.x_caret += (x_advance + current_text_context.space);
            continue;
        }

        // РАСЧЕТ КООРДИНАТ ОКНА (с защитой от 0,0)
        // Мы берем y_caret и добавляем компенсацию, чтобы y_caret был ВЕРХОМ строки
        int16_t x_start = current_text_context.x_caret + (glyph->xOffset * size);
        int16_t y_start = current_text_context.y_caret + (glyph->yOffset * size) + baseline_offset;

        // Защита от отрицательных координат (главная причина сброса в 0,0)
        if (x_start < 0)
            x_start = 0;
        if (y_start < 0)
            y_start = 0;
        if (x_start >= TFT_DISPLAY_WIDTH || y_start >= TFT_DISPLAY_HEIGHT) {
            current_text_context.x_caret += (x_advance + current_text_context.space);
            continue;
        }

        ST7789_SetAddressWindow (x_start, y_start, x_start + (w * size) - 1, y_start + (h * size) - 1);
        ST7789_OpenSession();

        uint16_t fr_c = TFT_matrix_getBrush();
        uint16_t bk_c = TFT_matrix_getBackColor();

        uint32_t bo = glyph->bitmapOffset;
        uint8_t bits = 0, bit_count = 0;

        for (uint16_t yy = 0; yy < h; yy++) {
            uint8_t line_data[w];  // Буфер строки
            for (uint16_t xx = 0; xx < w; xx++) {
                if (bit_count == 0) {
                    bits = bitmap[bo++];
                    bit_count = 8;
                }
                line_data[xx] = (bits & 0x80);
                bits <<= 1;
                bit_count--;
            }

            for (uint8_t sy = 0; sy < size; sy++) {
                for (uint16_t xx = 0; xx < w; xx++) {
                    uint16_t color = line_data[xx] ? fr_c : bk_c;
                    for (uint8_t sx = 0; sx < size; sx++) { ST7789_SendColor (color); }
                }
            }
        }

        ST7789_CloseSession();
        current_text_context.x_caret += (x_advance + current_text_context.space);
    }
}
// ------------------------------------------------------------------------------------------------
// Печать реалистичных 7SEG символов с "выключенными" сегментами (BIG_7SEG_DIGIT.h)
void TFT_matrix_draw_7seg_BIN (uint8_t sym_code, const BINFont *font) {

    if (font == NULL || font->data == NULL)
        return;

    uint8_t size = current_text_context.scale;
    if (size == 0)
        size = 1;

    // Параметры из структуры шрифта
    uint16_t w = font->width;
    uint16_t h = font->height;
    uint16_t g_size = font->glyph_size;
    uint16_t bytesPerRow = (w + 7) / 8;

    // Индексы: текущий символ и "восьмерка" (ASCII '8' это 0x38, но в твоем массиве это индекс 8)
    uint16_t curr_idx = (sym_code < font->total_chars) ? sym_code : 0;
    uint16_t eight_idx = 8;  // Предполагаем, что 8-ка всегда по индексу 8

    // 1. Вычисляем итоговые размеры
    uint16_t full_w = (uint16_t)w * size;
    uint16_t full_h = (uint16_t)h * size;

    // 2. ПРОВЕРКА ГОРИЗОНТАЛИ: Влезет ли символ?
    if (current_text_context.x_caret + full_w > TFT_DISPLAY_WIDTH) {
        current_text_context.x_caret = 0;  // Возврат каретки
        // Сдвиг Y (высота символа + межстрочный интервал)
        current_text_context.y_caret += (full_h + 4);
    }

    // 3. ПРОВЕРКА ВЕРТИКАЛИ: Если совсем не влезло (низ экрана)
    if (current_text_context.y_caret >= TFT_DISPLAY_HEIGHT)
        return;

    // Теперь берем актуальные координаты
    uint16_t start_x = current_text_context.x_caret;
    uint16_t start_y = current_text_context.y_caret;

    // Теперь вызываем окно — оно гарантированно в границах памяти дисплея
    ST7789_SetAddressWindow (start_x, start_y,
                             start_x + full_w - 1,
                             start_y + ((start_y + full_h > TFT_DISPLAY_HEIGHT) ? (TFT_DISPLAY_HEIGHT - start_y - 1) : (full_h - 1)));

    uint16_t f_c = TFT_matrix_getBrush();
    uint16_t b_c = TFT_matrix_getBackColor();
    uint16_t d_c = CL_DARKENGREEN;  // Цвет "погашенного" сегмента

    ST7789_OpenSession();

    for (uint16_t y = 0; y < h; y++) {
        for (uint8_t sy = 0; sy < size; sy++) {  // Масштаб Y

            for (uint16_t b = 0; b < bytesPerRow; b++) {
                
                // Смещение в массиве для текущей строки
                uint32_t offset = (y * bytesPerRow) + b;
                uint8_t val_curr = font->data[(curr_idx * g_size) + offset];
                uint8_t val_eight = font->data[(eight_idx * g_size) + offset];

                for (int8_t bit = 7; bit >= 0; bit--) {
                    uint16_t cur_x = (b * 8) + (7 - bit);
                    if (cur_x < w) {
                        uint16_t color;

                        if (val_curr & (1 << bit)) {
                            color = f_c;  // Активный сегмент
                        } else if (val_eight & (1 << bit)) {
                            color = d_c;  // Подложка (сегмент выключен)
                        } else {
                            color = b_c;  // Фон индикатора
                        }

                        for (uint8_t sx = 0; sx < size; sx++) {
                            ST7789_SendColor (color);  // Масштаб X
                        }
                    }
                }
            }
        }
    }

    ST7789_CloseSession();
    current_text_context.x_caret += (w * size + current_text_context.space);
}
// ------------------------------------------------------------------------------------------------
// Печать значений 7 сегментными шрифтами
// scale = [1..3]
// допустимые символы [0..9] и "точка"
void TFT_matrix_draw_7seg_str (uint8_t *str, const BINFont *font) {

    while (*str) {

        //printf ("%02x\r\n", *str);
        uint8_t sym_code = *str++;

        if (sym_code == 0x2e) {

            TFT_matrix_pushCaret();

            switch (current_text_context.scale) {

            case 3:

                TFT_matrix_addCaretY (123);
                break;

            case 2:

                TFT_matrix_addCaretY (50 * 2 - 18);
                break;

            default:

                TFT_matrix_addCaretY (50 - 9);
                break;
            }

            TFT_matrix_writeChar_Scaled (0xE8);

            TFT_matrix_popCaret();

            switch (current_text_context.scale) {

            case 3:

                TFT_matrix_addCaretX (23);
                break;

            case 2:

                TFT_matrix_addCaretX (16);
                break;

            default:

                TFT_matrix_addCaretX (9);
                break;
            }

            continue;
        }

        sym_code -= 0x30;

        if (sym_code > 9) { continue; }

        TFT_matrix_draw_7seg_BIN (sym_code, font);
    }
}
// ------------------------------------------------------------------------------------------------
// Рисование индикатора прогресса
void TFT_matrix_drawProgressBar (ProgressBar *pb, uint8_t newValue) {
    if (newValue > 100)
        newValue = 100;

    // 1. Отрисовка рамки (только при инициализации)
    if (pb->PrevValue == 0xFF) {
        ST7789_setForeColor (pb->BorderColor);  // Устанавливаем цвет рамки
        TFT_matrix_draw_rect (pb->Location, pb->ClientSize);
    }

    // 2. Внутренние габариты (минус 2 пикселя на линии рамки)
    uint16_t innerWidth = pb->ClientSize.Width - 2;
    uint16_t innerHeight = pb->ClientSize.Height - 2;
    uint16_t startX = pb->Location.X + 1;
    uint16_t startY = pb->Location.Y + 1;

    // 3. Считаем ширину заполнения
    uint16_t pos = (uint32_t)innerWidth * newValue / 100;

    // 4. Рисуем ЗАПОЛНЕННУЮ часть
    if (pos > 0) {
        ST7789_setForeColor (pb->FillColor);  // Устанавливаем цвет прогресса в Fore
        TFT_matrix_fillRect ((Rect){
                             {startX, startY     },
                             {pos,    innerHeight}
        },
                         ForeGround);
    }

    // 5. Затираем ОСТАВШУЮСЯ часть (фон внутри прогресс-бара)
    if (pos < innerWidth) {
        ST7789_setForeColor (pb->BackColor);  // Устанавливаем цвет фона в Fore
        TFT_matrix_fillRect ((Rect){
                             {startX + pos,     startY     },
                             {innerWidth - pos, innerHeight}
        },
                         ForeGround);
    }

    pb->PrevValue = newValue;
}
// ------------------------------------------------------------------------------------------------
// Вывод RAW форматированных бинарных массивов
void TFT_matrix_draw_bin_data (Rect area, const uint8_t *source) {

    uint16_t x_current = area.Location.X;
    uint16_t y_current = area.Location.Y;

    uint16_t f_color = TFT_matrix_getBrush();
    uint16_t b_color = TFT_matrix_getBackColor();

    // 1. Проходим по исходным пикселям
    for (uint16_t row = 0; row < area.ClientSize.Height; row++) {
        x_current = area.Location.X;

        for (uint16_t col = 0; col < area.ClientSize.Width; col++) {

            uint16_t current_data_index = (row * area.ClientSize.Width) + col;

            // Раскладываем байт на биты MSB
            for (int8_t dot_index = 7; dot_index >= 0; dot_index--) {

                // ПРОВЕРКА CLIPRECT
                if (TFT_matrix_point_in_cliprect (x_current, y_current)) {
                    ST7789_SetAddressWindow (x_current, y_current, x_current, y_current);

                    ST7789_OpenSession();

                    if (is_set_idx(source[current_data_index], dot_index)) {

                        ST7789_SendColor (f_color);
                    } else {

                        ST7789_SendColor (b_color);
                    }

                    ST7789_CloseSession();
                }

                x_current += 1;
            }
        }

        y_current += 1;
    }
}
// ------------------------------------------------------------------------------------------------
/* // Вывод 7Seg шрифтов с подложкой из неактивных символов
// для шрифта 32x50 единым массивом, символ = 200 байт
// Внимание ! Ссылка дается на весь массив символов, а не на конкретный символ!
void TFT_matrix_draw_led_font (uint8_t sym_code, Point coords, const uint8_t *font_data) {

    static const uint8_t SEG_SYM_WIDTH = (32 / 8);
    static const uint8_t SEG_SYM_HEIGHT = 50;

    uint8_t real_sym_code = (sym_code < 10) ? sym_code : 9;

    uint16_t x_current = coords.X;
    uint16_t y_current = coords.Y;

    uint16_t f_color = TFT_matrix_getBrush();
    uint16_t b_color = TFT_matrix_getBackColor();

    //uint16_t CL_BG = ST7789_DarkenColor(CL_GREEN, 3);

    // 1. Проходим по исходным пикселям
    for (uint16_t row = 0; row < SEG_SYM_HEIGHT; row++) {
        x_current = coords.X;

        for (uint16_t col = 0; col < SEG_SYM_WIDTH; col++) {

            uint16_t current_data_index = (real_sym_code * 200) + (((row * SEG_SYM_WIDTH) + col));
            uint16_t back_data_index = (8 * 200) + ((row * SEG_SYM_WIDTH) + col);

            // Раскладываем байт на биты MSB
            for (int8_t dot_index = 7; dot_index >= 0; dot_index--) {

                // ПРОВЕРКА CLIPRECT
                if (TFT_matrix_point_in_cliprect (x_current, y_current)) {
                    ST7789_SetAddressWindow (x_current, y_current, x_current, y_current);

                    uint8_t bg_present = is_set_idx(font_data[back_data_index], dot_index);

                    ST7789_OpenSession();

                    if (is_set_idx(font_data[current_data_index], dot_index)) {

                        ST7789_SendColor (f_color);
                    } else {

                        if (bg_present) {

                            ST7789_SendColor (CL_DARKENGREEN);
                        } else {

                            ST7789_SendColor (b_color);
                        }
                    }

                    ST7789_CloseSession();
                }

                x_current += 1;
            }
        }

        y_current += 1;
    }
} */
// ------------------------------------------------------------------------------------------------
// Рисование масштабируемой иконки из бинарного потока, размеры в первых 4-байтах массива (из флеш памяти)
void TFT_matrix_draw_bitmap(const uint8_t *source) {

    if (current_text_context.scale < 1 && current_text_context.scale > 5)
        current_text_context.scale = 1;

    // 1. Извлекаем ширину и высоту (из первых 4 байт)
    Int16x coord;
    coord.cvalue[0] = source[0];
    coord.cvalue[1] = source[1];
    uint16_t w = coord.ivalue;

    coord.cvalue[0] = source[2];
    coord.cvalue[1] = source[3];
    uint16_t h = coord.ivalue;

    // 2. Проходим по исходным пикселям иконки
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            
            // Рассчитываем координаты начала "увеличенного" пикселя на экране
            uint16_t screen_x = current_text_context.x_caret + (col * current_text_context.scale);
            uint16_t screen_y = current_text_context.y_caret + (row * current_text_context.scale);

            // 3. Быстрая проверка: если начало квадрата вне clipRect,
            // можно либо пропустить, либо проверять каждую точку внутри (для точности)
            
            // Получаем цвет (RGB565 - 2 байта)
            uint32_t pixel_idx = 4 + (row * w + col) * 2;
            uint8_t b1 = source[pixel_idx];
            uint8_t b2 = source[pixel_idx + 1];

            // 4. Отрисовка внутреннего квадрата scale x scale
            for (uint8_t sy = 0; sy < current_text_context.scale; sy++) {

                for (uint8_t sx = 0; sx < current_text_context.scale; sx++) {

                    uint16_t curr_x = screen_x + sx;
                    uint16_t curr_y = screen_y + sy;

                    // ПРОВЕРКА CLIPRECT
                    if (TFT_matrix_point_in_cliprect(curr_x, curr_y)) {

                        ST7789_SetAddressWindow(curr_x, curr_y, curr_x, curr_y);
                        ST7789_OpenSession();
                        ST7789_SendByte(b1);
                        ST7789_SendByte(b2);
                        ST7789_CloseSession();
                    }
                }
            }
        }
    }
}
// ------------------------------------------------------------------------------------------------
