/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_stringList.c
 * Author             : vantr
 * Description        : Обертка к List<string> из C#
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
//  -------------------------------------------------------------------------
#include "sss_stringList.h"
#include <string.h>
//  -------------------------------------------------------------------------
// Статический пул памяти для строк в SRAM микроконтроллера
static PoolString string_pool[POOL_MAX_STRINGS];
//  -------------------------------------------------------------------------
// Внутренняя функция выделения чистой ячейки из пула памяти
static PoolString *allocate_pool_string (void) {
    for (uint8_t i = 0; i < POOL_MAX_STRINGS; i++) {
        if (!string_pool[i].is_used) {
            string_pool[i].is_used = true;
            string_pool[i].length = 0;
            memset (string_pool[i].data, 0, sizeof (string_pool[i].data));
            return &string_pool[i];
        }
    }
    return NULL;
}
//  -------------------------------------------------------------------------
// Инициализация списка строк
void string_list_init (DynamicStringList *l) {
    if (!l)
        return;
    l->count = 0;
    for (uint8_t i = 0; i < POOL_MAX_STRINGS; i++) {
        l->items[i] = NULL;
    }
}
//  -------------------------------------------------------------------------
// Очистка списка строк
void string_list_clear (DynamicStringList *l) {
    if (!l)
        return;
    for (uint8_t i = 0; i < l->count; i++) {
        if (l->items[i] != NULL) {
            l->items[i]->is_used = false;
            l->items[i] = NULL;
        }
    }
    l->count = 0;
}
//  -------------------------------------------------------------------------
// Добавление строки к списку
int8_t string_list_add (DynamicStringList *l, const char *initial_text) {
    if (!l || l->count >= POOL_MAX_STRINGS)
        return -1;

    PoolString *new_str = allocate_pool_string();
    if (!new_str)
        return -1;

    l->items[l->count] = new_str;
    uint8_t idx = l->count;
    l->count++;

    if (initial_text) {
        string_list_set_text (l, idx, initial_text);
    }
    return idx;
}
//  -------------------------------------------------------------------------
// Удаление строки из списка по индексу
void string_list_remove_at (DynamicStringList *l, uint8_t index) {
    if (!l || index >= l->count)
        return;

    l->items[index]->is_used = false;

    for (uint8_t i = index; i < l->count - 1; i++) {
        l->items[i] = l->items[i + 1];
    }
    l->count--;
    l->items[l->count] = NULL;
}
//  -------------------------------------------------------------------------
// Присвоение имеющейся строке нового значения
void string_list_set_text (DynamicStringList *l, uint8_t index, const char *text) {
    if (!l || index >= l->count || !text)
        return;

    PoolString *str = l->items[index];
    str->length = 0;

    while (*text && (str->length < STRING_MAX_LEN)) {
        str->data[str->length++] = *text++;
    }
    str->data[str->length] = '\0';
}
//  -------------------------------------------------------------------------
// Получение адреса строки по её индексу
char *string_list_get_addr (DynamicStringList *l, uint8_t index) {
    if (!l || index >= l->count)
        return NULL;
    return l->items[index]->data;
}
//  -------------------------------------------------------------------------
// Легковесная функция перевода числа в строку (без оверхеда на Flash)
void string_list_append_int (DynamicStringList *l, uint8_t index, int32_t value) {

    if (!l || index >= l->count)
        return;

    PoolString *str = l->items[index];
    if (str->length >= STRING_MAX_LEN)
        return;

    // Временный локальный буфер под число (максимум 11 знаков для int32_t с минусом)
    char buf[12];
    uint8_t i = 0;
    bool is_negative = false;

    // Обработка нуля
    if (value == 0) {
        buf[i++] = '0';
    } else {
        // Обработка отрицательных чисел
        if (value < 0) {

            is_negative = true;
            // Используем беззнаковое деление, чтобы избежать переполнения при -2147483648
            uint32_t u_val = (uint32_t)(-value);
            while (u_val > 0) {
                buf[i++] = (u_val % 10) + '0';
                u_val /= 10;
            }
        } else {
            uint32_t u_val = (uint32_t)value;
            while (u_val > 0) {
                buf[i++] = (u_val % 10) + '0';
                u_val /= 10;
            }
        }
        if (is_negative) {
            buf[i++] = '-';
        }
    }

    // Символы в buf сейчас лежат задом наперед. Переносим их в правильном порядке в список строки
    while (i > 0) {
        if (str->length >= STRING_MAX_LEN)
            break;
        str->data[str->length++] = buf[--i];
    }

    str->data[str->length] = '\0';
}
//  -------------------------------------------------------------------------
