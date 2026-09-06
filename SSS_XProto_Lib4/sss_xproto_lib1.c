/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_xproto_lib1.h
 * Author             : vantr
 * Description        : Библиотека поддержки протокола X-Proto (версия для USART)
 * Target             : CH32X033
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// ----------------------------------------------------------------------------
#include <stdint.h>
#include <string.h>

#include "SSS_UART_Lib_V1/uart_lib.h"
#include "SSS_XProto_Lib4/sss_xproto_lib1.h"
#include "sss_xproto_lib1.h"
#include "sss_xproto_targets.h"

#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_Common_Lib_V1/sss_math.h"
#include "SSS_CRC8_Lib_V1/crc8_Lib_V1.h"
// ----------------------------------------------------------------------------
// Большой буфер приема
static uint8_t DataBuffer[XPROTO_XBUFFER_SIZE] = {0};
// ----------------------------------------------------------------------------
// Временный пакет для обработки BIN
static packet_t t_buffer = {0};
// ----------------------------------------------------------------------------
// Указатель в буфере
static volatile uint16_t DataBufferPtr = 0;
// ----------------------------------------------------------------------------
// Ошибки декодирования протокола
static volatile uint8_t proto_last_err = 0;
// -----------------------------------------------------------------------------
// Объявляем "слабую" функцию-обработчик команд протокола
__attribute__ ((weak)) void execute_xproto_cmd (void) {
    // По умолчанию ничего не делаем
}
// -----------------------------------------------------------------------------
// Объявляем "слабую" функцию-обработчик команд вне протокола
__attribute__ ((weak)) void execute_cmd_noproto (void) {
    // По умолчанию ничего не делаем
}
// -----------------------------------------------------------------------------
// Вызывайте этот метод в цикле для обработки сообщений
uint8_t xproto_processingCommand() {

    // ----------------------------------------------------------------
    // Если данные были получены...
    if (UART_getLastState() == UART_COM_COMPLETED) {
        // ------------------------------------------------------------
        // Здесь обработка принятого сообщения
        // ------------------------------------------------------------
        uint16_t real_rx_data_length = xproto_GetAvailable();

        if (real_rx_data_length > 0) {

            //xproto_printTXBufferToDebug();

            if (xproto_rxDataIsProto() == 1) {

                execute_xproto_cmd();
            }
            else {

                // Очистка пакета
                // memset (&t_buffer, 0, sizeof (packet_t));

                // uint16_t real_copied = xproto_getBufferData (t_buffer.buffer, XPROTO_MAX_DATA_SIZE);
                if (xproto_GetAvailable() > 0) {

                    execute_cmd_noproto();
                }
            }
        }
        // ------------------------------------------------------------
        // Сброс буфера для нового приема
        xproto_resetBufferData();
        // Разрешение приема
        xproto_setReadyToReceive();
        // ------------------------------------------------------------
    }

    return 1;
}
// -----------------------------------------------------------------------------
// Получение последнего состояния декодера протокола
uint8_t xproto_getProtoLastState() {

    return proto_last_err;
}
// -----------------------------------------------------------------------------
// Установка флага готовности к приему данных
void xproto_setReadyToReceive() {

    UART_Reset();
}
// -----------------------------------------------------------------------------
// Создание байта Target из номера устройства и идентификатор протокола
uint8_t xproto_createTarget (uint8_t DeviceID, uint8_t PackIdent) {

    return (uint8_t)(((DeviceID & 0x0F) << 4) | (PackIdent & 0x0F));
}
// -----------------------------------------------------------------------------
// Альтернативный метод создания Target
uint8_t xproto_createTarget2 (ProtoTarget Target) {

    return xproto_createTarget (Target.Device, Target.PackIdent);
}
// -----------------------------------------------------------------------------
/// Декодирование байта Target в идентификаторы протокола и устройства
ProtoTarget xproto_decodeTarget (uint8_t Value) {
    ProtoTarget result;

    result.PackIdent = (uint8_t)(Value & 0x0F);
    result.Device = (uint8_t)((Value & 0xF0) >> 4);

    return result;
}
// -----------------------------------------------------------------------------
// Сброс данных приема после обработки
void xproto_resetBufferData() {

    // Очистка: адрес буфера, значение (0), размер
    // memset (OperRxBuffer, 0, XPROTO_RX_BUF_SIZE);

    for (int index = 0; index < XPROTO_XBUFFER_SIZE; index++) { DataBuffer[index] = 0; }
    DataBufferPtr = 0;
    
    UART_Reset();
}
// -----------------------------------------------------------------------------
// Получение реально доступных (не 0) данных с начала буфера приема
uint16_t xproto_RealAvailable (uint8_t *buffer, uint16_t data_size) {

    for (uint16_t index = 0; index < data_size; index++) {

        if (buffer[index] == 0) {

            return (index == 0) ? 0 : index;
        }
    }

    return data_size;
}
// -----------------------------------------------------------------------------
// Получение количества ПРИНЯТЫХ байтов, должен быть терминатор
uint16_t xproto_GetAvailable() {

    return xproto_RealAvailable (DataBuffer, XPROTO_XBUFFER_SIZE);
}

// -----------------------------------------------------------------------------
// Добавление символа в буфер с инкрементом указателя текущего адреса
void xproto_addBufferChar (uint8_t *buffer, volatile uint16_t *buffer_pos, uint16_t data_lenght, uint8_t value) {

    if (*buffer_pos < data_lenght - 1) {

        buffer[(*buffer_pos)++] = value;
        //*buffer_pos += 1;
    } else {

        // Если предел буфера достигнут, замыкаем его терминатором
        buffer[data_lenght - 1] = UART_ENDOFLINE;
    }
}

// -----------------------------------------------------------------------------
// Добавить в буфер указанный массив данных
uint16_t xproto_addBufferChars (uint8_t *buffer, uint16_t data_lenght) {

    uint16_t count_copied = 0;

    for (int index = 0; index < data_lenght; index++) {

        if (DataBufferPtr < XPROTO_XBUFFER_SIZE) {

            xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, buffer[index]);
            count_copied += 1;
        } else {

            break;
        }
    }

    return count_copied;
}

// -----------------------------------------------------------------------------
// Копирование данных из большого буфера
uint16_t xproto_getBufferData (uint8_t *ResultData, uint16_t max_length) {

    uint16_t real_length = xproto_RealAvailable (DataBuffer, XPROTO_XBUFFER_SIZE);
    if (real_length == 0) {
        return 0;
    }

    uint16_t result = 0;
    for (uint16_t cp_index = 0; cp_index < real_length; cp_index++) {

        if (result >= max_length) {
            break;
        }

        ResultData[cp_index] = DataBuffer[cp_index];

        result += 1;
    }

    return result;
}
// -----------------------------------------------------------------------------
// Отправка в буфер передачи одиночного символа
void xproto_sendChar (uint8_t value, uint8_t crlf) {

    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, value);

    if (crlf) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, UART_ENDOFLINE);
        xproto_sendTXBufferNow();
    }
}
// -----------------------------------------------------------------------------
// Отправка строки в буфер передачи
void xproto_sendStr (uint8_t *str, uint8_t length, uint8_t crlf) {

    for (uint16_t str_index = 0; str_index < length; str_index++) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, str[str_index]);
    }

    if (crlf) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, UART_ENDOFLINE);
        xproto_sendTXBufferNow();
    }
}
// -----------------------------------------------------------------------------
uint16_t xproto_sendTXBufferNow() {

    uint16_t data_len = xproto_GetAvailable();
    
    if (data_len == 0)
        return 0;

    // ВОТ ОНО: Мы берем длину данных + 1 байт из буфера (там гарантированно '0')
    uint16_t total_to_send = data_len + 1;

    // Защита, чтобы не выйти за границы массива DataBuffer
    if (total_to_send > XPROTO_XBUFFER_SIZE)
        total_to_send = XPROTO_XBUFFER_SIZE;

    UART_SendString(DataBuffer);
    // ------------------------------------------------------------------------
    // -- Передача чанками по 64 байта
    /*uint16_t sent_bytes = 0;
    const uint16_t chunk_size = 64;

    while (sent_bytes < total_to_send) {
        uint16_t bytes_to_send = total_to_send - sent_bytes;
        if (bytes_to_send > chunk_size)
            bytes_to_send = chunk_size;

        // Отправляем порцию. USBFS_Print внутри СТРОГО ждет
        // окончания предыдущей передачи через while(Busy).
        UART_TxStringEx2 (&DataBuffer[sent_bytes], bytes_to_send);

        //printf ("USB Chunk [%d bytes]: %.*s\r\n", bytes_to_send, bytes_to_send, (char *)&DataBuffer[sent_bytes]);
        Delay_Ms(100);

        sent_bytes += bytes_to_send;
    } */
    // ------------------------------------------------------------------------

    // ЖДЕМ, пока последний пакет (с нашим нулем) реально улетит в USB
    // Без этого xproto_resetBufferData может сработать слишком рано!
    // while (UART_getLastState() == UART_COM);

    xproto_resetBufferData();
    xproto_setReadyToReceive();

    return total_to_send;
}

// -----------------------------------------------------------------------------
// Отладочный вывод содержимого буфера в терминал отладки LinkE
void xproto_printTXBufferToDebug() {

    uint16_t data_len = xproto_GetAvailable();

    if (data_len == 0) return;
    //printf ("Данные: %.*s", data_len, DataBuffer);
    //printf("\r\n");
}
// -----------------------------------------------------------------------------
// Формирование "протокольного пакета" в TX буфере
// Функция автоматически изменяет указатель в буфере TX
void xproto_createProtoPacket (uint8_t *Data, ProtoHeader Header) {

    if (Header.length.val > XPROTO_MAX_DATA_SIZE) {
        proto_last_err = XPROTO_PROTO_LEN_ERROR;
        return;
    }

    proto_last_err = XPROTO_PROTO_NO_ERROR;

    // Обнуляем массив передачи
    xproto_resetBufferData();

    // 0 - заголовок
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, Header.magic);

    // 1 - CRC8 контрольная сумма данных
    Params t_data = _int_to_hex (Header.crc8);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, t_data.value1);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, t_data.value2);

    // 2 - длина данных 16бит
    uint8_t len_hex[4] = {0};
    fill_buffer_hex16(Header.length.val, len_hex);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, len_hex[0]);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, len_hex[1]);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, len_hex[2]);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, len_hex[3]);

    //printf ("length: %08x\r\n", Header.length.val);
    //printf ("len_hex: %c%c%c%c\r\n", len_hex[0], len_hex[1], len_hex[2], len_hex[3]);

    // 3 - Цель передачи
    t_data = _int_to_hex (Header.target);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, t_data.value1);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, t_data.value2);

    // 7..Length - данные в HEX виде (каждый байт - 2 цифры) до XPROTO_MAX_DATA_SIZE байт
    for (uint16_t rx_index = 0; rx_index < Header.length.val; rx_index++) {

        t_data = _int_to_hex (Data[rx_index]);
        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, t_data.value1);
        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, t_data.value2);
    }

    // Последний байт - терминатор
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, UART_ENDOFLINE);

    //printf("total:%u\r\n", (unsigned int)xproto_GetAvailable());
}
// ----------------------------------------------------------------------------
// Заполнение структура данными
packet_t* xproto_decodeBINPacked(uint8_t *pack_data, uint16_t length) {

    // 1. Проверка входных данных на NULL
    if (!pack_data) { return &t_buffer; }

    // 1. Очистка пакета
    memset (&t_buffer, 0, sizeof (packet_t));

    // 2. Защита от "дурака" (переполнения)
    // Копируем только то, что влезет в наш buffer (256 байт)
    uint16_t safe_length = (length > XPROTO_MAX_DATA_SIZE) ? XPROTO_MAX_DATA_SIZE : length;

    // 3. Копируем данные
    memcpy (t_buffer.full_buffer, pack_data, safe_length);

    // Возвращаем указатель на глобальный/статический объект
    return &t_buffer;
}
// ----------------------------------------------------------------------------
// Создание отклика/статуса последней операции в ответ
void xproto_createStatusPacket(ProtoHeader header, uint32_t pack_ident, uint8_t status_code) {

    // Очистка пакета
    memset (&t_buffer, 0, sizeof (packet_t));

    // 1. Прямое заполнение через union
    t_buffer.header_u32 = pack_ident;
    t_buffer.buffer[0] = status_code;

    // 2. Длина теперь вычисляется логически (4 байта ID + 1 байт статус)
    header.length.val = 5;

    ProtoTarget real_target = xproto_decodeTarget(header.target);
    header.target = xproto_createTarget(real_target.Device, XP_STATUS_PACKET);

    header.crc8 = crc8_calculate(t_buffer.full_buffer, header.length.val);

    // 3. Передаем указатель прямо на начало буфера в объединении
    // Мы берем t_buffer.full_buffer, так как это и есть наши данные
    xproto_createProtoPacket (t_buffer.full_buffer, header);

    if (proto_last_err == XPROTO_PROTO_NO_ERROR) {
        xproto_sendTXBufferNow();
    }
}
// ----------------------------------------------------------------------------
// Создание бинарного пакета данных
void xproto_createBINDataPacket (uint8_t *data, uint16_t data_len, uint32_t bin_pack_ident, ProtoTarget target) {

    // 1. Обнуляем наш контейнер
    memset (&t_buffer, 0, sizeof (packet_t));

    // 2. Копируем "мясо" (256 байт)
    // t_buffer.buffer — это массив, который начинается ПОСЛЕ id
    memcpy (t_buffer.buffer, data, data_len);

    // 3. Заполняем ID бинарного пакета
    t_buffer.header_u32 = bin_pack_ident;

    // Вычисляем размер полезной нагрузки XProto как:
    // Размер фиксированного заголовка (ID) + Реальное количество данных
    uint16_t total_payload_len = sizeof (t_buffer.header_u32) + data_len;

    uint8_t real_Target = xproto_createTarget2 (target);

    // Передаем ЧЕСТНУЮ вычисленную длину в заголовок XProto
    ProtoHeader header = xproto_createProtoHeader (t_buffer.full_buffer, total_payload_len, real_Target);

    // Собираем пакет XProto, скармливая ему весь наш union
    xproto_createProtoPacket (t_buffer.full_buffer, header);

    if (xproto_getProtoLastState() == XPROTO_PROTO_NO_ERROR) {
        xproto_sendTXBufferNow();
    }
}
// ----------------------------------------------------------------------------
// Создание бинарного пакета данных
void xproto_createBINDataPacket2 (uint16_t *data, uint16_t data_len, uint32_t bin_pack_ident, ProtoTarget target) {

    // 1. Обнуляем наш контейнер
    memset (&t_buffer, 0, sizeof (packet_t));

    // 2. Заполняем ID бинарного пакета
    t_buffer.header_u32 = bin_pack_ident;

    // 3. Вычисляем размер полезной нагрузки XProto как:
    // Размер фиксированного заголовка (ID) + Реальное количество данных
    uint16_t total_payload_len = sizeof (t_buffer.header_u32) + (data_len * 2);

    //printf ("total_payload_len: %u\r\n", (unsigned int)total_payload_len);

    // 4. Проверка размера массива
    if (total_payload_len > XPROTO_MAX_DATA_SIZE) {

        proto_last_err = XPROTO_PROTO_LEN_ERROR;

        return;
    }

    Int16x data_t = {0};
    uint16_t cpy_idx = 0;

    // 6. Копирование массива
    for (uint16_t adrarr_idx = 0; adrarr_idx < data_len; adrarr_idx++) {

        data_t.ivalue = data[adrarr_idx];
        t_buffer.buffer[cpy_idx] = data_t.cvalue[0];
        cpy_idx += 1;
        t_buffer.buffer[cpy_idx] = data_t.cvalue[1];
        cpy_idx += 1;
    }

    //xproto_PrintData (t_buffer.full_buffer, total_payload_len);

    uint8_t real_Target = xproto_createTarget2 (target);

    // Передаем ЧЕСТНУЮ вычисленную длину в заголовок XProto
    ProtoHeader header = xproto_createProtoHeader (t_buffer.full_buffer, total_payload_len, real_Target);

    // Собираем пакет XProto, скармливая ему весь наш union
    xproto_createProtoPacket (t_buffer.full_buffer, header);

    if (xproto_getProtoLastState() == XPROTO_PROTO_NO_ERROR) {
        xproto_sendTXBufferNow();
    }
}
// ----------------------------------------------------------------------------
// Создание и отправка полного командного пакета
void xproto_createCMDPacket(uint16_t cmd, uint32_t pack_ident, uint32_t param, uint32_t ext_param, ProtoTarget target) {

    cmd_packex_t result = {0};

    result.id = pack_ident;
    result.cmd = cmd;
    result.param = param;
    result.ext_param = ext_param;

    uint16_t pack_size = sizeof(result);

    uint8_t real_target = xproto_createTarget (target.Device, XP_CMD_PACKET);

    ProtoHeader header = xproto_createProtoHeader(result.raw, pack_size, real_target);
    xproto_createProtoPacket(result.raw, header);

    if (xproto_getProtoLastState() == XPROTO_PROTO_NO_ERROR) {

        xproto_sendTXBufferNow();
    }
}
// ----------------------------------------------------------------------------
// Декодирование пакета команд
uint8_t xproto_decodeCMDPacket(const uint8_t* data, uint16_t pack_size, cmd_packex_t* result) {

    // 1. Проверка на NULL, чтобы не поймать HardFault
    if (data == NULL || result == NULL) {
        return 0;
    }

    // 2. Проверка размера: пакет должен быть не меньше нашей новой структуры (14 байт)
    if (pack_size < sizeof (cmd_packex_t)) {
        // Ошибка: пакет битый или неполный
        return 0;
    }

    // 3. Просто копируем 14 байт в структуру
    // Это безопасно, так как размер cmd_packex_t фиксирован
    memcpy (result->raw, data, sizeof (cmd_packex_t));

    return 1;
}
// ----------------------------------------------------------------------------
// Проверка, является ли принятые данные пакетом протокола
uint8_t xproto_rxDataIsProto() {

    uint16_t real_avail = xproto_RealAvailable (DataBuffer, XPROTO_XBUFFER_SIZE);
    if (real_avail < 10) {
        return 0;
    }

    if (DataBuffer[0] != XPROTO_PROTO_MAGICBYTE) {
        return 0;
    }

    Int16x len;
    len.cvalue[1] = _hex_to_byte2 (DataBuffer[3], DataBuffer[4]);
    len.cvalue[0] = _hex_to_byte2 (DataBuffer[5], DataBuffer[6]);

    uint16_t pack_size = (len.ivalue * 2) + 9;

    return (pack_size < real_avail) ? 1 : 0;
}
// -----------------------------------------------------------------------------
// Декодирование принятого протокольного пакета (буфер RX)
// Если декодирование успешно, будет выдано количество байт данных, если нет - ноль
ProtoHeader xproto_decodeProtoPacket (uint8_t *ResultData) {

    ProtoHeader result = {0};
    proto_last_err = 1;

    result.magic = DataBuffer[0];
    result.target = XPROTO_PROTO_COMMON_ERROR;

    if (xproto_rxDataIsProto() == 0) {
        return result;
    }

    if (DataBuffer[0] != XPROTO_PROTO_MAGICBYTE) {
        return result;
    }

    result.crc8 = _hex_to_byte2 (DataBuffer[1], DataBuffer[2]);
    result.length.bytes[1] = _hex_to_byte2 (DataBuffer[3], DataBuffer[4]);
    result.length.bytes[0] = _hex_to_byte2 (DataBuffer[5], DataBuffer[6]);
    uint8_t real_Target = _hex_to_byte2 (DataBuffer[7], DataBuffer[8]);

    uint16_t cur_pos = 9;
    Params t_data;
    uint16_t total_decode = 0;

    uint8_t real_crc8 = 0;

    // Проверяем, достаточно ли символов в буфере для такой длины
    if (DataBufferPtr < (result.length.val * 2) + cur_pos) {
        proto_last_err = XPROTO_PROTO_LEN_ERROR;
        return result;
    }

    //printf ("Pos: %d, Char: '%c', Code: 0x%02X\r\n", cur_pos, DataBuffer[cur_pos], DataBuffer[cur_pos]);

    for (total_decode = 0; total_decode < result.length.val; total_decode++) {
        // Безопасное извлечение пары
        t_data.value1 = DataBuffer[cur_pos++];
        t_data.value2 = DataBuffer[cur_pos++];

        uint8_t t_value = _hex_to_byte (t_data);
        ResultData[total_decode] = t_value;

        // Считаем CRC8 только от реально декодированного байта
        real_crc8 = crc8_update (real_crc8, t_value);
    }

    if (total_decode != result.length.val) {

        proto_last_err = XPROTO_PROTO_LEN_ERROR;
        result.target = XPROTO_PROTO_COMMON_ERROR;
        return result;
    }

    //printf ("CRC Rx: 0x%02X | CRC Cal: 0x%02X\r\n", result.crc8, real_crc8);

    if (real_crc8 != result.crc8) {

        proto_last_err = XPROTO_PROTO_CRC_ERROR;
        result.target = XPROTO_PROTO_COMMON_ERROR;
        return result;
    }

    result.target = real_Target;
    proto_last_err = XPROTO_PROTO_NO_ERROR;

    return result;
}
// -----------------------------------------------------------------------------
// Создание заголовка для указанных данных
ProtoHeader xproto_createProtoHeader (const uint8_t *ResultData, uint16_t Length, uint8_t Target) {

    ProtoHeader result;

    result.magic = XPROTO_PROTO_MAGICBYTE;
    result.length.val = Length;
    result.target = Target;

    if (Length > XPROTO_MAX_DATA_SIZE) {

        proto_last_err = XPROTO_PROTO_LEN_ERROR;
        return result;
    }

    result.crc8 = crc8_calculate (ResultData, Length);

    proto_last_err = XPROTO_PROTO_NO_ERROR;

    return result;
}
// -----------------------------------------------------------------------------
// Сравнение строки с буфером RX, если строки равны, будет выдан 0
// Или поиск совпадающего начала (к примеру CMD=)
// Возврат: TRUE (1) - если условия совпадают, иначе FALSE (0)
uint8_t xproto_Compare_Str (const uint8_t *fstr, uint16_t length) {

    if (XPROTO_XBUFFER_SIZE >= length) {

        // Побайтовое сравнение двух наборов символов
        for (int index = 0; index < length; index++) {

            if (fstr[index] != DataBuffer[index]) {

                return 0;
            }
        }

        return 1;
    }

    return 0;
}
// -----------------------------------------------------------------------------
// Копирование содержимого большого буфера в указанный буфер
uint8_t xproto_CopyStr (uint8_t *buffer, uint16_t index, uint16_t count) {

    if (index < XPROTO_XBUFFER_SIZE) {

        uint8_t ext_buf_index = 0;

        for (int x_ind = index; x_ind < XPROTO_XBUFFER_SIZE; x_ind++) {

            buffer[ext_buf_index] = DataBuffer[x_ind];
            ext_buf_index++;

            if (ext_buf_index == count) {
                break;
            }
        }

        return 0;
    }

    return 1;
}
// -----------------------------------------------------------------------------
// Отправка 8-битного числа в HEX формате
// <crlf> - перевести строку после отправки
void xproto_send8data_hex (uint8_t value, uint8_t crlf) {

    Params t_hrx_data = _int_to_hex (value);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, t_hrx_data.value1);
    xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, t_hrx_data.value2);

    if (crlf) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, UART_ENDOFLINE);
        xproto_sendTXBufferNow();
    }
}
// -----------------------------------------------------------------------------
// Отправка 16-битного числа в HEX формате
// <crlf> - перевести строку после отправки
void xproto_send16addr_hex (uint16_t value, uint8_t crlf) {

    uint8_t buffer16[4] = {0};

    fill_buffer_hex16 (value, (uint8_t *)buffer16);

    for (uint8_t index = 0; index < 4; index++) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, buffer16[index]);
    }

    if (crlf) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, UART_ENDOFLINE);
        xproto_sendTXBufferNow();
    }
}
// -----------------------------------------------------------------------------
// Отправка 32-битного числа в HEX формате
// <crlf> - перевести строку после отправки
void xproto_send32addr_hex (uint32_t value, uint8_t crlf) {

    uint8_t tmp_x32_data[8] = {0};

    fill_buffer_hex32 (value, tmp_x32_data);

    for (uint8_t index = 0; index < 8; index++) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, tmp_x32_data[index]);
    }

    if (crlf) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, UART_ENDOFLINE);
        xproto_sendTXBufferNow();
    }
}
// -----------------------------------------------------------------------------
// Отправка целого десятичного (до 8 цифр [0..4 294 967 295]) числа
// format = 0, отправка полного числа, включающего незначащие нули
// format > 0, незначащие нули будут опущены
// <crlf> - перевести строку после отправки
void xproto_sendLongInt (uint32_t value, uint8_t format, uint8_t crlf) {

    uint8_t tmp_buffer[10] = {0};
    fill_buffer (value, tmp_buffer, 10);

    uint8_t tx_enable = 0;

    for (uint8_t rx_index = 0; rx_index < 10; rx_index++) {

        if ((format == 1) && (tmp_buffer[9 - rx_index]) != 0) {
            tx_enable = 1;
        }

        if (tx_enable == 1 || format == 0) {

            tx_enable = 1;
            xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, tmp_buffer[9 - rx_index] + 0x30);
        }
    }

    if (tx_enable == 0) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, (uint8_t)'0');
    }

    if (crlf) {

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, UART_ENDOFLINE);
        xproto_sendTXBufferNow();
    }
}
// -----------------------------------------------------------------------------
// Извлечение адреса из присланного по uart сообщения
// Вход - 4 байта в HEX виде (индекс первого байта из четырех в буфере uart)
uint16_t xproto_decode_4HEX_addr (uint8_t start_index) {

    uint8_t buffer16[4] = {0};

    reset_buffer ((uint8_t *)buffer16, 0, 4);

    if (xproto_CopyStr ((uint8_t *)buffer16, start_index, 4) == 0) {

        // buf_temp[0] = uart_getBufferChar(start_index);
        // buf_temp[1] = uart_getBufferChar(start_index + 1);
        // buf_temp[2] = uart_getBufferChar(start_index + 2);
        // buf_temp[3] = uart_getBufferChar(start_index + 3);

        return hex_to_int16 ((uint8_t *)buffer16);
    }

    return 0;
}
// -----------------------------------------------------------------------------
// Извлечение адреса из присланного по uart сообщения
// Вход - 4 байта в HEX виде (индекс первого байта из четырех в буфере uart)
uint32_t xproto_decode_8HEX_addr (uint8_t start_index) {

    uint8_t buffer32[8] = {0};

    if (xproto_CopyStr((uint8_t *)buffer32, start_index, 8) == 0) {

        // buf_temp[0] = uart_getBufferChar(start_index);
        // buf_temp[1] = uart_getBufferChar(start_index + 1);
        // buf_temp[2] = uart_getBufferChar(start_index + 2);
        // buf_temp[3] = uart_getBufferChar(start_index + 3);

        return hex_to_int32 ((uint8_t *)buffer32);
    }

    return 0;
}
// ----------------------------------------------------------------------------
// Печать в терминал отладки байтовых данных в HEX формате
void xproto_PrintData(const uint8_t *data, uint16_t len) {
#ifdef USE_DEBUG_PRINT
    // ------------------------------------------------------------------------
    printf("BIN->HEX [%u bytes]\r\n", len);
    // ------------------------------------------------------------------------
    for (uint16_t i = 0; i < len; i++) {
    
        // Печатаем каждый байт как две заглавные HEX-цифры без пробелов
        printf("%02X ", data[i]);

        // Опционально: перенос строки каждые 64 символа (32 байта),
        // чтобы в терминале не было бесконечной колбасы
        if ((i + 1) % 32 == 0 && i < len - 1) {
    
            printf("\r\n");
            Delay_Ms(100);
        }
    }

    printf("\r\n");
#endif
}
// ----------------------------------------------------------------------------
// Это обработчик события приема данных с компьютера.
// Компилятор увидит это и заменит ею "weak" версию в драйвере UART
uint8_t UART_OnReceive (uint8_t pData) {
    
    if (DataBufferPtr < XPROTO_XBUFFER_SIZE) {

        //printf ("%c", (char)pData);

        xproto_addBufferChar (DataBuffer, &DataBufferPtr, XPROTO_XBUFFER_SIZE, pData);

        if (pData == UART_ENDOFLINE) {

            //printf ("<EOL>");
            return UART_COM_COMPLETED;
        }

        return UART_COM_READY;
    } else {

        return UART_COM_BUFFERFULL;
    }
}
// ----------------------------------------------------------------------------
