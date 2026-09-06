/********************************** (C) COPYRIGHT ******************************
 * File Name          : matrix_engine.c
 * Author             : vantr
 * Version            : V1.0.1
 * Date               : 2026/02/08
 * Description        : Библиотека графики для OLED дисплея на SSD1306
 * Module             : Графическая библиотека MATRIX™ (CH32X033)
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе. Допускается к использованию.
 *******************************************************************************/
// ------------------------------------------------------------------------------------------------
#include "fonts.h"
#include "oled_spi_matrix_engine.h"
#include "SSS_Common_Lib_V1/sss_classes.h"
#include "OLED_SPI_SSD1306.h"

#include "SSS_Common_Lib_V1/matrix_cyrrilic.h"

#include <stdlib.h>
#include "debug.h"
// ------------------------------------------------------------------------------------------------
// Экранный буфер - 16x64 (128x32 физически)
uint8_t OLED__screen_buffer[matrix_HARDWARE_BUFFER_SIZE];
// ------------------------------------------------------------------------------------------------
// Контекст управления выводом стандартного текста
TextContext3 current_text_context;
// ------------------------------------------------------------------------------------------------
// Область отрисовки
Rect OLED_client_rect = {
    {0,                 0                 },
    {DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1}
};
// ------------------------------------------------------------------------------------------------
// Установка флага прозрачности заднего плана
void OLED_SPI_matrix_set_bg_transparent(uint8_t value) {
    
    current_text_context.x_bg_transparent = (value == 0) ? 0 : 1;
}

// ------------------------------------------------------------------------------------------------
// Установка свойств контекста вывода строк стандартным шрифтом
void OLED_SPI_matrix_set_textContext_prop (TextContextTypes3 prop_name, int16_t value) {

    switch (prop_name) {

    case TX_Scale3:

        current_text_context.scale = (uint8_t)((value > 3) ? 3 : value);
        break;

    case TX_Direction3:

        current_text_context.direction = (WriteString_DirectionType)value;
        break;

    case TX_Space3:

        current_text_context.space = value;
        break;

    case TX_Wrap3:

        current_text_context.wrap_mode = (WriteString_WrapMode)value;
        break;

    case TX_Align3:

        current_text_context.align = (WriteString_Align)value;
        break;

    case TX_X_Caret3:

        current_text_context.x_caret = value;
        break;

    case TX_Y_Caret3:

        current_text_context.y_caret = value;
        break;

    case TX_Inverse3:

        current_text_context.inverse = value;
        break;

    case TX_Brush3:

        current_text_context.x_brush = value;
        break;
    }
}
// ------------------------------------------------------------------------------------------------
// Рисование или гашение пикселя в указанной позиции буфера экрана [32x8], не обновляет экран
void OLED_SPI_matrix_setPixel(uint8_t X, uint8_t Y, uint8_t Value) {
    // --------------------------------------------------------------------------------------------
    if (OLED_SPI_matrix_point_in_cliprect(X, Y) == 0) { return; }
    // --------------------------------------------------------------------------------------------
    /* ----------------------------------
     * | x_brush | transp | in   | out  |
     * ----------------------------------
     * |    1    |   0    |  0   |  0   |
     * |    1    |   0    |  1   |  1   |
     * ----------------------------------
     * |    1    |   1    |  0   |  -   |
     * |    1    |   1    |  1   |  1   |
     * ----------------------------------
     * |    0    |   0    |  0   |  1   |
     * |    0    |   0    |  1   |  0   |
     * ----------------------------------
     * |    0    |   1    |  0   |  -   |
     * |    0    |   1    |  1   |  1   |
     * ----------------------------------
     */
    // --------------------------------------------------------------------------------------------
    uint8_t b_y = Y / 8;
    //uint8_t p_y = 7 - (Y % 8); // Индекс бита в байте
    uint8_t p_y = (Y % 8); // Индекс бита в байте
    uint16_t B_real = X + (b_y * 128); // Номер байта в буфере "OLED__screen_buffer"
    // --------------------------------------------------------------------------------------------
    uint8_t cur_brush = Value;
    
    if (current_text_context.x_brush == 0) {
        
        switch(current_text_context.x_bg_transparent) {
            
            case 0:
                
                cur_brush = (Value == 0) ? 1 : 0;
                break;
                
            default:
                
                if (Value == 0) { return; }
                
                cur_brush = 1;
                break;
        }
    } else {

        switch (current_text_context.x_bg_transparent) {

        case 0:

            cur_brush = (Value == 0) ? 0 : 1;
            break;

        default:

            if (Value == 0) {
                return;
            }

            cur_brush = 1;
            break;
        }
    }
    // --------------------------------------------------------------------------------------------
    if (cur_brush == 0) {
        // Задний план
        cbi(OLED__screen_buffer[B_real], p_y);
    } else {
        // Передний план
        sbi(OLED__screen_buffer[B_real], p_y);
    }
    // --------------------------------------------------------------------------------------------
}
// ------------------------------------------------------------------------------------------------
// Возврат каретки в начальную, нулевую позицию
void OLED_SPI_matrix_returnCaret() {

    current_text_context.x_caret = 0;
    current_text_context.y_caret = 0;
}
// ------------------------------------------------------------------------------------------------
// Установка каретки в любое указанное положение [-127...127]
void OLED_SPI_matrix_setCaret(int16_t value) {

    current_text_context.x_caret = value;
}
// ------------------------------------------------------------------------------------------------
// Установка отступа по вертикали
void OLED_SPI_matrix_setMargin(int16_t value) {

    current_text_context.y_caret = value;
}
// ------------------------------------------------------------------------------------------------
// Установка каретки в указанные координаты матрицы
void OLED_SPI_matrix_setCoord(int16_t X, int16_t Y) {

    current_text_context.x_caret = X;
    current_text_context.y_caret = Y;
}
// ------------------------------------------------------------------------------------------------
// Установка каретки в указанные координаты матрицы
void OLED_SPI_matrix_setCoord2(Point data) {

    current_text_context.x_caret = data.X;
    current_text_context.y_caret = data.Y;
}
// ------------------------------------------------------------------------------------------------
// Сдвиг каретки на указанное количество пикселей [-127...127]
void OLED_SPI_matrix_addCaret(int16_t value) {

    current_text_context.x_caret += value;
}
// ------------------------------------------------------------------------------------------------
// Установить цвет переднего плана
void OLED_SPI_matrix_setBrush(uint8_t color) {

    current_text_context.x_brush = (color == 0) ? 0 : 1;
}
// ------------------------------------------------------------------------------------------------
// Посмотреть цвет переднего плана
uint8_t OLED_SPI_matrix_getBrush() {

    return current_text_context.x_brush;
}
// ------------------------------------------------------------------------------------------------
// Посмотреть цвет заднего плана (инверсный цвету переднего плана)
uint8_t OLED_SPI_matrix_getBackColor() {

    return (current_text_context.x_brush == 0) ? 1 : 0;
}
// ------------------------------------------------------------------------------------------------
// Инверсия содержимого экрана
void OLED_SPI_matrix_inverseScreen() {

    for (unsigned int i = 0; i < sizeof (OLED__screen_buffer); i++) {
        OLED__screen_buffer[i] = ~OLED__screen_buffer[i];
    }
}
// ------------------------------------------------------------------------------------------------
// Создание точки из координат
Point OLED_SPI_matrix_create_point(uint8_t X, uint8_t Y) {
    
    Point result;
    
    result.X = X;
    result.Y = Y;
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Создание структуры размера из данных
Size OLED_SPI_matrix_create_size(uint8_t width, uint8_t height) {
    
    Size result;
    
    result.Width = width;
    result.Height = height;
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Создание прямоугольной области
Rect OLED_SPI_matrix_create_rect(Point coords, Size client_size) {
    
    Rect result;
    
    result.Location = coords;
    result.ClientSize = client_size;
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Создание прямоугольной области
Rect OLED_SPI_matrix_create_rect2(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height) {
    
    Rect result;
    
    result.Location = OLED_SPI_matrix_create_point(X, Y);
    result.ClientSize = OLED_SPI_matrix_create_size(Width, Height);
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Проверка, попадает ли указанная точка в указанную область
uint8_t OLED_SPI_matrix_point_in_rect(Point data, Rect rect) {
    
    if (data.X >= rect.Location.X && data.X <= rect.Location.X + rect.ClientSize.Width) {
        
        if (data.Y >= rect.Location.Y && data.Y <= rect.Location.Y + rect.ClientSize.Height) {
            
            return 1;
        }
    }
    
    return 0;
}
// ------------------------------------------------------------------------------------------------
// Проверка, попадает ли указанная точка в отрисовываемую область экрана
uint8_t OLED_SPI_matrix_point_in_cliprect(uint8_t X, uint8_t Y) {
    
    if (X >= OLED_client_rect.Location.X && X <= OLED_client_rect.Location.X + OLED_client_rect.ClientSize.Width) {
        
        if (Y >= OLED_client_rect.Location.Y && Y <= OLED_client_rect.Location.Y + OLED_client_rect.ClientSize.Height) {
            
            return 1;
        }
    }
    
    return 0;
}
// ------------------------------------------------------------------------------------------------
// Установка области отрисовки дисплея
void OLED_SPI_matrix_set_clientrect(Rect value) {
    
    Point coords;    
    coords.X = (value.Location.X < DISPLAY_WIDTH) ? value.Location.X : DISPLAY_WIDTH - 1;
    coords.Y = (value.Location.Y < DISPLAY_HEIGHT) ? value.Location.Y : DISPLAY_HEIGHT - 1;
    
    Size size;
    size.Width = (value.ClientSize.Width < DISPLAY_WIDTH) ? value.ClientSize.Width : DISPLAY_WIDTH;
    size.Height = (value.ClientSize.Height < DISPLAY_HEIGHT) ? value.ClientSize.Height : DISPLAY_WIDTH;
    
    if (coords.X + size.Width > DISPLAY_WIDTH - 1) {
        
        size.Width = (DISPLAY_WIDTH - 1) - coords.X;
    }
    
    
    if (coords.Y + size.Height > DISPLAY_HEIGHT - 1) {
        
        size.Height = (DISPLAY_HEIGHT - 1) - coords.Y;
    }
    
    OLED_client_rect.Location = coords;
    OLED_client_rect.ClientSize = size;
}
// ------------------------------------------------------------------------------------------------
// Получение области отрисовки экрана
Rect OLED_SPI_matrix_get_clientrect() {
    
    return OLED_client_rect;
}

// ------------------------------------------------------------------------------------------------
// Горизонтальная масштабируемая печать пропорциональным стандартным шрифтом
void OLED_SPI_matrix_writeCharScaled(uint8_t sym) {
    
    if (current_text_context.scale == 0) {
        current_text_context.scale = 1;
    }  // Защита от 0

    if ((uint16_t)sym >= sizeof (FONT_IMAGES) / sizeof (FONT_IMAGES[0])) { return; }

    uint8_t gl_width = FONT_IMAGES[sym][0];  // Чистая ширина символа
    uint16_t total_width = (uint16_t)gl_width * current_text_context.scale;

    // Проверка переноса строки (с учетом масштаба)
    if (current_text_context.wrap_mode != WM_None && current_text_context.x_caret + total_width > DISPLAY_WIDTH) {

        current_text_context.x_caret = 0;
        current_text_context.y_caret += (8 * current_text_context.scale) + current_text_context.space;
    }

    for (int index = 1; index <= gl_width; index++) {
        uint8_t sym_part = FONT_IMAGES[sym][index];

        for (int sx = 0; sx < current_text_context.scale; sx++) {
            int current_x = current_text_context.x_caret + ((index - 1) * current_text_context.scale) + sx;

            // Вместо break — просто пропускаем итерацию, если вышли за X
            if (current_x >= DISPLAY_WIDTH)
                continue;

            for (int y_step = 0; y_step < 8; y_step++) {
                uint8_t pixel = (sym_part >> y_step) & 0x01;

                for (int sy = 0; sy < current_text_context.scale; sy++) {
                    int current_y = current_text_context.y_caret + (y_step * current_text_context.scale) + sy;

                    // Пропускаем отрисовку пикселя, если вышли за Y, но продолжаем цикл
                    if (current_y >= DISPLAY_HEIGHT)
                        continue;

                    OLED_SPI_matrix_setPixel (current_x, current_y, pixel);
                }
            }
        }
    }

    // Смещение каретки: ширина символа * scale + межсимвольный интервал
    current_text_context.x_caret += total_width + current_text_context.space;
}

// ------------------------------------------------------------------------------------------------
// Вертикальная масштабируемая печать пропорциональным стандартным шрифтом
void OLED_SPI_matrix_writeVCharUpScaled (uint8_t sym) {

    if (current_text_context.scale == 0) {
        return;;
    }

    uint8_t gl_width = FONT_IMAGES[sym][0] + 1;  // Берем ширину из шрифта
    uint16_t total_v_height = (uint16_t)gl_width * current_text_context.scale;

    /* // ПРОВЕРКА ПЕРЕНОСА (если буква не влезает вверх)
    if (y_caret - total_v_height < 0) {
        y_caret = DISPLAY_HEIGHT - 1;      // Возвращаемся вниз
        x_caret += (8 * scale) + x_space;  // Сдвигаемся вправо на "высоту" буквы
    } */

    // 2. ЖЕСТКАЯ ПРОВЕРКА: если символ целиком не влезает до ВЕРХНЕГО края (0)
    // Мы переносим его на новую колонку ЗАРАНЕЕ
    if (current_text_context.wrap_mode != WM_None && current_text_context.y_caret < total_v_height) {
        current_text_context.y_caret = DISPLAY_HEIGHT - 1;  // Падаем в самый низ
        current_text_context.x_caret += (8 * current_text_context.scale) + current_text_context.space;  // Уходим вправо на новую "строку"
    }

    // 3. Если по X тоже вышли за экран — всё, финиш
    if (current_text_context.x_caret + (8 * current_text_context.scale) > DISPLAY_WIDTH)
        return;

    // Если ушли за правую границу экрана — выходим
    if (current_text_context.x_caret + (8 * current_text_context.scale) > DISPLAY_WIDTH)
        return;

    for (int index = 1; index < gl_width; index++) {
        uint8_t sym_part = FONT_IMAGES[sym][index];

        for (int sy = 0; sy < current_text_context.scale; sy++) {
            int current_y = current_text_context.y_caret - ((index - 1) * current_text_context.scale) - sy;
            if (current_y < 0 || current_y >= DISPLAY_HEIGHT)
                continue;

            for (int bit = 0; bit < 8; bit++) {
                uint8_t pixel = (sym_part >> bit) & 0x01;

                for (int sx = 0; sx < current_text_context.scale; sx++) {
                    int current_x = current_text_context.x_caret + (bit * current_text_context.scale) + sx;
                    if (current_x >= DISPLAY_WIDTH)
                        continue;

                    OLED_SPI_matrix_setPixel (current_x, current_y, pixel);
                }
            }
        }
    }

    // Смещаем каретку вверх для следующего символа
    current_text_context.y_caret -= (total_v_height + current_text_context.space);
}

// ------------------------------------------------------------------------------------------------
// Вывод пользовательского символа указанного размера до [64x16]
void OLED_SPI_matrix_writeLargeCustomChar(unsigned short *image, uint8_t width, uint8_t height) {

    for (int index = 0; index < height; index++) { // Строки образа символа

        uint8_t sym_part = image[index];

        for (int pix_index = 0; pix_index < width; pix_index++) { // Пиксели образа символа

            uint8_t pix_addr = current_text_context.x_caret + pix_index;  // Физическое положение пикселя

            uint8_t pix_value = (is_set_idx (sym_part, pix_index)) ? 1 : 0;

            OLED_SPI_matrix_setPixel (pix_addr, index + current_text_context.y_caret, pix_value);
        }
    }

    current_text_context.x_caret += (width + current_text_context.space);  // Передвигаем каретку на следующий символ
}
// ------------------------------------------------------------------------------------------------
// Вывод пользовательского изображения до [64x32]
void OLED_SPI_matrix_writeBitmap(uint32_t *image, uint8_t height) {

    uint8_t t_x_space = current_text_context.space;
    uint8_t t_x_caret = current_text_context.x_caret;
    uint8_t t_y_caret = current_text_context.y_caret;
    current_text_context.space = 0;

    for (int index = 0; index < height; index++) { // Строки образа символа
        
        unsigned long sym_part = image[index];

        current_text_context.x_caret = t_x_caret;

        uint8_t part = (sym_part & 0xFF);
        OLED_SPI_matrix_writeHByte(part, 8);
        
        part = (sym_part >> 8);
        OLED_SPI_matrix_writeHByte(part, 8);
        
        part = (sym_part >> 16);
        OLED_SPI_matrix_writeHByte(part, 8);
        
        part = (sym_part >> 24);
        OLED_SPI_matrix_writeHByte(part, 8);

        current_text_context.y_caret += 1;
    }

    current_text_context.space = t_x_space;
    current_text_context.y_caret = t_y_caret;
    current_text_context.x_caret += (32 + current_text_context.space);  // Передвигаем каретку на следующий символ
}

// ------------------------------------------------------------------------------------------------
// Печать строки символами двоичного шрифта
void OLED_SPI_matrix_writeStringBin (const uint8_t *str, const BINFont *font, float line_spacing) {

    while (*str) {

        OLED_SPI_matrix_writeCharBin(*str++, font, line_spacing);
    }
}

// ------------------------------------------------------------------------------------------------
// // Печать одного символа бинарного шрифта
void OLED_SPI_matrix_writeCharBin (const uint8_t ch, const BINFont *font, float line_spacing) {

    uint8_t c = (uint8_t)(ch - MATRIX_SHIFT_SYMCODE);  // Твое смещение

    // Обработка символа новой строки \n (если встретится в строке)
    if (ch + MATRIX_SHIFT_SYMCODE == '\n') {

        current_text_context.x_caret = 0;
        // Сдвигаем на высоту шрифта с учетом твоего коэффициента
        current_text_context.y_caret += (uint16_t)(font->height * line_spacing);

        return;
    }

    // Нет такого кода в таблице?
    if (c >= font->total_chars) {

        return;
    }

    // 1. ПРОВЕРКА ПЕРЕНОСА: влезет ли символ по ширине?
    if (current_text_context.x_caret > 0 && (current_text_context.x_caret - current_text_context.space + font->width > DISPLAY_WIDTH)) {
        current_text_context.x_caret = 0;                                         // Возврат каретки
        current_text_context.y_caret += (uint16_t)(font->height * line_spacing);  // Переход на новую строку
    }

    // 2. ПРОВЕРКА ВЕРТИКАЛИ: не ушли ли мы за низ экрана?
    if (current_text_context.y_caret > DISPLAY_HEIGHT + font->height) {
        return;;  // Дальше рисовать нет смысла
    }

    OLED_SPI_matrix_writeBitmapEx (&font->data[c * font->glyph_size], font->width, font->height);
}

// ------------------------------------------------------------------------------------------------
// Рисуем горизонтальный bitmap в вертикальный буфер дисплея
// x, y - координаты в пикселях (0-127, 0-63)
// bitmap - твой исходный массив
void OLED_SPI_matrix_writeBitmapEx (const uint8_t *bitmap, uint8_t width, uint8_t height) {

    uint8_t start_x = current_text_context.x_caret;
    uint8_t start_y = current_text_context.y_caret;

    uint8_t bytesPerRow = (width + 7) / 8;  // Автоматический расчет байт в строке

    for (uint8_t y = 0; y < height; y++) {
        for (uint8_t b = 0; b < bytesPerRow; b++) {
            // Читаем байт из массива по порядку
            uint8_t val = bitmap[y * bytesPerRow + b];

            // Разворачиваем байт: от 7-го бита к 0-му (MSB-First)
            for (uint8_t bit = 0; bit < 8; bit++) {
                // Вычисляем текущую координату X внутри байта
                uint8_t current_x = (b * 8) + bit;

                // Рисуем пиксель только если мы не вышли за границы ширины 'w'
                if (current_x < width) {
                    if (val & (0x80 >> bit)) {
                        OLED_SPI_matrix_setPixel (start_x + current_x, start_y + y, 1);
                    } else {
                        // Затираем фон (если нужно прозрачно - убери этот else)
                        OLED_SPI_matrix_setPixel (start_x + current_x, start_y + y, 0);
                    }
                }
            }
        }
    }
    // После отрисовки двигаем каретку для следующего элемента
    current_text_context.x_caret = start_x + width + current_text_context.space;
}

// ------------------------------------------------------------------------------------------------
// Измерение длины BIN строки
Size OLED_SPI_matrix_measureStringBin (const uint8_t *str, const BINFont *font) {

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
// Печать GFX символов (нет поддержки кириллицы)
void OLED_SPI_matrix_drawString_GFX2X (const uint8_t *str, const GFXfont *font, float line_spacing) {

    if (str == NULL || font == NULL)
        return;

    // float line_spacing = 1.1;

    while (*str) {
        uint8_t c = (uint8_t)(*str++ - MATRIX_SHIFT_SYMCODE);  // Твое смещение

        // Обработка символа новой строки \n (если встретится в строке)
        if (*str == '\n') {

            current_text_context.x_caret = 0;
            // Сдвигаем на высоту шрифта с учетом твоего коэффициента
            current_text_context.y_caret += (uint16_t)(font->yAdvance * line_spacing);

            continue;
        }

        // Если символ вне диапазона — пропускаем
        if (c > (font->last - font->first)) {

            continue;
        }

        GFXglyph *glyph = &(font->glyph[c]);
        uint8_t *bitmap = font->bitmap;

        // 1. ПРОВЕРКА ПЕРЕНОСА: влезет ли символ по ширине?
        uint16_t next_char_width = glyph->xAdvance;
        if (current_text_context.x_caret + next_char_width > DISPLAY_WIDTH) {
            current_text_context.x_caret = 0;                      // Возврат каретки
            current_text_context.y_caret += (uint16_t)(font->yAdvance * line_spacing);  // Переход на новую строку
        }

        // 2. ПРОВЕРКА ВЕРТИКАЛИ: не ушли ли мы за низ экрана?
        if (current_text_context.y_caret > DISPLAY_HEIGHT + (font->yAdvance)) {
            break;  // Дальше рисовать нет смысла
        }

        uint16_t bo = glyph->bitmapOffset;
        uint8_t w = glyph->width, h = glyph->height;
        int8_t xo = glyph->xOffset, yo = glyph->yOffset;
        uint8_t bits = 0, bit = 0;

        // Начальная точка отрисовки символа с учетом масштаба
        int16_t x_base = current_text_context.x_caret + xo;
        int16_t y_base = current_text_context.y_caret + yo;

        for (uint8_t yy = 0; yy < h; yy++) {
            for (uint8_t xx = 0; xx < w; xx++) {
                if (!(bit++ & 7)) {
                    bits = bitmap[bo++];
                }

                if (bits & 0x80) {
                    int16_t px = x_base + xx;
                    int16_t py = y_base + yy;

                    OLED_SPI_matrix_setPixel (px, py, 1);
                }

                bits <<= 1;
            }
        }

        // Сдвиг каретки с учетом масштаба и твоего x_space
        current_text_context.x_caret += (glyph->xAdvance + current_text_context.space);
    }
}

// ------------------------------------------------------------------------------------------------
// Заливка указанной области указанным цветом
void OLED_SPI_matrix_fill_rect (Rect area, uint8_t value) {

    for (int cur_y = 0; cur_y < area.ClientSize.Height; cur_y++) {

        for (int cur_x = 0; cur_x < area.ClientSize.Width; cur_x++) {

            OLED_SPI_matrix_setPixel (area.Location.X + cur_x, area.Location.Y + cur_y, value);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Заливка указанной области указанным цветом
void OLED_SPI_matrix_fill_rect2 (uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t value) {

    Rect area = {{ x, y }, { w, h}};
    OLED_SPI_matrix_fill_rect(area, value);
}

// ------------------------------------------------------------------------------------------------
// Печать строки стандартным шрифтом
void OLED_SPI_matrix_writeString (uint8_t *str) {

    // x_caret = x_scroll;

    for (uint8_t i = 0; str[i] != '\0'; ++i) {

        if (str[i] == 0xFF) {

            current_text_context.x_caret += current_text_context.space;
            continue;
        }

        // matrix_writeChar(str[i] - MATRIX_BIN_SHIFT_SYMCODE);

        // Кириллица в UTF8?
        if ((str[i] == 0xD0 || str[i] == 0xD1) && (matrix_cyr_Get_NOCyrillic() == 0)) {

            uint8_t new_code = matrix_cyr_BASE_UTF2CYR (str[i], str[i + 1]);
            i += 1;

            switch (current_text_context.direction) {

            case WS_Vertical:

                OLED_SPI_matrix_writeVCharUpScaled (new_code);
                break;

            default:

                OLED_SPI_matrix_writeCharScaled (new_code);
                break;
            }

        } else {

            switch (current_text_context.direction) {

            case WS_Vertical:

                OLED_SPI_matrix_writeVCharUpScaled (str[i] - MATRIX_SHIFT_SYMCODE);
                break;

            default:

                OLED_SPI_matrix_writeCharScaled (str[i] - MATRIX_SHIFT_SYMCODE);
                break;
            }
        }
    }

    matrix_cyr_RESet_NOCyrillic();
}

// ------------------------------------------------------------------------------------------------
// Вычисление области пересечения двух областей
Rect OLED_SPI_matrix_rect_intersect (Rect r1, Rect r2) {

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
// Проверка, является ли указанная область пустой
uint8_t OLED_SPI_matrix_rect_isEmpty (Rect area) {

    return (area.ClientSize.Height == 0 && area.ClientSize.Width == 0);
}

// ------------------------------------------------------------------------------------------------
// Получение координат углов указанной области
Point OLED_SPI_matrix_get_corner (Rect area, Corner subject) {

    if (OLED_SPI_matrix_rect_isEmpty (area)) {
        return (Point){0, 0};
    }

    int dx = (subject == TopRight || subject == BottomRight) ? area.ClientSize.Width : 0;
    int dy = (subject == BottomLeft || subject == BottomRight) ? area.ClientSize.Height : 0;

    return (Point){area.Location.X + dx, area.Location.Y + dy};
}

// ------------------------------------------------------------------------------------------------
// Метод печати строки стандартным шрифтом в центр указанной области с центровкой и обрезкой (ellipsis) по границам области
void OLED_SPI_matrix_drawStringInscribed_Smart (uint8_t *str, Rect area_owner) {

    if (!str || *str == '\0')
        return;

    // Сохраняем текущий режим переноса, чтобы не испортить глобальные настройки
    uint8_t old_wrap = current_text_context.wrap_mode;
    OLED_SPI_matrix_set_textContext_prop (TX_Wrap3, WM_None);

    Size strSize = OLED_SPI_matrix_measureString (str);
    Rect old_clip = OLED_client_rect;

    int32_t ownerW = area_owner.ClientSize.Width;
    int32_t ownerH = area_owner.ClientSize.Height;
    int32_t strLen = strSize.Width;
    int32_t strThick = strSize.Height;

    //printf ("strSize.Width: %d\r\n", strSize.Width);
    //printf ("owner.Width: %d\r\n", ownerW);

    OLED_SPI_matrix_set_clientrect (OLED_SPI_matrix_rect_intersect (old_clip, area_owner));

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
            OLED_SPI_matrix_writeString (str);
        } else {
            // Логика Ellipsis (для длинных строк Right обычно игнорируют, чтобы видеть начало)
            //printf("--- ellipsis\r\n");
            Size dotSize = OLED_SPI_matrix_measureString ((uint8_t *)"...");
            current_text_context.x_caret = area_owner.Location.X;
            Rect text_clip = area_owner;
            text_clip.ClientSize.Width -= dotSize.Width;
            OLED_SPI_matrix_set_clientrect (OLED_SPI_matrix_rect_intersect (old_clip, text_clip));
            OLED_SPI_matrix_writeString (str);
            OLED_SPI_matrix_set_clientrect (OLED_SPI_matrix_rect_intersect (old_clip, area_owner));
            current_text_context.x_caret = area_owner.Location.X + ownerW - dotSize.Width;
            OLED_SPI_matrix_writeString ((uint8_t *)"...");
        }
    } else {  // --- ВЕРТИКАЛЬНЫЙ РЕЖИМ (сверху вниз!) ---
        current_text_context.x_caret = area_owner.Location.X + (int16_t)((ownerW - strThick + 1) / 2);

        if (strLen <= ownerH) {
            if (current_text_context.align == WA_Center) {
                current_text_context.y_caret = area_owner.Location.Y + strLen + (int16_t)((ownerH - strLen) / 2);
            } else if (current_text_context.align == WA_Right) {
                // Для печати ВВЕРХ "прижать к верху области" — это закончить на Y_start
                current_text_context.y_caret = area_owner.Location.Y + strLen + current_text_context.space * current_text_context.scale + 2;
            } else {  // WA_Left (прижать к низу)
                current_text_context.y_caret = area_owner.Location.Y + ownerH - 2;
            }
            OLED_SPI_matrix_writeString (str);
        } else {
            // Ellipsis для вертикали
            // --- ВЕТКА ДЛИННОЙ СТРОКИ (Ellipsis) ---
            Size dotSize = OLED_SPI_matrix_measureString ((uint8_t *)"...");

            // 1. Начинаем снизу области
            current_text_context.y_caret = OLED_SPI_matrix_get_corner(area_owner, BottomLeft).Y;

            Rect text_clip = area_owner;
            // 2. Обрезаем область СНИЗУ (минус высота точек)
            // Было: text_clip.Location.Y += dotSize.Width; (это обрезало верх)
            text_clip.ClientSize.Height -= dotSize.Width;
            text_clip.Location.Y += dotSize.Width;

            // 3. Убеждаемся, что перенос выключен, чтобы не уйти в бок
            uint8_t old_wrap = current_text_context.wrap_mode;
            OLED_SPI_matrix_set_textContext_prop (TX_Wrap3, WM_None);

            OLED_SPI_matrix_set_clientrect (OLED_SPI_matrix_rect_intersect (old_clip, text_clip));
            OLED_SPI_matrix_writeString (str);

            // 4. Рисуем точки в самом верху области
            OLED_SPI_matrix_set_clientrect (OLED_SPI_matrix_rect_intersect (old_clip, area_owner));
            current_text_context.y_caret = area_owner.Location.Y + dotSize.Width + 1;
            OLED_SPI_matrix_writeString ((uint8_t *)"...");
            OLED_SPI_matrix_set_textContext_prop (TX_Wrap3, old_wrap);
        }
    }

    // В конце функции возвращаем режим на место
    OLED_SPI_matrix_set_textContext_prop (TX_Wrap3, old_wrap);
    OLED_SPI_matrix_set_clientrect (old_clip);
}

// ------------------------------------------------------------------------------------------------
// Вывод горизонтальной строки из указанного количества пикселей в текущую позицию каретки
void OLED_SPI_matrix_writeHByte(uint8_t value, uint8_t length) {

    for (int i = 0; i < length; i++) {

        if (is_set_idx (value, i)) {

            OLED_SPI_matrix_setPixel (current_text_context.x_caret + i, current_text_context.y_caret, 1);
        } else {

            OLED_SPI_matrix_setPixel (current_text_context.x_caret + i, current_text_context.y_caret, 0);
        }
    }

    current_text_context.x_caret += (length + current_text_context.space);  // Передвигаем каретку на следующий символ
}
// ------------------------------------------------------------------------------------------------
// Вывод вертикальной строки вниз из указанного количества пикселей в текущую позицию каретки
void OLED_SPI_matrix_writeVByteDown(uint8_t value, uint8_t length) {

    for (int i = 0; i < length; i++) {

        if (is_set_idx (value, i)) {

            OLED_SPI_matrix_setPixel (current_text_context.x_caret, current_text_context.y_caret + i, 1);
        } else {

            OLED_SPI_matrix_setPixel (current_text_context.x_caret, current_text_context.y_caret + i, 0);
        }
    }

    current_text_context.x_caret += (current_text_context.space + 1);  // Передвигаем каретку на следующий символ
}
// ------------------------------------------------------------------------------------------------
// Вывод вертикальной строки вверх из указанного количества пикселей в текущую позицию каретки
void OLED_SPI_matrix_writeVByteUp(uint8_t value, uint8_t length) {

    for (int i = 0; i < length; i++) {

        if (is_set_idx (value, i)) {

            OLED_SPI_matrix_setPixel (current_text_context.x_caret, current_text_context.y_caret - i, 1);
        } else {

            OLED_SPI_matrix_setPixel (current_text_context.x_caret, current_text_context.y_caret - i, 0);
        }
    }

    current_text_context.x_caret += (current_text_context.space + 1);  // Передвигаем каретку на следующий символ
}

// ------------------------------------------------------------------------------------------------
// Обновление экрана V1
// Используется при окончании отрисовки фрейма
void OLED_SPI_matrix_refresh() {
    // --------------------------------------------------------------------------------------------
    OLED_SPI_SSD1306_BASE_SendBuffer((uint8_t *)OLED__screen_buffer, matrix_HARDWARE_BUFFER_SIZE, 0);
    // --------------------------------------------------------------------------------------------
}
// ------------------------------------------------------------------------------------------------
// Очистка буфера и инициализация графики без изменения физического экрана
// Используется для начала рисования фрейма
void OLED_SPI_matrix_frame_init() {

    for (unsigned int i = 0; i < sizeof (OLED__screen_buffer); i++) {
        OLED__screen_buffer[i] = 0;
    }

    OLED_SPI_matrix_returnCaret();

    current_text_context.space = 1;
    current_text_context.x_brush = 1;
    current_text_context.x_bg_transparent = 0;

    OLED_client_rect.Location = OLED_SPI_matrix_create_point(0, 0);
    OLED_client_rect.ClientSize = OLED_SPI_matrix_create_size(DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
}
// ------------------------------------------------------------------------------------------------
// Очистка физического и буфера экрана с обновлением
void OLED_SPI_matrix_clearScreen() {

    OLED_SPI_matrix_frame_init();
    OLED_SPI_matrix_refresh();
}

// ------------------------------------------------------------------------------------------------
// Рассчет длины указанной строки в пикселях (стандартные шрифты)
Size OLED_SPI_matrix_measureString (uint8_t *str) {

    if (current_text_context.scale == 0) {
        return (Size){0, 0};
    }

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
            code -= MATRIX_SHIFT_SYMCODE;
        }

        // Берем ширину из нулевого байта образа и масштабируем
        // +1 если в шрифте есть защитный интервал, как в коде печати
        uint8_t char_w = FONT_IMAGES[code][0];

        result.Width += (char_w * current_text_context.scale) + current_text_context.space;
    }

    return result;
}
// ------------------------------------------------------------------------------------------------
// Рисование линии по алгоритму Брезенхейма
void OLED_SPI_matrix_line(int x0, int y0, int x1, int y1) {

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    // --------------------------------------------------------------------------------------------
    for (;;) {

        OLED_SPI_matrix_setPixel(x0, y0, 1);

        if (x0 == x1 && y0 == y1) break;
        e2 = err;

        if (e2 >-dx) {

            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {

            err += dx;
            y0 += sy;
        }
    }
    // --------------------------------------------------------------------------------------------
}
// ------------------------------------------------------------------------------------------------
// Рисование линии по алгоритму Брезенхейма
void OLED_SPI_matrix_line2(Point A1, Point A2) {

    OLED_SPI_matrix_line(A1.X, A1.Y, A2.X, A2.Y);
}
// ------------------------------------------------------------------------------------------------
// Рисование окружности методом Брезенхейма
void OLED_SPI_matrix_circle(int x0, int y0, int radius, uint8_t fill) {
    // --------------------------------------------------------------------------------------------
    int x = radius - 1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);
    // --------------------------------------------------------------------------------------------
    while (x >= y) {

        if (fill) {

            OLED_SPI_matrix_line(x0 - x, y0 + y, x0 + x, y0 + y);
            OLED_SPI_matrix_line(x0 - x, y0 - y, x0 + x, y0 - y);
            OLED_SPI_matrix_line(x0 - y, y0 + x, x0 + y, y0 + x);
            OLED_SPI_matrix_line(x0 + y, y0 - x, x0 - y, y0 - x);
        } else {

            OLED_SPI_matrix_setPixel(x0 + x, y0 + y, 1);
            OLED_SPI_matrix_setPixel(x0 + y, y0 + x, 1);
            OLED_SPI_matrix_setPixel(x0 - y, y0 + x, 1);
            OLED_SPI_matrix_setPixel(x0 - x, y0 + y, 1);
            OLED_SPI_matrix_setPixel(x0 - x, y0 - y, 1);
            OLED_SPI_matrix_setPixel(x0 - y, y0 - x, 1);
            OLED_SPI_matrix_setPixel(x0 + y, y0 - x, 1);
            OLED_SPI_matrix_setPixel(x0 + x, y0 - y, 1);
        }

        if (err <= 0) {

            y++;
            err += dy;
            dy += 2;
        }

        if (err > 0) {

            x--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
    // --------------------------------------------------------------------------------------------
}
// ------------------------------------------------------------------------------------------------
// Рисование окружности методом Брезенхейма
void OLED_SPI_matrix_circle2(Point data, uint8_t fill) {

    OLED_SPI_matrix_circle(data.X, data.Y, data.extend, fill);
}
// ------------------------------------------------------------------------------------------------
// Рисование полигона из произвольного количества отрезков (больше одного)
void OLED_SPI_matrix_polygon(Point *data, uint8_t count, uint8_t closed) {
    // --------------------------------------------------------------------------------------------
    Point start_p = data[0];
    // --------------------------------------------------------------------------------------------
    for (int index = 1; index < count; index++) {
        Point next_p = data[index];

        OLED_SPI_matrix_line2(start_p, next_p);
        start_p = next_p;
    }
    // --------------------------------------------------------------------------------------------
    if (closed == 1) {
        // Замыкаем полигон
        OLED_SPI_matrix_line2(start_p, data[0]);
    }
    // --------------------------------------------------------------------------------------------
}
// ------------------------------------------------------------------------------------------------
// Рисование простой рамки указанного размера
void OLED_SPI_matrix_draw_rect(Point data, Size size) {
    
    Point t_tmp[4];
    
    // TopLeft
    t_tmp[0] = data;
    
    // TopRight
    t_tmp[1].X = data.X + size.Width;
    t_tmp[1].Y = data.Y;
    
    // BottomRight
    t_tmp[2].X = t_tmp[1].X;
    t_tmp[2].Y = data.Y + size.Height;
    
    // BottomLeft
    t_tmp[3].X = data.X;
    t_tmp[3].Y = t_tmp[2].Y;
    
    OLED_SPI_matrix_polygon(t_tmp, 4, 1);
}

// ------------------------------------------------------------------------------------------------
// Рисование простой рамки указанного размера
void OLED_SPI_matrix_draw_rect2 (Rect area) {

    OLED_SPI_matrix_draw_rect(area.Location, area.ClientSize);
}

// ------------------------------------------------------------------------------------------------
// Рамка в один пиксель по краям дисплея
void OLED_SPI_matrix_client_rect() {

    Point data;
    data.X = 0;
    data.Y = 0;

    Size size;
    size.Width = DISPLAY_WIDTH - 1;
    size.Height = DISPLAY_HEIGHT - 1;

    OLED_SPI_matrix_draw_rect (data, size);
}
// ------------------------------------------------------------------------------------------------
// Построение заштрихованного прямоугольника
void OLED_SPI_matrix_filled_rect(Point data, Size size) {
    // --------------------------------------------------------------------------------------------
    Point data2, data3;
    
    data2 = data;
    data3.X = data.X + size.Width;
    data3.Y = data.Y;
    // --------------------------------------------------------------------------------------------
    for (int i = 0; i < size.Height; i++) {
        
        OLED_SPI_matrix_line2(data2, data3);
        
        data2.Y += 1;
        data3.Y += 1;
    }
    // --------------------------------------------------------------------------------------------
}
// ------------------------------------------------------------------------------------------------
// Сдвиг указанных координат на указанный диапазон [-127..127]
Point OLED_SPI_matrix_shift_coords(Point data, int8_t X, int8_t Y) {
    
    Point result;
    
    result.X = data.X + X;
    result.Y = data.Y + Y;
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Получение координат верхнего правого угла прямоугольной области
Point OLED_SPI_matrix_get_rect_topright(Rect rect) {
    
    Point result;
    
    result.X = rect.Location.X + rect.ClientSize.Width;
    result.Y = rect.Location.Y;
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Получение координат верхнего правого угла прямоугольной области
Point OLED_SPI_matrix_get_rect_bottomright(Rect rect) {
    
    Point result;
    
    result.X = rect.Location.X + rect.ClientSize.Width;
    result.Y = rect.Location.Y + rect.ClientSize.Height;
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Получение координат верхнего правого угла прямоугольной области
Point OLED_SPI_matrix_get_rect_bottomleft(Rect rect) {
    
    Point result;
    
    result.X = rect.Location.X;
    result.Y = rect.Location.Y + rect.ClientSize.Height;
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Пропорционально уменьшить прямоугольную область
Rect OLED_SPI_matrix_reduce_rect(Rect rect, uint8_t value) {
    
    Rect result;
    
    result.Location.X = rect.Location.X + value;
    result.Location.Y = rect.Location.Y + value;
    result.ClientSize.Width = rect.ClientSize.Width - (value * 2);
    result.ClientSize.Height = rect.ClientSize.Height - (value * 2);
    
    return result;
}
// ------------------------------------------------------------------------------------------------
// Пропорционально уменьшить прямоугольную область на один пиксель
Rect OLED_SPI_matrix_reduce_rect2(Rect rect) {
    
    return OLED_SPI_matrix_reduce_rect(rect, 1);
}

// ------------------------------------------------------------------------------------------------
// Пропорционально увеличить прямоугольную область
Rect OLED_SPI_matrix_increase_rect(Rect rect, uint8_t value) {
    
    Rect result;
    
    result.Location.X = rect.Location.X - value;
    result.Location.Y = rect.Location.Y - value;
    result.ClientSize.Width = rect.ClientSize.Width + (value * 2);
    result.ClientSize.Height = rect.ClientSize.Height + (value * 2);
    
    return result;
}
// ------------------------------------------------------------------------------------------------
