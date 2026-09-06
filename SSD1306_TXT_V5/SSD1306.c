/********************************** (C) COPYRIGHT ******************************
 * File Name          : SSD1306.h
 * Author             : vantr
 * Description        : Библиотека текстового вывода на дисплеи SSD1306 (ПОЛНАЯ)
 *                    : работает с кириллицей UTF8 прямо из текста программы
 *******************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "debug.h"
#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_Common_Lib_V1/sss_math.h"
#include "SSS_Common_Lib_V1/sss_graphics_common_types.h"
#include "SSS_I2CHW_MLib_V4.4/i2c.h"
#include "SSD1306.h"
#include "string.h"
#ifdef SSD1306_USE_EEPROM_FONTS
#include "SSS_EPR24Cxx_Lib1/epr_lib1.h"
#else
#include "FONTS.h"
#endif
// -----------------------------------------------------------------------------
Params temp_data;
// -----------------------------------------------------------------------------
// value1 - X столбец [0..DISPLAY_WIDTH_SYM - 1]
// value2 - Y строка [0..DISPLAY_HEIGHT_ROW - 1]
Params screen_options;
// Params caret_tmp;
//  -----------------------------------------------------------------------------
//  Режим переноса при переполнении
SSD1306_WRAP_MODE4 _current_wrap = FULLWRAP4;
// -----------------------------------------------------------------------------
// Ограничения печати для метода "SSD1306_TXT_PrintLongInt"
SSD1306_PRINTINT_RESTRICT_MODES _current_intrestr = REST_0;
// -----------------------------------------------------------------------------
// Лимитирование выводимой строки
volatile uint8_t _flag_str_limit = 0;
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_EEPROM_FONTS
// Буфер для одиночного символа
volatile uint8_t SYM_IMAGE[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// -----------------------------------------------------------------------------
// Сброс буферов конвертора
// buffer - ссылка на очищаемый байтовый буфер
// buffer_size - размер очищаемого буфера
void reset_buffer2 (uint8_t *buffer, uint8_t buffer_size) {

    for (int i = 0; i < buffer_size; i++) {

        buffer[i] = 0;
    }
}
#endif
// -----------------------------------------------------------------------------
// Запись команды/данных в чип контроллера
uint8_t SSD1306_BASE_SEND (uint8_t mode, uint8_t data) {

    Delay_Us (10);
    uint8_t tmp_buf[2] = {mode, data};
    
    return i2c_write (SSD1306_CHIP_SAVEADDR, tmp_buf, sizeof (tmp_buf));
}
// -----------------------------------------------------------------------------
// Установка контрастности [0..FF], где 0x7F - по умолчанию после включения
void SSD1306_TXT_CONTRAST (uint8_t value) {

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMD_SETCONTRAST);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, value);
}
// -----------------------------------------------------------------------------
// Включение дисплея
void SSD1306_TXT_ON (void) {

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x14);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAF);
}
// -----------------------------------------------------------------------------
// Полное отключение дисплея
void SSD1306_TXT_OFF (void) {

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x10);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAE);
}
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_MY_FONTS
// Пересчет кода символа кириллицы из UTF8 в мой табличный
// Используйте именно так! Строку - напрямую, размер ТОЛЬКО через sizeof() потому,
// что строка видимая и её длина будут отличаться!
// uint8_t cyr_str[] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ абвгдеёжзийклмнопрстуфхцчшщъыьэюя";
// SSD1306_TXT_WriteStr ((uint8_t *)cyr_str, sizeof (cyr_str), 0);
uint8_t SSD1306_BASE_UTF2CYR (uint8_t value1, uint8_t value2) {

    if (value1 == 0xD0) {

        // Заглавные и первая часть строчных А-п (0x90 до 0xBF в UTF8)
        if (value2 >= 0x90 && value2 <= 0xBF) {
            return value2 - 0x30;  // 0x30 это 48 в DEC
        }
        
        // Буква 'Ё' (если нужна, нужно определить её код в твоей таблице)
        if (value2 == 0x81) return 0x65;
    } else if (value1 == 0xD1) {
        
        // Вторая часть строчных р-я (0x80 до 0x8F в UTF8)
        if (value2 >= 0x80 && value2 <= 0x8F) {
            
            return value2 + 0x10;  // 0x10 это 16 в DEC
        }
        
        // Буква 'ё' (если нужна)
        if (value2 == 0x91) return 0x85;
    }

    return 0x3F;  // Вернуть '?' или пробел, если символ не кириллица в этом диапазоне
}
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
// Установка флага ограничения на вывод ОДНОЙ строки
void SSD1306_TXT_SetStr_Limit(uint8_t limit_value) {

    _flag_str_limit = limit_value;
}
// -----------------------------------------------------------------------------
// Очистка экрана
void SSD1306_TXT_ClearScreen (uint8_t on_by_complete) {
    // -------------------------------------------------------------------------
    SSD1306_TXT_OFF();
    // -------------------------------------------------------------------------
    // Сброс координат вывода
    screen_options.value1 = 0;
    screen_options.value2 = 0;
    // -------------------------------------------------------------------------
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDCOLADDR_SETCOL);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x7F);
    // -------------------------------------------------------------------------
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDPAGEADDR_SETPAGE);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x07);  // 128x64
    // -------------------------------------------------------------------------
    uint16_t total_bytes = (OLED_DISPLAY_WIDTH * OLED_DISPLAY_HEIGHT_SYM) / 8;

    for (int index = 0; index < total_bytes; index++) {

        SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, 0x00);
    }
    // -------------------------------------------------------------------------
    if (on_by_complete > 0) {

        SSD1306_TXT_ON();
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_EEPROM_FONTS
// Загрузка символа 8x8 из EEPROM (24Cxx) по индексу символа
// Выход: успешность выполненой операции (bool)
uint8_t SSD1306_LoadSymbol (uint16_t sym_index) {

    // Очистка символьного буфера
    reset_buffer2 ((uint8_t *)SYM_IMAGE, 8);
    // for (int s_index = 0; s_index < 8; s_index++) { SYM_IMAGE[s_index] = 0; }

    if (sym_index == 0xFFFF) {
        return TRUE;
    }
    
    sym_index -= 0x20;

    if (sym_index >= SYMBOLS_COUNT_IN_EPROM) {
        return FALSE;
    }

    uint8_t result = EPR_readPage(0, (sym_index * 8), (uint8_t *)SYM_IMAGE, 8);
    
    return (result == 8) ? TRUE : FALSE;
}
#endif
// -----------------------------------------------------------------------------
// "Удвоение" указанного значения 8 бит -> 16 бит
Int16x SSD1306_BASE_DOUBLEVALUE (uint8_t Value, uint16_t Style) {
    // -------------------------------------------------------------------------
    Int16x result;
    result.ivalue = 0;
    // -------------------------------------------------------------------------
    // Растягивание значения до 16 бит
    for (int index = 0; index < 8; index++) {

        if (is_set_idx(Value, index)) {

            result.ivalue = sbi (result.ivalue, index * 2);
            result.ivalue = sbi (result.ivalue, index * 2 + 1);
        }
    }
    // -------------------------------------------------------------------------
    // Сдвиг символа на один пиксель вниз для его центровки
    if (is_clear_mask(Style, SYMSTYLE_NOALIGN)) {

        result.ivalue = result.ivalue << 1;
    }
    // -------------------------------------------------------------------------
    if (is_set_mask(Style, SYMSTYLE_INV)) {

        result.ivalue = ~result.ivalue;
    }
    // -------------------------------------------------------------------------
    // Применение стилей
    if (is_set_mask(Style, SYMSTYLE_UL)) {

        result.ivalue = sbi (result.ivalue, 0);
    }
    // -------------------------------------------------------------------------
    if (is_set_mask(Style, SYMSTYLE_DL)) {

        result.ivalue = sbi (result.ivalue, 15);
    }
    // -------------------------------------------------------------------------
    return result;
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка нового режима переноса при переполнении строки вывода
void SSD1306_TXT_SetWrapMode (SSD1306_WRAP_MODE4 NewMode) {

    _current_wrap = NewMode;
}
// -----------------------------------------------------------------------------
// Установка новых координат для печати символов
void SSD1306_TXT_SetCursor (uint8_t X, uint8_t Y) {

    screen_options.value1 = (X < OLED_DISPLAY_WIDTH_SYM) ? X : OLED_DISPLAY_WIDTH_SYM - 1;
    screen_options.value2 = (Y < OLED_DISPLAY_HEIGHT_ROW) ? Y : OLED_DISPLAY_HEIGHT_ROW - 1;
}
// -----------------------------------------------------------------------------
// Перевод каретки в начало текущей строки
void SSD1306_TXT_CarretReturnThisLine (void) {

    screen_options.value1 = 0;
}
// -----------------------------------------------------------------------------
// Перевод каретки в начало текущего экрана
void SSD1306_TXT_CarretReturnThisScreen (void) {

    screen_options.value1 = 0;
    screen_options.value2 = 0;
}
// -----------------------------------------------------------------------------
// Переход на следующее знакоместо сообразно настройкам
void SSD1306_TXT_IncrementCarret (void) {

    screen_options.value1 += 1;

    if (screen_options.value1 >= OLED_DISPLAY_WIDTH_SYM) {

        switch (_current_wrap) {

        case FULLWRAP4:
        case NORMALWRAP4:

            screen_options.value1 = 0;
            screen_options.value2 += 1;

            if (screen_options.value2 >= OLED_DISPLAY_HEIGHT_ROW && _current_wrap == FULLWRAP4) {

                screen_options.value2 = 0;
            }

            break;

        default:

            break;
        }
    }
}
// -----------------------------------------------------------------------------
// Аппаратная установка курсора в указанную позицию (ширина символа 8 пикселей)
// Координаты должны быть корректными!
// Системная процедура. Не вызывать из своего кода!
// step = 8 или 16
void SSD1306_BASE_MoveCursor (uint8_t X, uint8_t Y, uint8_t step) {
    // -------------------------------------------------------------------------
    // Установка начального столбца текущего символа
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDCOLADDR_SETCOL);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, X * step);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, X * step + (step - 1));
    // -------------------------------------------------------------------------
    // Установка начального банка строки текущего символа
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDPAGEADDR_SETPAGE);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, Y);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, Y);
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Печать широких 16x16 символов
// start_table - начало области двойных символов в общей таблице символов
// symbol_code - код выводимого символа из таблицы start_table от её начала
void SSD1306_TXT_PutWidthSymbol (uint8_t symbol_code, uint16_t style) {
    // -------------------------------------------------------------------------
    /* Params caret_tmp;

    caret_tmp.value1 = screen_options.value1;
    caret_tmp.value2 = screen_options.value2; */
    // -------------------------------------------------------------------------
    // Если курсор вне поля, ничего не делаем...
    if (screen_options.value1 + 1 >= OLED_DISPLAY_WIDTH_SYM ||
        screen_options.value2 + 1 >= OLED_DISPLAY_HEIGHT_ROW) {

        return;
    }
    // -------------------------------------------------------------------------
    // Получение начального столбца экрана для вывода символа
    // temp_data.value1 = screen_options.value1 * 8;
    // -------------------------------------------------------------------------
    // Передача символа (верхняя половина)
    SSD1306_BASE_MoveCursor ((screen_options.value1 / 2), screen_options.value2, 16);
    // -------------------------------------------------------------------------
#ifdef SSD1306_USE_EEPROM_FONTS
    SSD1306_LoadSymbol (symbol_code);
#endif
    // -------------------------------------------------------------------------
    Int16x dbl_sym;

    dbl_sym = SSD1306_BASE_DOUBLEVALUE (0, style);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, dbl_sym.cvalue[0]);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, dbl_sym.cvalue[0]);
    // SSD1306_BASE_SEND(SSD1306_CMD_WRITEDATA, temp_data.sign);
    //  -------------------------------------------------------------------------
    for (int index = 0; index < 7; index++) {
        // - Вывод верхней части символа ---------------------------------------
#ifdef SSD1306_USE_EEPROM_FONTS
        temp_data.extend = SYM_IMAGE[index];
#else
        temp_data.extend = FONT_IMAGES[symbol_code - 0x20][index];
#endif
        dbl_sym = SSD1306_BASE_DOUBLEVALUE (temp_data.extend, style);

        SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, dbl_sym.cvalue[0]);
        SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, dbl_sym.cvalue[0]);
        // ---------------------------------------------------------------------
    }
    // -------------------------------------------------------------------------
    // Передача символа (нижняя половина)
    SSD1306_BASE_MoveCursor ((screen_options.value1 / 2), screen_options.value2 + 1, 16);
    // -------------------------------------------------------------------------
    dbl_sym = SSD1306_BASE_DOUBLEVALUE (0, style);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, dbl_sym.cvalue[1]);
    SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, dbl_sym.cvalue[1]);
    // SSD1306_BASE_SEND(SSD1306_CMD_WRITEDATA, temp_data.sign);
    //  -------------------------------------------------------------------------
    for (int index = 0; index < 7; index++) {
        // - Вывод нижней части символа ---------------------------------------
#ifdef SSD1306_USE_EEPROM_FONTS
        temp_data.extend = SYM_IMAGE[index];
#else
        temp_data.extend = FONT_IMAGES[symbol_code - 0x20][index];
#endif
        dbl_sym = SSD1306_BASE_DOUBLEVALUE (temp_data.extend, style);

        SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, dbl_sym.cvalue[1]);
        SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, dbl_sym.cvalue[1]);
        // ---------------------------------------------------------------------
    }
    // -------------------------------------------------------------------------
    // Переход на следующий символ
    SSD1306_TXT_IncrementCarret();
    SSD1306_TXT_IncrementCarret();
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Печать ДВУХСТРОЧНЫХ символов ("LED")
// start_table - начало области двойных символов в общей таблице символов
// symbol_code - код выводимого символа из таблицы start_table от её начала
void SSD1306_TXT_PutDoubleSymbol (uint8_t symbol_code, uint16_t style) {
    // -------------------------------------------------------------------------
    uint8_t caret_tmpx = screen_options.value1;
    uint8_t caret_tmpy = screen_options.value2;
    // -------------------------------------------------------------------------
    // Печать верхней части символа
    SSD1306_TXT_SetCursor (caret_tmpx, caret_tmpy);
    SSD1306_TXT_PutSymbol (symbol_code, SYMSTYLE_NOALIGN | style);
    // -------------------------------------------------------------------------
    // Печать нижней части символа
    SSD1306_TXT_SetCursor (caret_tmpx, caret_tmpy + 1);
    SSD1306_TXT_PutSymbol (symbol_code + 1, SYMSTYLE_NOALIGN | style);
    // -------------------------------------------------------------------------
    SSD1306_TXT_SetCursor(caret_tmpx, caret_tmpy);
    SSD1306_TXT_IncrementCarret();
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Печать символа 8x8 в текущую позицию курсора
void SSD1306_TXT_PutSymbol (uint8_t symbol_code, uint16_t style) {
    // -------------------------------------------------------------------------
    // Если курсор вне поля, ничего не делаем...
    if (screen_options.value1 >= OLED_DISPLAY_WIDTH_SYM ||
        screen_options.value2 >= OLED_DISPLAY_HEIGHT_ROW) {

        return;
    }
    // -------------------------------------------------------------------------
    SSD1306_BASE_MoveCursor (screen_options.value1, screen_options.value2, 8);
    // -------------------------------------------------------------------------
#ifdef SSD1306_USE_EEPROM_FONTS
    SSD1306_LoadSymbol (symbol_code);
#endif
    // -------------------------------------------------------------------------
    // Передача символа
    for (int index = -1; index < 7; index++) {
        // ---------------------------------------------------------------------
        // Сдвиг малого символа на один пиксел вправо
        if (index >= 0) {
#ifdef SSD1306_USE_EEPROM_FONTS
            temp_data.extend = SYM_IMAGE[index];
#else
            temp_data.extend = FONT_IMAGES[symbol_code - 0x20][index];
#endif
        } else {

            temp_data.extend = 0;
        }
        // ---------------------------------------------------------------------
        // Применение стилей
        if (is_set_mask(style, SYMSTYLE_INV)) {

            temp_data.extend = ~temp_data.extend;
        }
        // ---------------------------------------------------------------------
        if (is_set_mask(style, SYMSTYLE_UL)) {

            temp_data.extend = sbi (temp_data.extend, 0);
        }
        // ---------------------------------------------------------------------
        if (is_set_mask(style, SYMSTYLE_DL)) {

            temp_data.extend = sbi (temp_data.extend, 7);
        }
        // ---------------------------------------------------------------------
        SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, temp_data.extend);
        // ---------------------------------------------------------------------
    }
    // -------------------------------------------------------------------------
    // Переход на следующий символ
    SSD1306_TXT_IncrementCarret();
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Вывод строки на индикатор в позицию курсора
// ВНИМАНИЕ! Только нуль-терминальные строки!
void SSD1306_TXT_WriteStr (uint8_t *str, uint16_t style) {

    uint8_t real_printed_syms = 0;

    while (*str) {  // Пока символ, на который указывает str, не 0

        if (_flag_str_limit > 0 && real_printed_syms >= _flag_str_limit) { break; }

#ifdef SSD1306_USE_MY_FONTS
        // Проверяем текущий байт на маркер кириллицы
        if ((*str == 0xD0 || *str == 0xD1) && (is_clear_mask(style, SYMSTYLE_NOCYR))) {

            // Защита от битого хвоста
            if (*(str + 1) == '\0') { break; }

            // Берем текущий и следующий байт для конвертации
            uint8_t new_code = SSD1306_BASE_UTF2CYR (*str, *(str + 1));

            // Продвигаем указатель сразу на 2 байта (пропускаем пару UTF-8)
            str += 2;

            SSD1306_TXT_PutSymbolEx (new_code + 0x20, style);
            real_printed_syms += 1;
        } else {
            // Обычный символ: выводим и сдвигаем указатель на 1
            SSD1306_TXT_PutSymbolEx (*str++, style);
            real_printed_syms += 1;
        }
#else
        SSD1306_TXT_PutSymbolEx (*str++, style);
#endif
    }

    _flag_str_limit = 0;
}
// -----------------------------------------------------------------------------
// Вывод символа на индикатор в позицию курсора
void SSD1306_TXT_PutSymbolEx (uint8_t symbol_code, uint16_t style) {

    // Широкий символ 16x16
    if (is_set_mask(style, SYMSTYLE_WIDE)) {

        SSD1306_TXT_PutWidthSymbol (symbol_code, style);
        return;
    }

    // LED символ 8x16
    if (is_set_mask(style, SYMSTYLE_LED)) {

        SSD1306_TXT_PutDoubleSymbol (symbol_code, style);
        return;
    }

    // Широкий LED символ 8x16
    if (is_set_mask(style, SYMSTYLE_WIDTHLED)) {

        SSD1306_TXT_PutWideDoubleSymbol (symbol_code, style);
        return;
    }

    // Стандартный символ 8x8
    SSD1306_TXT_PutSymbol (symbol_code, style);
}
// -----------------------------------------------------------------------------
// Вывод строки на индикатор в позицию курсора
void SSD1306_TXT_PutWideDoubleSymbol (uint8_t symbol_code, uint16_t style) {

    Params caret_tmp;
    caret_tmp.value1 = screen_options.value1;
    caret_tmp.value2 = screen_options.value2;

    // Если курсор вне поля, ничего не делаем...
    if (screen_options.value1 + 1 >= OLED_DISPLAY_WIDTH_SYM ||
        screen_options.value2 + 1 >= OLED_DISPLAY_HEIGHT_ROW) {

        return;
    }

    SSD1306_TXT_SetCursor (screen_options.value1, screen_options.value2);
    SSD1306_TXT_PutWidthSymbol (symbol_code, SYMSTYLE_NOALIGN | SYMSTYLE_WIDE | style);

    screen_options.value1 = caret_tmp.value1;
    // screen_options.value2 = caret_tmp.value2;

    SSD1306_TXT_SetCursor (screen_options.value1, screen_options.value2 + 2);
    SSD1306_TXT_PutWidthSymbol (symbol_code + 1, SYMSTYLE_NOALIGN | SYMSTYLE_WIDE | style);

    // screen_options.value1 = caret_tmp.value1 + 2;
    // screen_options.value2 = caret_tmp.value2;
    SSD1306_TXT_SetCursor (caret_tmp.value1 + 2, caret_tmp.value2);
}
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_PRINTEX
// Установка ограничений для печати целых значений
void SSD1306_TXT_SetPrIntRestrict(SSD1306_PRINTINT_RESTRICT_MODES mode) {

    _current_intrestr = mode;
}
// -----------------------------------------------------------------------------
// Печать целого десятичного (до 10 цифр [0..4 294 967 295]) числа с текущей позиции курсора
// format = 0, печать полного числа, включающего незначащие нули
// format > 0, незначащие ведущие нули будут опущены
// format = 0xFF, как при format = 0, но ведущие нули будут заменены на пробелы
// point - позиция децимальной точки, 0 - если точка не нужна
// На количество символов влияет значение в регистре _current_intrestr!
void SSD1306_TXT_PrintLongInt (uint32_t value, uint8_t format, uint8_t point, uint16_t style) {

    uint8_t tmp_buffer[10] = {0};
    fill_buffer (value, tmp_buffer, 10);

    uint8_t tx_enable = 0;

    for (uint8_t rx_index = 0; rx_index < 10; rx_index++) {

        if (_current_intrestr != REST_0) {

            if (rx_index < (10 - (uint8_t)_current_intrestr)) { continue; }
        }

        if ((format == 1) && (tmp_buffer[9 - rx_index]) != 0) {
            tx_enable = 1;
        }

        // Вывод символа, если разрешено
        if (tx_enable == 1 || format == 0) {

            tx_enable = 1;
            SSD1306_TXT_PutSymbol ((char)(tmp_buffer[9 - rx_index] + 0x30), style);
        } else {

            if (point > 0 && rx_index == (9 - point)) { } else {
            
                if (format == 0xFF) {
                    
                    if (tmp_buffer[9 - rx_index] != 0) {

                        tx_enable = 1;
                        SSD1306_TXT_PutSymbol ((char)(tmp_buffer[9 - rx_index] + 0x30), style);
                    } else {

                    
                        SSD1306_TXT_PutSymbolEx (' ', style);
                    }
                }
            }
        }

        // Если точка, то вывод будет происходить обязательно
        if (point > 0 && rx_index == (9 - point)) {

            if (tx_enable == 0) {

                SSD1306_TXT_PutSymbolEx (tmp_buffer[9 - rx_index] + 0x30, style);
            }

            SSD1306_TXT_PutSymbolEx ('.', style);
            tx_enable = 1;
        }
    }

    // Если здесь все еще не был разрешен вывод, печатаем ноль...
    if (tx_enable == 0) {

        SSD1306_TXT_PutSymbolEx ('0', style);
    }
}
// -----------------------------------------------------------------------------
// Печать 8-битного числа в HEX формате
void SSD1306_TXT_Print8_hex (uint8_t value, uint16_t style) {

    Params t_hrx_data = _int_to_hex (value);
    SSD1306_TXT_PutSymbolEx (t_hrx_data.value1, style);
    SSD1306_TXT_PutSymbolEx (t_hrx_data.value2, style);
}
// -----------------------------------------------------------------------------
// Печать 16-битного числа в HEX формате
void SSD1306_TXT_Print16_hex (uint16_t value, uint16_t style) {

    uint8_t buffer16[4] = {0};

    fill_buffer_hex16 (value, (uint8_t *)buffer16);

    for (uint8_t index = 0; index < sizeof (buffer16); index++) {

        SSD1306_TXT_PutSymbolEx (buffer16[index], style);
    }
}
// -----------------------------------------------------------------------------
// Печать 32-битного числа в HEX формате
void SSD1306_TXT_Print32_hex (uint32_t value, uint16_t style) {

    uint8_t buffer32[8] = {0};

    fill_buffer_hex32 (value, (uint8_t *)buffer32);

    for (uint8_t index = 0; index < sizeof (buffer32); index++) {

        SSD1306_TXT_PutSymbolEx (buffer32[index], style);
    }
}
#endif
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_NEW_INIT
// Инициализация контроллера и дисплея
// New - Free 3128
void SSD1306_TXT_Init (uint8_t init_contrast) {
    // -------------------------------------------------------------------------
    uint8_t init_byte = 0;
    // -------------------------------------------------------------------------
    I2CInit();
    __delay_ms (500);
    // -------------------------------------------------------------------------
    /* Init LCD */
    // -------------------------------------------------------------------------
    // Первый, проверенный вариант
    for (int init_index = 0; init_index < 28; init_index++) {

        init_byte = display_init_data[init_index];
        if (init_index == 9) {

            init_byte = init_contrast;
        }

        if (init_index == 27) {

            SSD1306_TXT_ClearScreen (1);
        }

        SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, init_byte);
    }
    // -------------------------------------------------------------------------
}
#else
// -----------------------------------------------------------------------------
// Инициализация контроллера и дисплея
void SSD1306_TXT_Init (uint8_t init_contrast) {
    // -------------------------------------------------------------------------
    Delay_Ms (500);
    // -------------------------------------------------------------------------
    /* uint32_t restore_state;
    // 1. Читаем mstatus, сохраняем его и выключаем прерывания
    asm volatile ("csrrci %0, mstatus, 8"
                  : "=r"(restore_state)); */
    // -------------------------------------------------------------------------
/*#ifdef I2C_SPEED_STANDARD_MODE
    IIC_Init (I2C_SPEED_STANDARD_MODE, I2C_SELF_ADDRESS);
#endif
#ifdef I2C_SPEED_FAST_MODE
    IIC_Init (I2C_SELF_ADDRESS);
#endif */
    // -------------------------------------------------------------------------
    // 2. Восстанавливаем состояние (включаем только если они были включены)
    /*asm volatile ("csrs mstatus, %0"
                  :
                  : "r"(restore_state & 0x8)); */
    // -------------------------------------------------------------------------
    // Init LCD
    // -------------------------------------------------------------------------
    Delay_Ms (500);

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAE);  // Дисплей выключен

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x20);  // Режим автоматической адресации
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);  // 0x00 - по горизонтали с переходом на новую страницу (строку)
                                                     // 0x01 - по вертикали с переходом на новую строку
                                                     // 0x02 - только по выбранной странице без перехода

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xB0);  // Старновая страница GDDRAM [0..7]

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xC8);  // Режим сканирования озу дисплея для изменения системы координат
                                                     // С0 - снизу/верх (начало нижний левый угол)
                                                     // С8 - сверху/вниз (начало верний левый угол)

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);  // Установка начального адреса колонки
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x10);  // Установка конечного адреса колонки

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x40);  // Установка начального адреса строки [0..63]

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x81);  // Установка яркости (контрастности) дисплея
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, init_contrast);

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA1);                        // set segment re-map 0 to 127

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA6);                        //--set normal display
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA8);                        //--set multiplex ratio(1 to 64)
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMD_WRITECMD, 0x1F);  //
#endif
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x3F);
#endif

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA4);  // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD3);  //-set display offset
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);  //-not offset

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD5);  //--set display clock divide ratio/oscillator frequency
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xF0);  //--set divide ratio

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD9);  //--set pre-charge period
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x22);  //

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xDA);  //--set com pins hardware configuration
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x02);
#endif
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x12);
#endif

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xDB);  //--set vcomh
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x20);  // 0x20,0.77xVcc

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);  //--set DC-DC enable
    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x14);  //

    SSD1306_TXT_ClearScreen (1);

    SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAF);  //--turn on SSD1306 panel
    // -------------------------------------------------------------------------
}
#endif
// -----------------------------------------------------------------------------
