/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_stringList.h
 * Author             : vantr
 * Version            : V1.0.0
 * Date               : 2026/07/13
 * Description        : Обертка к List<string> из C#
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе.
 * ВНИМАНИЕ! Если это не используется, исключайте библиотеку из сборки для экономии RAM!
 *******************************************************************************/
//  -------------------------------------------------------------------------
#ifndef DYNAMIC_STRING_LIST_H
#define DYNAMIC_STRING_LIST_H
//  -------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
//  -------------------------------------------------------------------------
// Настройки пула памяти и строк
#define STRING_MAX_LEN          30  // Максимальная длина одной строки (без '\0')
#define POOL_MAX_STRINGS        16  // Максимальное количество строк в списке/пуле
//  -------------------------------------------------------------------------
// Структура «сырой» строки в физическом пуле памяти.
// Выровнена по границе 4 байт для безопасного и быстрого доступа на MCU RISC-V.
typedef struct {
    uint8_t length;
    volatile bool is_used;
    char data[STRING_MAX_LEN + 1];
} __attribute__ ((aligned (4))) PoolString;
//  -------------------------------------------------------------------------
// Структура списка строк (содержит только данные, экономит SRAM)
typedef struct {
    PoolString *items[POOL_MAX_STRINGS];
    uint8_t count;
} DynamicStringList;
//  -------------------------------------------------------------------------
void string_list_init (DynamicStringList *l);
void string_list_clear (DynamicStringList *l);
int8_t string_list_add (DynamicStringList *l, const char *initial_text);
void string_list_remove_at (DynamicStringList *l, uint8_t index);
void string_list_set_text (DynamicStringList *l, uint8_t index, const char *text);
char *string_list_get_addr (DynamicStringList *l, uint8_t index);
void string_list_append_int (DynamicStringList *l, uint8_t index, int32_t value);
//  -------------------------------------------------------------------------
#endif  // DYNAMIC_STRING_LIST_H
//  -------------------------------------------------------------------------
/* Пример использования:
 * //  -------------------------------------------------------------------------
 * // Очищаем старые строки из пула перед новым заходом
 * string_list_clear (&my_log);
 * string_list_init (&my_log);
 * //  -------------------------------------------------------------------------
 * // Создаем динамический набор данных
 * //  -------------------------------------------------------------------------
 * int8_t idx0 = string_list_add (&my_log, "Емкость чипа: ");
 * string_list_append_int (&my_log, idx0, mem_tmp_res.chip_size);
 * // Получаем АДРЕС строки в памяти
 * matrix_menu_create_menuitem ((uint8_t *)string_list_get_addr (&my_log, idx0), 0);
 * //  -------------------------------------------------------------------------
*/
