/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_byteList.h
 * Author             : vantr
 * Version            : V1.0.1
 * Date               : 2026/02/17
 * Description        : Обертка к List<byte> из C#
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef __SSS_BLIST1__
// -----------------------------------------------------------------------------
#define __SSS_BLIST1__
// ----------------------------------------------------------------------------
#include "app_config.h"
// ----------------------------------------------------------------------------
#ifndef LIST_MAX_SIZE
#define LIST_MAX_SIZE 64
#endif
#define LIST_NOT_FOUND -1
// ----------------------------------------------------------------------------
typedef struct {
    uint8_t items[LIST_MAX_SIZE];
    uint8_t count;
} ByteList;
// ----------------------------------------------------------------------------
//#define list_Clear(l) ((l)->count = 0)
#define list_Count(l) ((l)->count)
#define list_Add(l, v) list_insert_at (l, (l)->count, v)
// -----------------------------------------------------------------------------
uint8_t list_pop_last(ByteList *l);
void list_clear (ByteList *l);
void list_fill (ByteList *l, uint8_t value);
void list_insert_at (ByteList *l, uint8_t index, uint8_t val);
void list_remove_at (ByteList *l, uint8_t index);
int16_t list_index_of (ByteList *l, uint8_t val);
void list_remove_value (ByteList *l, uint8_t val);
uint8_t list_copy_to (ByteList *l, uint8_t src_index, uint8_t *target_arr, uint8_t dst_index, uint8_t count);
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------