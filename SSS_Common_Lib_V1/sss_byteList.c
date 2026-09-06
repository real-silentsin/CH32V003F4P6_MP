/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_byteList.c
 * Author             : vantr
 * Description        : Обертка к List<byte> из C#
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include <stdint.h>
#include "string.h"
#include "sss_byteList.h"
// ----------------------------------------------------------------------------
// Ограничить емкость массива до указанного значения
void list_limit_to (ByteList *l, uint8_t new_count) {

    if (new_count < l->count) { l->count = new_count; }
}
// ----------------------------------------------------------------------------
// Получение последнего значения из списка с его удалением
uint8_t list_pop_last (ByteList *l) {

    if (l->count > 0) {

        uint8_t result = l->items[l->count - 1];
        l->count--;

        return result;
    }

    return 0;
}
// ----------------------------------------------------------------------------
// Реальная очистка значений всего списка
void list_clear (ByteList *l) {

    memset (&l->items, 0, LIST_MAX_SIZE);
    l->count = 0;
}
// ----------------------------------------------------------------------------
// Заполнить весь список указанным значением
void list_fill (ByteList *l, uint8_t value) {

    memset (&l->items, value, LIST_MAX_SIZE);
}
// ----------------------------------------------------------------------------
// Аналог list.Insert(index, value)
void list_insert_at (ByteList *l, uint8_t index, uint8_t val) {

    // 1. Проверяем, есть ли место и не лезем ли мы за пределы текущего Count
    if (l->count >= LIST_MAX_SIZE || index > l->count) {
        return;
    }

    // 2. Если вставляем не в самый конец, раздвигаем массив
    if (index < l->count) {

        uint8_t bytes_to_move = l->count - index;
        memmove (&l->items[index + 1], &l->items[index], bytes_to_move);
    }

    // 3. Кладем значение и инкрементируем размер
    l->items[index] = val;
    l->count++;
}
// ----------------------------------------------------------------------------
// Аналог list.RemoveAt(index)
void list_remove_at (ByteList *l, uint8_t index) {

    if (index >= l->count)
        return;  // Индекс за пределами — выходим

    // Если удаляем не последний элемент, сдвигаем хвост влево
    if (index < l->count - 1) {

        // Вычисляем, сколько байт нужно передвинуть
        uint8_t bytes_to_move = l->count - index - 1;

        // memmove правильно обрабатывает перекрывающиеся области
        memmove (&l->items[index], &l->items[index + 1], bytes_to_move);
    }

    l->count--;  // Уменьшаем счетчик
}
// ----------------------------------------------------------------------------
// Возвращает индекс элемента или -1
int16_t list_index_of (ByteList *l, uint8_t val) {

    for (uint8_t i = 0; i < l->count; i++) {

        if (l->items[i] == val) {

            return (int16_t)i;
        }
    }
    return LIST_NOT_FOUND;
}
// ----------------------------------------------------------------------------
// Аналог вашего list.Remove(value) в C#
void list_remove_value (ByteList *l, uint8_t val) {

    int16_t index = list_index_of (l, val);

    if (index != LIST_NOT_FOUND) {

        list_remove_at (l, (uint8_t)index);
    }
}
// ----------------------------------------------------------------------------
/**
 * @brief Копирует элементы из списка в целевой массив
 *
 * @param l          Указатель на исходный ByteList
 * @param src_index  С какого индекса в списке начинаем брать данные (sourceIndex)
 * @param target_arr Массив, в который копируем (destinationArray)
 * @param dst_index  С какого индекса в целевом массиве начинаем вставку (destinationIndex)
 * @param count      Сколько байт скопировать
 * @return uint8_t   1 если успешно, 0 если вышли за границы
 */
uint8_t list_copy_to (ByteList *l, uint8_t src_index, uint8_t *target_arr, uint8_t dst_index, uint8_t count) {
    // 1. Проверяем, не выходим ли за границы списка
    if (src_index + count > l->count) {
        return 0;  // Ошибка: в списке нет столько данных
    }

    // 2. Копируем данные
    // Используем memcpy, так как целевой массив и список — это разные области памяти
    memcpy (&target_arr[dst_index], &l->items[src_index], count);

    return 1;  // Успех
}
// ----------------------------------------------------------------------------
/*
// ---------------------------------------------------------------------
// Примеры использования:
ByteList myList = { .count = 0 };

list_Add(&myList, 0xAA);             // [AA], count=1
list_Add(&myList, 0xCC);             // [AA, CC], count=2
list_insert_at(&myList, 1, 0xBB);    // [AA, BB, CC], count=3
list_remove_at(&myList, 0);          // [BB, CC], count=2
// ---------------------------------------------------------------------
ByteList my_list = {{10, 20, 30, 40, 50}, 5}; // Список из 5 элементов

list_remove_at(&my_list, 1); // Удаляем число "20" (индекс 1)

// Теперь my_list.items содержит {10, 30, 40, 50}
// my_list.count равен 4
// ---------------------------------------------------------------------
// Функция, которая принимает "массив"
void send_data(uint8_t *data, uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        // что-то делаем, например:
        // UART_SendData(data[i]);
    }
}

// Вызов для твоего списка:
send_data(myList.items, myList.count);
// ---------------------------------------------------------------------
uint8_t main_buffer[64] = {0};
ByteList myList;

// ... допустим, в myList уже лежат [0xAA, 0xBB, 0xCC, 0xDD, 0xEE] (count=5)

// Копируем 3 байта, начиная с индекса 1 (т.е. BB, CC, DD)
// в main_buffer, начиная с позиции 10
list_copy_to(&myList, 1, main_buffer, 10, 3);

// Теперь в main_buffer[10]=BB, [11]=CC, [12]=DD
// ---------------------------------------------------------------------
*/
// ----------------------------------------------------------------------------