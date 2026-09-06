/********************************** (C) COPYRIGHT ******************************
 * File Name          : matrix_menu.c
 * Author             : vantr
 * Description        : Библиотека для создания экранных меню
 * Module             : Графическая библиотека MATRIX™
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
#include "matrix_menu.h"

#include "SSS_Common_Lib_V1/sss_math.h"
#include "string.h"
#include "debug.h"
// -----------------------------------------------------------------------------
// Ссылка на главное меню
TFT_Menu _mainMenu;
// -----------------------------------------------------------------------------
// Ссылка на элементы меню
TFT_MenuItem _items[MENU_MAX_ITEMS];
// -----------------------------------------------------------------------------
// Индексы выделения для каждого уровня меню
static uint8_t _sel_level_data[MAX_MENU_LEVELS] = {0};
// -----------------------------------------------------------------------------
// Текущий уровень меню
static uint8_t _current_menu_level = 0;
// -----------------------------------------------------------------------------
// Отступ снизу при отрисовке имени меню
uint8_t _mm_header_margin = 10;
// -----------------------------------------------------------------------------
// Флаг изменения меню
uint8_t _mm_changed = 0;
// -----------------------------------------------------------------------------
// Контекст модального окна для меню
TextContext2 modal_window_text_context;
// -----------------------------------------------------------------------------
// Получение количества элементов в меню
uint8_t TFT_matrix_menu_get_count() {

    return _mainMenu.Count;
}

// -----------------------------------------------------------------------------
// Ссылка на главное меню
TFT_Menu *TFT_matrix_menu_mainmenu() {

    return &_mainMenu;
}

// -----------------------------------------------------------------------------
// Быстрое создание элемента меню из параметров
uint8_t TFT_matrix_menu_create_menuitem (uint8_t *caption, uint16_t ActionID) {

    if (_mainMenu.Count < MENU_MAX_ITEMS) {

        TFT_MenuItem result;

        result.ActionID = ActionID;
        result.Caption = caption;
        result.Selected = 0;
        result.ColorText = _mainMenu.ColorText;

        _items[_mainMenu.Count] = result;
        _mainMenu.Count += 1;

        _mm_changed = 1;
        return _mainMenu.Count - 1;
    }

    return 0xFF;
}

// -----------------------------------------------------------------------------
// Быстрое создание элемента меню из параметров
uint8_t TFT_matrix_menu_create_menuitem2 (uint8_t *caption, uint16_t ActionID, uint16_t color) {

    uint16_t mitem_idx = TFT_matrix_menu_create_menuitem (caption, ActionID);

    if (mitem_idx != 0xFF) {

        _items[mitem_idx].ColorText = color;
        return mitem_idx;
    }

    return 0xFF;
}
// -----------------------------------------------------------------------------
// Создание и регистрация нового корневого меню: 1 - успешное, 0 - неудачное
uint8_t TFT_matrix_menu_create_new_root_menu(uint8_t select_zero) {

    if (_current_menu_level < MAX_MENU_LEVELS) {
        // Инициализация графики
        TFT_matrix_frame_init();

        // Очищаем старый экран
        TFT_matrix_menu_clear();

        // Уровень меню - 0
        _current_menu_level = 0;
        _mainMenu.Level = 0;

        uint8_t old_sel = _sel_level_data[0];
        // Очистка буфера выделения
        reset_buffer (_sel_level_data, 0, MAX_MENU_LEVELS);

        if (select_zero == 0) {
         
            _sel_level_data[0] = old_sel;
        }

        return 1;
    }

    return 0;
}

// -----------------------------------------------------------------------------
// Создание и регистрация нового меню: 1 - успешное, 0 - неудачное
uint8_t TFT_matrix_menu_create_new_menu() {

    if (_current_menu_level < MAX_MENU_LEVELS) {
        // Инициализация графики
        TFT_matrix_frame_init();

        // Очищаем старый экран
        TFT_matrix_menu_clear();

        // Уровень меню
        _current_menu_level += 1;
        _mainMenu.Level = _current_menu_level;

        // Очистка буфера выделения
        _sel_level_data[_current_menu_level] = 0;

        return 1;
    }

    return 0;
}

// -----------------------------------------------------------------------------
// Создание элементов меню завершено
void TFT_matrix_menu_create_menu_complete() {

    // 1. Автоматически выставляем фокус на индекс, который сохранен для текущего уровня.
    // Если пришли впервые сверху — там гарантированно будет 0 (благодаря опережающей очистке).
    // Если вернулись назад через close_menu — там будет лежать старый индекс этого уровня!
    TFT_matrix_menu_set_selected (_sel_level_data[_mainMenu.Level]);

    // 2. Переводим меню в рабочий режим отрисовки и опроса кнопок
    _mainMenu.State = Running;

    // 3. Выставляем флаг изменений, чтобы движок отрисовал кадр на нужной позиции
    _mm_changed = 1;
}

// -----------------------------------------------------------------------------
// Закрытие без уничтожения текущего меню при возврате на предыдущий уровень
void TFT_matrix_menu_close_menu() {

    if (_current_menu_level > 0) {

        // Мы уходим с текущего уровня навсегда, его история на этом уровне больше не нужна
        //_sel_level_data[_current_menu_level] = 0;

        // Делаем шаг НАЗАД по иерархии дерева
        _current_menu_level -= 1;

        _mainMenu.Level = _current_menu_level;
    }
}

// -----------------------------------------------------------------------------
// Установка значений по умолчанию
void TFT_matrix_menu_default_config (Rect client_rect, uint8_t item_height) {

    _mainMenu.MenuName = (uint8_t *)".";
    _mainMenu.State = Newed;  // Только что созданное меню!

    _mainMenu.ColorBackground = CL_BLACK;
    _mainMenu.ColorSelectedBackground = CL_WHITE;
    _mainMenu.ColorText = CL_WHITE;
    _mainMenu.ColorBorder = CL_WHITE;
    _mainMenu.ColorSelectedText = CL_BLACK;
    _mainMenu.ColorMenuHeap = CL_BLUE;
    _mainMenu.ColorMenuHeapText = CL_YELLOW;

    _mainMenu.MenuItemHeight = (item_height < 25) ? 25 : item_height;
    _mainMenu.MenuRect = client_rect;
    _mainMenu.TopItemIndex = 0;
    _mainMenu.TextScale = 1;
    _mainMenu.TextMenuNameScale = 2;
    _mainMenu.TextSpace = 1;
    _mainMenu.TextMenuSpace = 1;

    _mainMenu.ItemNumberWidth = 16;
    _mainMenu.ViewItemNumber = 0;

    _mm_changed = 0;
}

// -----------------------------------------------------------------------------
// Установка области отрисовки меню
void TFT_matrix_menu_set_rect (Rect client_rect) {

    _mainMenu.MenuRect = client_rect;
    _mm_changed = 1;
}

// -----------------------------------------------------------------------------
// Установка высоты единичного элемента меню
void TFT_matrix_menu_set_menuitem_height (uint8_t item_height) {

    _mainMenu.MenuItemHeight = (item_height < 25) ? 25 : item_height;
    _mm_changed = 1;
}

// -----------------------------------------------------------------------------
// Установка первого отображаемого элемента меню
void TFT_matrix_menu_set_top_itemindex (uint8_t item_index) {

    _mainMenu.TopItemIndex = item_index;
    _mm_changed = 1;
}

// -----------------------------------------------------------------------------
// Установка цвета текста/выделенного текста
void TFT_matrix_menu_set_text_color (uint16_t color, uint8_t selected) {

    switch (selected) {

    case 0:

        _mainMenu.ColorText = color;
        break;

    default:

        _mainMenu.ColorSelectedText = color;
        break;
    }

    _mm_changed = 1;
}

// -----------------------------------------------------------------------------
// Установка цвета неактивного/выделенного заднего плана
void TFT_matrix_menu_set_back_color (uint16_t color, uint8_t selected) {

    switch (selected) {

    case 0:

        _mainMenu.ColorBackground = color;
        break;

    default:

        _mainMenu.ColorSelectedBackground = color;
        break;
    }

    _mm_changed = 1;
}

// -----------------------------------------------------------------------------
// Очистка меню
void TFT_matrix_menu_clear() {

    _mainMenu.Count = 0;
    memset (_items, 0, sizeof (_items));

    TFT_matrix_textcontext_default (&modal_window_text_context);

    _mm_changed = 1;
}

// -----------------------------------------------------------------------------
// Получение ссылки на элемент меню
TFT_MenuItem *TFT_matrix_menu_get (uint8_t index) {

    if (index < _mainMenu.Count) {

        return &_items[index];
    }

    return NULL;
}
// -----------------------------------------------------------------------------
// Сделать видимым указанный элемент меню
void matrix_menu_set_visible (uint8_t item_index) {
    // -------------------------------------------------------------------------
    if (item_index >= _mainMenu.Count)
        return;

    // 1. Если элемент выше текущего окна просмотра — скроллим вверх до него
    if (item_index < _mainMenu.TopItemIndex) {
        _mainMenu.TopItemIndex = item_index;

        _mm_changed = 1;
        return;
    }

    // 2. Рассчитываем, сколько ПОЛНЫХ пунктов влезает в высоту окна
    uint8_t visible_count = _mainMenu.MenuRect.ClientSize.Height / _mainMenu.MenuItemHeight;

    // 3. Если элемент ниже текущего окна просмотра
    // (item_index - TopItemIndex + 1) — это порядковый номер элемента на экране
    if (item_index - _mainMenu.TopItemIndex + 1 > visible_count) {
        // Устанавливаем TopItemIndex так, чтобы item_index был САМЫМ НИЖНИМ в окне
        _mainMenu.TopItemIndex = item_index - visible_count + 1;

        _mm_changed = 1;
    }
}
// -----------------------------------------------------------------------------
// Установка выделенного элемента меню
void TFT_matrix_menu_set_selected (uint8_t item_index) {
    // -------------------------------------------------------------------------
    if (item_index + 1 > _mainMenu.Count) {

        item_index = 0;
    }
    // -------------------------------------------------------------------------
    for (int index = 0; index < _mainMenu.Count; index++) {

        _items[index].Selected = (item_index == index) ? 1 : 0;
    }
    // -------------------------------------------------------------------------
    _sel_level_data[_mainMenu.Level] = item_index;

    matrix_menu_set_visible (item_index);
    _mm_changed = 1;
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Получение индекса выделенного элемента меню
int16_t TFT_matrix_menu_get_selected() {
    // -------------------------------------------------------------------------
    return _sel_level_data[_mainMenu.Level];
}
// -----------------------------------------------------------------------------
// Получение индекса выделенного для указанного уровня
int16_t TFT_matrix_menu_get_seldata(uint8_t Level) {
    // -------------------------------------------------------------------------
    return _sel_level_data[Level];
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Получение выделенного элемента меню
TFT_MenuItem *TFT_matrix_menu_get_selected_item() {
    // -------------------------------------------------------------------------
    for (int index = 0; index < _mainMenu.Count; index++) {

        // Если выделенный существует
        if (_items[index].Selected == 1) {

            return &_items[index];
        }
    }

    // Если не существует, возвращаем элемент по умолчанию
    if (_mainMenu.DefaultActionIndex < _mainMenu.Count) {

        return &_items[_mainMenu.DefaultActionIndex];
    }
    // -------------------------------------------------------------------------
    // Увы, ничего не нашли
    return NULL;
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка имени меню
void TFT_matrix_menu_set_name (uint8_t *caption) {

    _mainMenu.MenuName = caption;
    _mm_changed = 1;
}
// -----------------------------------------------------------------------------
// Установка отступа снизу для имени меню
void TFT_matrix_menuname_set_margin (uint8_t value) {

    _mm_header_margin = value;
    _mm_changed = 1;
}
// -----------------------------------------------------------------------------
// Установка курсора на текст
void matrix_menu_itemtext_coord (TFT_MenuItem item, uint8_t scale) {

    Point textc = TFT_matrix_shift_coords (item.Location, 5, ((_mainMenu.MenuItemHeight - 8 * scale) / 2));
    TFT_matrix_setCoord2 (textc);
}
// -----------------------------------------------------------------------------
// Показать модальное окно поверх меню
void TFT_matrix_menu_show_modal_windows (uint8_t *text, uint16_t fore_color, uint16_t back_color) {

    Size text_size = TFT_matrix_measureString (text);
    
    if (text_size.Width < TFT_DISPLAY_WIDTH && text_size.Height < TFT_DISPLAY_HEIGHT) {

        Point zero = {0, 0};
        Rect text_rect = TFT_matrix_center_rect (TFT_matrix_get_clientrect(), TFT_matrix_create_rect (zero, text_size));
        Rect area_rect = TFT_matrix_increase_rect (text_rect, 12);
        Rect rect_rect = TFT_matrix_increase_rect (text_rect, 10);

        TFT_matrix_set_otherContext_prop (&modal_window_text_context, TX_Direction, WS_Horizonlal);
        TFT_matrix_set_otherContext_prop (&modal_window_text_context, TX_Align, WA_Center);
        TFT_matrix_set_otherContext_prop (&modal_window_text_context, TX_ForeColor, fore_color);
        TFT_matrix_set_otherContext_prop (&modal_window_text_context, TX_BackColor, back_color);
        TFT_matrix_set_otherContext_prop (&modal_window_text_context, TX_Scale, 2);
        TFT_matrix_set_otherContext_prop (&modal_window_text_context, TX_Wrap, WM_None);
        TFT_matrix_set_otherContext_prop (&modal_window_text_context, TX_Space, 3);

        TFT_matrix_copy_textContext (&modal_window_text_context);

        TFT_matrix_fillRect (area_rect, BackGround);
        TFT_matrix_draw_rect (rect_rect.Location, rect_rect.ClientSize);

        TFT_matrix_drawStringInscribed_Smart (text, text_rect);
    }
}

// -----------------------------------------------------------------------------
// Принудительное обновление меню
void TFT_matrix_menu_refresh_now() {

    _mm_changed = 1;
    TFT_matrix_menu_draw();
}
// -----------------------------------------------------------------------------
// Альтернативный метод рисования главного меню
void TFT_matrix_menu_draw() {
    // -------------------------------------------------------------------------
    if (_mm_changed == 0) {
        return;
    }
    // -------------------------------------------------------------------------
    TFT_matrix_restoreClientRect();
    Rect current_rect = _mainMenu.MenuRect;
    // -------------------------------------------------------------------------
    if (_mainMenu.Count == 0) {

        ST7789_setForeColor (_mainMenu.ColorBackground);
        TFT_matrix_fillRect (current_rect, ForeGround);
        TFT_matrix_client_rect();

        return;
    }
    // -------------------------------------------------------------------------
    Rect menuitems_area;

    menuitems_area.Location = current_rect.Location;
    menuitems_area.ClientSize = current_rect.ClientSize;

    if (_mainMenu.MenuName != (uint8_t *)'.') {

        menuitems_area.ClientSize.Width -= MENU_NAME_WIDTH;
    }

    TFT_matrix_set_clientrect (TFT_matrix_reduce_rect (menuitems_area, 1));
    current_rect = TFT_matrix_get_clientrect();
    // -------------------------------------------------------------------------
    Rect heap_area;

    TFT_matrix_set_clientrect (_mainMenu.MenuRect);

    heap_area.Location = TFT_matrix_get_rect_topright (menuitems_area);
    heap_area.ClientSize = (Size){MENU_NAME_WIDTH, menuitems_area.ClientSize.Height};
    // -------------------------------------------------------------------------
    uint8_t y_current = current_rect.Location.Y;
    uint8_t item_index = _mainMenu.TopItemIndex;

    Size item_size;
    item_size.Width = menuitems_area.ClientSize.Width - 2;
    item_size.Height = _mainMenu.MenuItemHeight;
    // -------------------------------------------------------------------------
    // 1. Отрисовка пунктов меню
    // -------------------------------------------------------------------------
    while (y_current < current_rect.Location.Y + current_rect.ClientSize.Height) {

        if (item_index > _mainMenu.Count - 1) {

            break;
        }
        // ---------------------------------------------------------------------
        // Не помещающийся в область меню последний пункт не рисуем вовсе
        if (y_current >= current_rect.Location.Y + current_rect.ClientSize.Height) {

            break;
        }
        // ---------------------------------------------------------------------
        _items[item_index].Location = TFT_matrix_create_point (menuitems_area.Location.X + 1, y_current);
        // ---------------------------------------------------------------------
        if (_items[item_index].Selected) {

            // Выделенный пункт меню
            // ST7789_setForeColor(_mainMenu.ColorSelectedBackground);
            // ST7789_setBackColor(_mainMenu.ColorSelectedText);
            ST7789_setForeColor (_items[item_index].ColorText);
            ST7789_setBackColor (_mainMenu.ColorSelectedText);

            _items[item_index].Location.extend = 2;
            TFT_matrix_hatching_rect (_items[item_index].Location, item_size);

            uint16_t delta_h = (item_size.Height - (_mainMenu.TextScale * 8)) + 2;
            Rect item_rect = TFT_matrix_create_rect (_items[item_index].Location, item_size);
            Rect f_rect = TFT_matrix_reduce_rect_height (item_rect, delta_h);
            TFT_matrix_fillRect3 (f_rect.Location, f_rect.ClientSize, ForeGround);

            ST7789_setForeColor (_mainMenu.ColorSelectedText);
            ST7789_setBackColor (_mainMenu.ColorSelectedBackground);

        } else {

            // Не выделенный пункт меню
            ST7789_setForeColor (_mainMenu.ColorBackground);
            ST7789_setBackColor (_mainMenu.ColorText);
            TFT_matrix_fillRect3 (_items[item_index].Location, item_size, ForeGround);

            ST7789_setForeColor (_items[item_index].ColorText);
            // ST7789_setBackColor(_mainMenu.ColorBackground);
            ST7789_setBackColor (_items[item_index].ColorText);
        }

        // Надпись на текущем пункте меню
        matrix_menu_itemtext_coord (_items[item_index], _mainMenu.TextScale);
        TFT_matrix_set_textContext_prop (TX_Scale, _mainMenu.TextScale);
        TFT_matrix_set_textContext_prop (TX_Direction, WS_Horizonlal);
        TFT_matrix_set_textContext_prop (TX_Align, WA_Left);

        Rect t_area = TFT_matrix_create_rect2 (5, y_current, item_size.Width - 5, item_size.Height);

        // 2. Если включен показ номеров — делим область на две части
        if (_mainMenu.ViewItemNumber > 0) {
            // Выделяем фиксированную "ячейку" под номер слева
            Rect n_area = t_area;
            n_area.ClientSize.Width = _mainMenu.ItemNumberWidth - 5;

            // Сдвигаем основную область текста вправо (она станет чуть меньше)
            t_area.Location.X += _mainMenu.ItemNumberWidth;
            t_area.ClientSize.Width -= _mainMenu.ItemNumberWidth;

            // Готовим строку номера
            uint8_t num_buf[5];
            Params num_tmp = _int_to_str (item_index + 1);  // +1 для человеческого счета
            uint8_t *p = num_buf;

            if (num_tmp.value1 != 0) {
                *p++ = num_tmp.value1 + '0';
            }
            *p++ = num_tmp.value2 + '0';
            *p = '\0';

            // Печать номера (в свою ячейку n_area)
            TFT_matrix_set_textContext_prop (TX_Align, WA_Right);
            TFT_matrix_drawStringInscribed_Smart (num_buf, n_area);
        }

        TFT_matrix_set_textContext_prop (TX_Align, WA_Left);
        TFT_matrix_set_textContext_prop (TX_Space, _mainMenu.TextSpace);
        TFT_matrix_drawStringInscribed_Smart ((uint8_t *)_items[item_index].Caption, t_area);
        // ---------------------------------------------------------------------
        // Переходим к следующему пункту
        item_index++;
        y_current += _mainMenu.MenuItemHeight;
        // ---------------------------------------------------------------------
    }
    // -------------------------------------------------------------------------
    // 2. ЗАЛИВКА ОСТАТКА (Пустое место под последним отрисованным пунктом)
    // -------------------------------------------------------------------------
    // Вычисляем, сколько пикселей осталось до нижнего края client_rect
    int16_t bottom_edge = current_rect.Location.Y + current_rect.ClientSize.Height;
    int16_t diff_y = bottom_edge - y_current;

    if (diff_y > 0) {
        // Формируем Rect для "подвала" меню
        Rect tail_rect;
        tail_rect.Location.X = current_rect.Location.X;
        tail_rect.Location.Y = y_current;
        tail_rect.ClientSize.Width = item_size.Width;
        tail_rect.ClientSize.Height = (uint16_t)diff_y;

        ST7789_setForeColor (_mainMenu.ColorBackground);
        TFT_matrix_fillRect (tail_rect, ForeGround);
    }
    // -------------------------------------------------------------------------
    // 3. Отрисовка боковой панели (имени) меню
    TFT_matrix_set_clientrect (_mainMenu.MenuRect);

    if (_mainMenu.MenuName[0] != '.') {

        ST7789_setForeColor (_mainMenu.ColorMenuHeap);
        TFT_matrix_fillRect (heap_area, ForeGround);
        // ---------------------------------------------------------------------
        ST7789_setForeColor (_mainMenu.ColorMenuHeapText);
        ST7789_setBackColor (_mainMenu.ColorMenuHeapText);

        // Печать заголовка
        TFT_matrix_set_textContext_prop (TX_Direction, WS_Vertical);
        TFT_matrix_set_textContext_prop (TX_Scale, _mainMenu.TextMenuNameScale);
        TFT_matrix_set_textContext_prop (TX_Align, WA_Center);
        TFT_matrix_set_textContext_prop (TX_Space, _mainMenu.TextMenuSpace);

        TFT_matrix_drawStringInscribed_Smart ((uint8_t *)_mainMenu.MenuName, heap_area);
    }
    // -------------------------------------------------------------------------
    // 4. Рисуем общую рамку области отрисовки
    current_rect = TFT_matrix_get_clientrect();
    ST7789_setForeColor (_mainMenu.ColorBorder);
    TFT_matrix_draw_rect (current_rect.Location, current_rect.ClientSize);
    // -------------------------------------------------------------------------
    _mm_changed = 0;
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------