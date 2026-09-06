/********************************** (C) COPYRIGHT ******************************
 * File Name          : matrix_menu.c
 * Author             : vantr
 * Description        : Библиотека графики для OLED дисплея на SSD1306
 * Module             : Графическая библиотека MATRIX™
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
#include "matrix_menu.h"
#include "string.h"
// -----------------------------------------------------------------------------
// Ссылка на главное меню
ST920_Menu ST920__mainMenu;
ST920_MenuItem ST920__items[MENU_MAX_ITEMS];
// -----------------------------------------------------------------------------
// Отступ снизу при отрисовке имени меню
uint8_t ST920_mm_header_margin = 3;
// -----------------------------------------------------------------------------
// Получение количества элементов в меню
uint8_t ST920_matrix_menu_get_count() {

    return ST920__mainMenu.Count;
}
// -----------------------------------------------------------------------------
// Ссылка на главное меню
ST920_Menu *ST920_matrix_menu_mainmenu() {

    return &ST920__mainMenu;
}
// -----------------------------------------------------------------------------
// Быстрое создание элемента меню из параметров
void ST920_matrix_menu_create_menuitem (uint8_t *caption, uint16_t ActionID) {

    if (ST920__mainMenu.Count < MENU_MAX_ITEMS) {
        
        ST920_MenuItem result;

        result.ActionID = ActionID;
        result.Caption = caption;
        result.Selected = 0;

        ST920__items[ST920__mainMenu.Count] = result;
        ST920__mainMenu.Count += 1;
    }
}
// -----------------------------------------------------------------------------
// Установка значений по умолчанию
void ST920_matrix_menu_default_config (Rect ST920_client_rect, uint8_t item_height) {

    ST920__mainMenu.MenuName = (uint8_t *)".";
    ST920__mainMenu.State = Newed;  // Только что созданное меню!
    ST920__mainMenu.Scale = 1;
    ST920__mainMenu.MenuHeapWidth = 15;
    ST920__mainMenu.TextMenuNameScale = 1;

    ST920__mainMenu.ViewItemNumber = 1;
    ST920__mainMenu.ItemNumberWidth = 15;

    ST920__mainMenu.MenuItemHeight = (item_height < 10) ? 10 : item_height;
    ST920__mainMenu.MenuRect = ST920_client_rect;
    ST920__mainMenu.TopItemIndex = 0;
}
// -----------------------------------------------------------------------------
// Получение индекса выделенного элемента меню
ST920_MenuItem* ST920_matrix_menu_get_selected() {
    // -------------------------------------------------------------------------
    for (int index = 0; index < ST920__mainMenu.Count; index++) {

        // Если выделенный существует
        if (ST920__items[index].Selected == 1) {

            return &ST920__items[index];
        }
    }

    return &ST920__items[ST920__mainMenu.DefaultActionIndex];
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка области отрисовки меню
void ST920_matrix_menu_set_rect(Rect ST920_client_rect) {

    ST920__mainMenu.MenuRect = ST920_client_rect;
}
// -----------------------------------------------------------------------------
// Установка высоты единичного элемента меню
void ST920_matrix_menu_set_menuitem_height(uint8_t item_height) {

    ST920__mainMenu.MenuItemHeight = item_height;
}
// -----------------------------------------------------------------------------
// Установка первого отображаемого элемента меню
void ST920_matrix_menu_set_top_itemindex(uint8_t item_index) {

    ST920__mainMenu.TopItemIndex = item_index;
}
// -----------------------------------------------------------------------------
// Очистка меню
void ST920_matrix_menu_clear() {

    ST920__mainMenu.Count = 0;
    memset (ST920__items, 0, sizeof (ST920__items));
}
// -----------------------------------------------------------------------------
// Получение ссылки на элемент меню
ST920_MenuItem* matrix_menu_get(uint8_t index) {
    
    if (index < ST920__mainMenu.Count) {
        
        return &ST920__items[index];
    }

    return &ST920__items[0];
}
// -----------------------------------------------------------------------------
// Сделать видимым указанный элемент меню
void ST920_matrix_menu_set_visible(uint8_t item_index) {
    // -------------------------------------------------------------------------
    if (item_index + 1 > ST920__mainMenu.Count) {
        return;
    }
    // -------------------------------------------------------------------------
    if (item_index < ST920__mainMenu.TopItemIndex) {
        
        ST920__mainMenu.TopItemIndex = item_index;
        return;
    }
    // -------------------------------------------------------------------------
    int item_top = ST920__mainMenu.TopItemIndex * ST920__mainMenu.MenuItemHeight;
    int item_y = (item_index * ST920__mainMenu.MenuItemHeight);
    int item_b = item_y + ST920__mainMenu.MenuItemHeight;
    
    if (item_b - item_top > ST920__mainMenu.MenuRect.ClientSize.Height) {
        
        ST920__mainMenu.TopItemIndex = (((item_b - ST920__mainMenu.MenuRect.ClientSize.Height) / ST920__mainMenu.MenuItemHeight)) + 1;
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка выделенного элемента меню
void ST920_matrix_menu_set_selected(uint8_t item_index) {
    // -------------------------------------------------------------------------
    if (item_index < ST920__mainMenu.Count) {
        // -------------------------------------------------------------------------
        for (int index = 0; index < ST920__mainMenu.Count; index++) {

            ST920__items[index].Selected = (item_index == index) ? 1 : 0;
        }
        // -------------------------------------------------------------------------
        ST920_matrix_menu_set_visible (item_index);
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка имени меню
void ST920_matrix_menu_set_name(uint8_t * caption) {
    
    ST920__mainMenu.MenuName = caption;
}
// -----------------------------------------------------------------------------
// Установка отступа снизу для имени меню
void ST920_matrix_menuname_set_margin(uint8_t value) {

    ST920_mm_header_margin = value;
}
// -----------------------------------------------------------------------------
// Установка курсора на текст
void ST920_matrix_menu_itemtext_coord(ST920_MenuItem item) {

    Point textc = ST920_matrix_shift_coords(item.Location, 2, 2);
    ST920_matrix_setCoord2 (textc);
}
// -----------------------------------------------------------------------------
// Рисование главного меню
void ST920_matrix_menu_draw() {
    // -------------------------------------------------------------------------
    ST920_matrix_frame_init();
    // -------------------------------------------------------------------------
    Rect current_rect = ST920__mainMenu.MenuRect;

    ST920_matrix_setBrush (1);
    // -------------------------------------------------------------------------
    if (ST920__mainMenu.Count == 0) {

        ST920_matrix_fill_rect (current_rect, 0);
        ST920_matrix_client_rect();

        return;
    }
    // -------------------------------------------------------------------------
    Rect menuitems_area;

    menuitems_area.Location = current_rect.Location;
    menuitems_area.ClientSize = current_rect.ClientSize;

    if (ST920__mainMenu.MenuName != (uint8_t *)'.') {

        menuitems_area.ClientSize.Width -= ST920__mainMenu.MenuHeapWidth;
    }
    // -------------------------------------------------------------------------
    Rect heap_area;

    ST920_matrix_set_clientrect (ST920__mainMenu.MenuRect);

    heap_area.Location = ST920_matrix_get_corner (menuitems_area, TopRight);
    heap_area.ClientSize = (Size){ST920__mainMenu.MenuHeapWidth, menuitems_area.ClientSize.Height};
    // -------------------------------------------------------------------------
    uint8_t y_current = current_rect.Location.Y;
    uint8_t item_index = ST920__mainMenu.TopItemIndex;

    Size item_size;
    item_size.Width = menuitems_area.ClientSize.Width - 1;
    item_size.Height = ST920__mainMenu.MenuItemHeight;
    // -------------------------------------------------------------------------
    while (y_current < current_rect.Location.Y + current_rect.ClientSize.Height) {

        if (item_index > ST920__mainMenu.Count - 1) {
            break;
        }

        ST920__items[item_index].Location = ST920_matrix_create_point (current_rect.Location.X, y_current);
        Rect item_rect = ST920_matrix_create_rect (ST920__items[item_index].Location, item_size);
        item_rect = ST920_matrix_reduce_rect (item_rect, 4);
        // ---------------------------------------------------------------------
        if (ST920__items[item_index].Selected) {

            ST920_matrix_filled_rect (ST920__items[item_index].Location, item_size);
            ST920_matrix_setBrush (0);
        } else {

            ST920_matrix_setBrush (1);
        }

        ST920_matrix_set_textContext_prop (TX_Direction5, WS_Horizonlal);
        ST920_matrix_set_textContext_prop (TX_Scale5, ST920__mainMenu.Scale);
        ST920_matrix_set_textContext_prop (TX_Align5, WA_Left);

        // 2. Если включен показ номеров — делим область на две части
        if (ST920__mainMenu.ViewItemNumber > 0) {
            // Выделяем фиксированную "ячейку" под номер слева
            Rect n_area = item_rect;
            n_area.ClientSize.Width = ST920__mainMenu.ItemNumberWidth - 5;

            // Сдвигаем основную область текста вправо (она станет чуть меньше)
            item_rect.Location.X += ST920__mainMenu.ItemNumberWidth;
            item_rect.ClientSize.Width -= ST920__mainMenu.ItemNumberWidth;

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
            ST920_matrix_set_textContext_prop (TX_Align5, WA_Right);
            ST920_matrix_drawStringInscribed_Smart (num_buf, n_area);
        }

        ST920_matrix_set_textContext_prop (TX_Align5, WA_Left);
        ST920_matrix_drawStringInscribed_Smart ((uint8_t *)ST920__items[item_index].Caption, item_rect);
        //  ---------------------------------------------------------------------
        item_index++;
        y_current += ST920__mainMenu.MenuItemHeight;
        // ---------------------------------------------------------------------
    }
    // -------------------------------------------------------------------------
    // Отрисовка боковой панели (имени) меню
    if (ST920__mainMenu.MenuName != (uint8_t *)'.') {

        if (ST920__mainMenu.MenuName[0] != '.') {

            ST920_matrix_setBrush (1);
            ST920_matrix_fill_rect (heap_area, 0);
            // ---------------------------------------------------------------------
            // Печать заголовка
            ST920_matrix_set_textContext_prop (TX_Direction5, WS_Vertical);
            ST920_matrix_set_textContext_prop (TX_Scale5, ST920__mainMenu.TextMenuNameScale);
            ST920_matrix_set_textContext_prop (TX_Align5, WA_Center);

            ST920_matrix_drawStringInscribed_Smart ((uint8_t *)ST920__mainMenu.MenuName, heap_area);
            ST920_matrix_draw_rect (heap_area.Location, heap_area.ClientSize);
        }
    }
    // -------------------------------------------------------------------------
    ST920_matrix_draw_rect (current_rect.Location, current_rect.ClientSize);
    ST920_matrix_refresh();
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------