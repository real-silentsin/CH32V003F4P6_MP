/********************************** (C) COPYRIGHT ******************************
 * File Name          : matrix_menu.c
 * Author             : vantr
 * Description        : Библиотека графики для OLED дисплея на SSD1306
 * Module             : Графическая библиотека MATRIX™
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
#include "oled_matrix_menu.h"
#include <sys/_types.h>
#include "string.h"
#include "debug.h"
// -----------------------------------------------------------------------------
// Ссылка на главное меню
_OLEDMenu _oled_mainMenu;
_OLEDMenuItem _oled_items[MENU_MAX_ITEMS];
// -----------------------------------------------------------------------------
// Отступ снизу при отрисовке имени меню
uint8_t _mm_header_margin2 = 3;
// -----------------------------------------------------------------------------
// Получение количества элементов в меню
uint8_t oled_oled_matrix_menu_get_count() {

    return _oled_mainMenu.Count;
}
// -----------------------------------------------------------------------------
// Ссылка на главное меню
_OLEDMenu *oled_matrix_menu_mainmenu() {

    return &_oled_mainMenu;
}
// -----------------------------------------------------------------------------
// Быстрое создание элемента меню из параметров
void oled_matrix_menu_create_menuitem (uint8_t *caption, uint16_t ActionID) {

    if (_oled_mainMenu.Count < MENU_MAX_ITEMS) {
        
        _OLEDMenuItem result;

        result.ActionID = ActionID;
        result.Caption = caption;
        result.Selected = 0;

        _oled_items[_oled_mainMenu.Count] = result;
        _oled_mainMenu.Count += 1;
    }
}
// -----------------------------------------------------------------------------
// Установка значений по умолчанию
void oled_matrix_menu_default_config (Rect oled_client_rect, uint8_t item_height) {

    _oled_mainMenu.MenuName = (uint8_t *)".";
    _oled_mainMenu.State = Newed;  // Только что созданное меню!
    _oled_mainMenu.Scale = 1;
    _oled_mainMenu.MenuHeapWidth = 15;
    _oled_mainMenu.TextMenuNameScale = 1;

    _oled_mainMenu.ViewItemNumber = 1;
    _oled_mainMenu.ItemNumberWidth = 15;

    _oled_mainMenu.MenuItemHeight = (item_height < 10) ? 10 : item_height;
    _oled_mainMenu.MenuRect = oled_client_rect;
    _oled_mainMenu.TopItemIndex = 0;
}
// -----------------------------------------------------------------------------
// Получение индекса выделенного элемента меню
_OLEDMenuItem* oled_matrix_menu_get_selected() {
    // -------------------------------------------------------------------------
    for (int index = 0; index < _oled_mainMenu.Count; index++) {

        // Если выделенный существует
        if (_oled_items[index].Selected == 1) {

            return &_oled_items[index];
        }
    }

    return &_oled_items[_oled_mainMenu.DefaultActionIndex];
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка области отрисовки меню
void oled_matrix_menu_set_rect(Rect oled_client_rect) {

    _oled_mainMenu.MenuRect = oled_client_rect;
}
// -----------------------------------------------------------------------------
// Установка высоты единичного элемента меню
void oled_matrix_menu_set_menuitem_height(uint8_t item_height) {

    _oled_mainMenu.MenuItemHeight = item_height;
}
// -----------------------------------------------------------------------------
// Установка первого отображаемого элемента меню
void oled_matrix_menu_set_top_itemindex(uint8_t item_index) {

    _oled_mainMenu.TopItemIndex = item_index;
}
// -----------------------------------------------------------------------------
// Очистка меню
void oled_matrix_menu_clear() {

    _oled_mainMenu.Count = 0;
    memset (_oled_items, 0, sizeof (_oled_items));
}
// -----------------------------------------------------------------------------
// Получение ссылки на элемент меню
_OLEDMenuItem* oled_matrix_menu_get(uint8_t index) {
    
    if (index < _oled_mainMenu.Count) {
        
        return &_oled_items[index];
    }

    return &_oled_items[0];
}
// -----------------------------------------------------------------------------
// Сделать видимым указанный элемент меню
void oled_matrix_menu_set_visible(uint8_t item_index) {
    // -------------------------------------------------------------------------
    if (item_index + 1 > _oled_mainMenu.Count) {
        return;
    }
    // -------------------------------------------------------------------------
    if (item_index < _oled_mainMenu.TopItemIndex) {
        
        _oled_mainMenu.TopItemIndex = item_index;
        return;
    }
    // -------------------------------------------------------------------------
    int item_top = _oled_mainMenu.TopItemIndex * _oled_mainMenu.MenuItemHeight;
    int item_y = (item_index * _oled_mainMenu.MenuItemHeight);
    int item_b = item_y + _oled_mainMenu.MenuItemHeight;
    
    if (item_b - item_top > _oled_mainMenu.MenuRect.ClientSize.Height) {
        
        _oled_mainMenu.TopItemIndex = (((item_b - _oled_mainMenu.MenuRect.ClientSize.Height) / _oled_mainMenu.MenuItemHeight)) + 1;
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка выделенного элемента меню
void oled_matrix_menu_set_selected(uint8_t item_index) {
    // -------------------------------------------------------------------------
    if (item_index < _oled_mainMenu.Count) {
        // -------------------------------------------------------------------------
        for (int index = 0; index < _oled_mainMenu.Count; index++) {

            _oled_items[index].Selected = (item_index == index) ? 1 : 0;
        }
        // -------------------------------------------------------------------------
        oled_matrix_menu_set_visible (item_index);
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка имени меню
void oled_matrix_menu_set_name(uint8_t * caption) {
    
    _oled_mainMenu.MenuName = caption;
}
// -----------------------------------------------------------------------------
// Установка отступа снизу для имени меню
void oled_matrix_menuname_set_margin(uint8_t value) {

    _mm_header_margin2 = value;
}
// -----------------------------------------------------------------------------
/* // Установка курсора на текст
void matrix_menu_itemtext_coord(_OLEDMenuItem item) {

    Point textc = oled_matrix_shift_coords(item.Location, 2, 2);
    oled_matrix_setCoord2(textc);
} */
// -----------------------------------------------------------------------------
// Рисование главного меню
void oled_matrix_menu_draw() {
    // -------------------------------------------------------------------------
    oled_matrix_frame_init();
    // -------------------------------------------------------------------------
    Rect current_rect = _oled_mainMenu.MenuRect;

    oled_matrix_setBrush (1);
    // -------------------------------------------------------------------------
    if (_oled_mainMenu.Count == 0) {

        oled_matrix_fill_rect(current_rect, 0);
        oled_matrix_client_rect();

        return;
    }
    // -------------------------------------------------------------------------
    Rect menuitems_area;

    menuitems_area.Location = current_rect.Location;
    menuitems_area.ClientSize = current_rect.ClientSize;

    if (_oled_mainMenu.MenuName != (uint8_t *)'.') {

        menuitems_area.ClientSize.Width -= _oled_mainMenu.MenuHeapWidth;
    }
    // -------------------------------------------------------------------------
    Rect heap_area;

    oled_matrix_set_clientrect (_oled_mainMenu.MenuRect);

    heap_area.Location = oled_matrix_get_corner(menuitems_area, TopRight);
    heap_area.ClientSize = (Size){_oled_mainMenu.MenuHeapWidth, menuitems_area.ClientSize.Height};
    // -------------------------------------------------------------------------
    uint8_t y_current = current_rect.Location.Y;
    uint8_t item_index = _oled_mainMenu.TopItemIndex;

    Size item_size;
    item_size.Width = menuitems_area.ClientSize.Width - 1;
    item_size.Height = _oled_mainMenu.MenuItemHeight;
    // -------------------------------------------------------------------------
    while (y_current < current_rect.Location.Y + current_rect.ClientSize.Height) {

        if (item_index > _oled_mainMenu.Count - 1) {
            break;
        }

        _oled_items[item_index].Location = oled_matrix_create_point (current_rect.Location.X, y_current);
        Rect item_rect = oled_matrix_create_rect (_oled_items[item_index].Location, item_size);
        item_rect = oled_matrix_reduce_rect(item_rect, 4);
        // ---------------------------------------------------------------------
        if (_oled_items[item_index].Selected) {

            oled_matrix_filled_rect (_oled_items[item_index].Location, item_size);
            oled_matrix_setBrush (0);
        } else {

            oled_matrix_setBrush (1);
        }

        oled_matrix_set_textContext_prop(TX_Direction2, WS_Horizonlal);
        oled_matrix_set_textContext_prop(TX_Scale2, _oled_mainMenu.Scale);
        oled_matrix_set_textContext_prop(TX_Align2, WA_Left);

        // 2. Если включен показ номеров — делим область на две части
        if (_oled_mainMenu.ViewItemNumber > 0) {
            // Выделяем фиксированную "ячейку" под номер слева
            Rect n_area = item_rect;
            n_area.ClientSize.Width = _oled_mainMenu.ItemNumberWidth - 5;

            // Сдвигаем основную область текста вправо (она станет чуть меньше)
            item_rect.Location.X += _oled_mainMenu.ItemNumberWidth;
            item_rect.ClientSize.Width -= _oled_mainMenu.ItemNumberWidth;

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
            oled_matrix_set_textContext_prop (TX_Align2, WA_Right);
            oled_matrix_drawStringInscribed_Smart (num_buf, n_area);
        }

        oled_matrix_set_textContext_prop (TX_Align2, WA_Left);
        oled_matrix_drawStringInscribed_Smart ((uint8_t *)_oled_items[item_index].Caption, item_rect);
        //  ---------------------------------------------------------------------
        item_index++;
        y_current += _oled_mainMenu.MenuItemHeight;
        // ---------------------------------------------------------------------
    }
    // -------------------------------------------------------------------------
    // Отрисовка боковой панели (имени) меню
    if (_oled_mainMenu.MenuName != (uint8_t *)'.') {

        if (_oled_mainMenu.MenuName[0] != '.') {

            oled_matrix_setBrush (1);
            oled_matrix_fill_rect(heap_area, 0);
            // ---------------------------------------------------------------------
            // Печать заголовка
            oled_matrix_set_textContext_prop (TX_Direction2, WS_Vertical);
            oled_matrix_set_textContext_prop (TX_Scale2, _oled_mainMenu.TextMenuNameScale);
            oled_matrix_set_textContext_prop (TX_Align2, WA_Center);

            oled_matrix_drawStringInscribed_Smart ((uint8_t *)_oled_mainMenu.MenuName, heap_area);
            oled_matrix_draw_rect(heap_area.Location, heap_area.ClientSize);
        }
    }
    // -------------------------------------------------------------------------
    oled_matrix_draw_rect(current_rect.Location, current_rect.ClientSize);
    oled_matrix_refresh();
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------