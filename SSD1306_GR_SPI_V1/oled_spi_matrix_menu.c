/********************************** (C) COPYRIGHT ******************************
 * File Name          : matrix_menu.c
 * Author             : vantr
 * Description        : Библиотека графики для OLED дисплея на SSD1306
 * Module             : Графическая библиотека MATRIX™
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
#include "oled_spi_matrix_menu.h"
#include <sys/_types.h>
#include "string.h"
#include "debug.h"
// -----------------------------------------------------------------------------
// Ссылка на главное меню
OLED_SPI_Menu _mainMenu;
OLED_SPI_MenuItem _items[MENU_MAX_ITEMS];
// -----------------------------------------------------------------------------
// Отступ снизу при отрисовке имени меню
uint8_t _mm_header_margin = 3;
// -----------------------------------------------------------------------------
// Получение количества элементов в меню
uint8_t OLED_SPI_matrix_menu_get_count() {

    return _mainMenu.Count;
}
// -----------------------------------------------------------------------------
// Ссылка на главное меню
OLED_SPI_Menu *OLED_SPI_matrix_menu_mainmenu() {

    return &_mainMenu;
}
// -----------------------------------------------------------------------------
// Быстрое создание элемента меню из параметров
void OLED_SPI_matrix_menu_create_menuitem (uint8_t *caption, uint16_t ActionID) {

    if (_mainMenu.Count < MENU_MAX_ITEMS) {
        
        OLED_SPI_MenuItem result;

        result.ActionID = ActionID;
        result.Caption = caption;
        result.Selected = 0;

        _items[_mainMenu.Count] = result;
        _mainMenu.Count += 1;
    }
}
// -----------------------------------------------------------------------------
// Установка значений по умолчанию
void OLED_SPI_matrix_menu_default_config (Rect client_rect, uint8_t item_height) {

    _mainMenu.MenuName = (uint8_t *)".";
    _mainMenu.State = Newed;  // Только что созданное меню!
    _mainMenu.Scale = 1;
    _mainMenu.MenuHeapWidth = 15;
    _mainMenu.TextMenuNameScale = 1;

    _mainMenu.ViewItemNumber = 1;
    _mainMenu.ItemNumberWidth = 15;

    _mainMenu.MenuItemHeight = (item_height < 10) ? 10 : item_height;
    _mainMenu.MenuRect = client_rect;
    _mainMenu.TopItemIndex = 0;
}
// -----------------------------------------------------------------------------
// Получение индекса выделенного элемента меню
OLED_SPI_MenuItem* OLED_SPI_matrix_menu_get_selected() {
    // -------------------------------------------------------------------------
    for (int index = 0; index < _mainMenu.Count; index++) {

        // Если выделенный существует
        if (_items[index].Selected == 1) {

            return &_items[index];
        }
    }

    return &_items[_mainMenu.DefaultActionIndex];
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка области отрисовки меню
void OLED_SPI_matrix_menu_set_rect(Rect client_rect) {

    _mainMenu.MenuRect = client_rect;
}
// -----------------------------------------------------------------------------
// Установка высоты единичного элемента меню
void OLED_SPI_matrix_menu_set_menuitem_height(uint8_t item_height) {

    _mainMenu.MenuItemHeight = item_height;
}
// -----------------------------------------------------------------------------
// Установка первого отображаемого элемента меню
void OLED_SPI_matrix_menu_set_top_itemindex(uint8_t item_index) {

    _mainMenu.TopItemIndex = item_index;
}
// -----------------------------------------------------------------------------
// Очистка меню
void OLED_SPI_matrix_menu_clear() {

    _mainMenu.Count = 0;
    memset (_items, 0, sizeof (_items));
}
// -----------------------------------------------------------------------------
// Получение ссылки на элемент меню
OLED_SPI_MenuItem* OLED_SPI_matrix_menu_get(uint8_t index) {
    
    if (index < _mainMenu.Count) {
        
        return &_items[index];
    }

    return &_items[0];
}
// -----------------------------------------------------------------------------
// Сделать видимым указанный элемент меню
void matrix_menu_set_visible(uint8_t item_index) {
    // -------------------------------------------------------------------------
    if (item_index + 1 > _mainMenu.Count) {
        return;
    }
    // -------------------------------------------------------------------------
    if (item_index < _mainMenu.TopItemIndex) {
        
        _mainMenu.TopItemIndex = item_index;
        return;
    }
    // -------------------------------------------------------------------------
    int item_top = _mainMenu.TopItemIndex * _mainMenu.MenuItemHeight;
    int item_y = (item_index * _mainMenu.MenuItemHeight);
    int item_b = item_y + _mainMenu.MenuItemHeight;
    
    if (item_b - item_top > _mainMenu.MenuRect.ClientSize.Height) {
        
        _mainMenu.TopItemIndex = (((item_b - _mainMenu.MenuRect.ClientSize.Height) / _mainMenu.MenuItemHeight)) + 1;
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка выделенного элемента меню
void OLED_SPI_matrix_menu_set_selected(uint8_t item_index) {
    // -------------------------------------------------------------------------
    if (item_index < _mainMenu.Count) {
        // -------------------------------------------------------------------------
        for (int index = 0; index < _mainMenu.Count; index++) {

            _items[index].Selected = (item_index == index) ? 1 : 0;
        }
        // -------------------------------------------------------------------------
        matrix_menu_set_visible (item_index);
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка имени меню
void OLED_SPI_matrix_menu_set_name(uint8_t * caption) {
    
    _mainMenu.MenuName = caption;
}
// -----------------------------------------------------------------------------
// Установка отступа снизу для имени меню
void OLED_SPI_matrix_menuname_set_margin(uint8_t value) {

    _mm_header_margin = value;
}
// -----------------------------------------------------------------------------
// Установка курсора на текст
void matrix_menu_itemtext_coord(OLED_SPI_MenuItem item) {

    Point textc = OLED_SPI_matrix_shift_coords (item.Location, 2, 2);
    OLED_SPI_matrix_setCoord2 (textc);
}
// -----------------------------------------------------------------------------
// Рисование главного меню
void OLED_SPI_matrix_menu_draw() {
    // -------------------------------------------------------------------------
    OLED_SPI_matrix_frame_init();
    // -------------------------------------------------------------------------
    Rect current_rect = _mainMenu.MenuRect;

    OLED_SPI_matrix_setBrush (1);
    // -------------------------------------------------------------------------
    if (_mainMenu.Count == 0) {

        OLED_SPI_matrix_fill_rect (current_rect, 0);
        OLED_SPI_matrix_client_rect();

        return;
    }
    // -------------------------------------------------------------------------
    Rect menuitems_area;

    menuitems_area.Location = current_rect.Location;
    menuitems_area.ClientSize = current_rect.ClientSize;

    if (_mainMenu.MenuName != (uint8_t *)'.') {

        menuitems_area.ClientSize.Width -= _mainMenu.MenuHeapWidth;
    }
    // -------------------------------------------------------------------------
    Rect heap_area;

    OLED_SPI_matrix_set_clientrect (_mainMenu.MenuRect);

    heap_area.Location = OLED_SPI_matrix_get_corner (menuitems_area, TopRight);
    heap_area.ClientSize = (Size){_mainMenu.MenuHeapWidth, menuitems_area.ClientSize.Height};
    // -------------------------------------------------------------------------
    uint8_t y_current = current_rect.Location.Y;
    uint8_t item_index = _mainMenu.TopItemIndex;

    Size item_size;
    item_size.Width = menuitems_area.ClientSize.Width - 1;
    item_size.Height = _mainMenu.MenuItemHeight;
    // -------------------------------------------------------------------------
    while (y_current < current_rect.Location.Y + current_rect.ClientSize.Height) {

        if (item_index > _mainMenu.Count - 1) {
            break;
        }

        _items[item_index].Location = OLED_SPI_matrix_create_point (current_rect.Location.X, y_current);
        Rect item_rect = OLED_SPI_matrix_create_rect (_items[item_index].Location, item_size);
        item_rect = OLED_SPI_matrix_reduce_rect (item_rect, 4);
        // ---------------------------------------------------------------------
        if (_items[item_index].Selected) {

            OLED_SPI_matrix_filled_rect (_items[item_index].Location, item_size);
            OLED_SPI_matrix_setBrush (0);
        } else {

            OLED_SPI_matrix_setBrush (1);
        }

        OLED_SPI_matrix_set_textContext_prop (TX_Direction3, WS_Horizonlal);
        OLED_SPI_matrix_set_textContext_prop (TX_Scale3, _mainMenu.Scale);
        OLED_SPI_matrix_set_textContext_prop (TX_Align3, WA_Left);

        // 2. Если включен показ номеров — делим область на две части
        if (_mainMenu.ViewItemNumber > 0) {
            // Выделяем фиксированную "ячейку" под номер слева
            Rect n_area = item_rect;
            n_area.ClientSize.Width = _mainMenu.ItemNumberWidth - 5;

            // Сдвигаем основную область текста вправо (она станет чуть меньше)
            item_rect.Location.X += _mainMenu.ItemNumberWidth;
            item_rect.ClientSize.Width -= _mainMenu.ItemNumberWidth;

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
            OLED_SPI_matrix_set_textContext_prop (TX_Align3, WA_Right);
            OLED_SPI_matrix_drawStringInscribed_Smart (num_buf, n_area);
        }

        OLED_SPI_matrix_set_textContext_prop (TX_Align3, WA_Left);
        OLED_SPI_matrix_drawStringInscribed_Smart ((uint8_t *)_items[item_index].Caption, item_rect);
        //  ---------------------------------------------------------------------
        item_index++;
        y_current += _mainMenu.MenuItemHeight;
        // ---------------------------------------------------------------------
    }
    // -------------------------------------------------------------------------
    // Отрисовка боковой панели (имени) меню
    if (_mainMenu.MenuName != (uint8_t *)'.') {

        if (_mainMenu.MenuName[0] != '.') {

            OLED_SPI_matrix_setBrush (1);
            OLED_SPI_matrix_fill_rect (heap_area, 0);
            // ---------------------------------------------------------------------
            // Печать заголовка
            OLED_SPI_matrix_set_textContext_prop (TX_Direction3, WS_Vertical);
            OLED_SPI_matrix_set_textContext_prop (TX_Scale3, _mainMenu.TextMenuNameScale);
            OLED_SPI_matrix_set_textContext_prop (TX_Align3, WA_Center);

            OLED_SPI_matrix_drawStringInscribed_Smart ((uint8_t *)_mainMenu.MenuName, heap_area);
            OLED_SPI_matrix_draw_rect (heap_area.Location, heap_area.ClientSize);
        }
    }
    // -------------------------------------------------------------------------
    OLED_SPI_matrix_draw_rect (current_rect.Location, current_rect.ClientSize);
    OLED_SPI_matrix_refresh();
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------