/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_xproto_lib1.h
 * Author             : vantr
 * Version            : V3.0.1
 * Date               : 2026/03/18
 * Description        : Библиотека поддержки протокола X-Proto (версия для USART)
 * Target             : CH32X033
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef __SSS_XPROTO1__
// -----------------------------------------------------------------------------
#define __SSS_XPROTO1__
// -----------------------------------------------------------------------------
#include <stdint.h>
// -----------------------------------------------------------------------------
// Идентификатор протокола
// ----------------------------------------------------------------------------
#define XPROTO_PROTO_MAGICBYTE        0x58
#define XPROTO_PROTO_TERMINATOR       0x00
#define XPROTO_PROTO_NO_ERROR         0x00
#define XPROTO_PROTO_COMMON_ERROR     0xFF
#define XPROTO_PROTO_NOHEAD_ERROR     0xFE
#define XPROTO_PROTO_LEN_ERROR        0xFD
#define XPROTO_PROTO_CRC_ERROR        0xFC
// ----------------------------------------------------------------------------
// Структура полного пакета данных XProto
/*  ---------------------------
              bytes   send
    ---------------------------
    magic		1	    1
    crc			1	    2
    length		2	    4
    target		1	    2
    ------------------------
    ident		4	    8
    data	  260       520
    terminator  1       1
    ------------------------
              ---       538
    ------------------------
*/
// Большой буфер данных
#define XPROTO_XBUFFER_SIZE 550         // Смотри в комментарии
#define XPROTO_MAX_DATA_SIZE 260        // Максимальный размер пересылаемых данных в байтах (для bin)
#define XPROTO_TRANSPORT_MAX_LENGTH 64  // Максимальный размер отправляемых транспортом
// ----------------------------------------------------------------------------
//#define UART_ENDOFLINE XPROTO_PROTO_TERMINATOR  // Символ окончания строки
// ----------------------------------------------------------------------------
#pragma pack(push, 1)
// ----------------------------------------------------------------------------
typedef union {
    // Весь заголовок как массив
    uint8_t raw[5];

    // Структурный доступ
    struct {
        uint8_t magic;
        uint8_t crc8;

        // Union внутри структуры для поля length
        union {
            uint16_t val;
            uint8_t bytes[2];
        } length;

        uint8_t target;
    };
} ProtoHeader;
// ----------------------------------------------------------------------------
#pragma pack(pop)
// ----------------------------------------------------------------------------
// Структура для байта Target
typedef struct {
    uint8_t Device;
    uint8_t PackIdent;
} ProtoTarget;
// ----------------------------------------------------------------------------
// Структура для формирования бинарного пакета данных с адресом
#pragma pack(push, 1)  // ЗАСТАВЛЯЕМ компилятор не добавлять пустые байты
typedef union {
    struct {
        uint32_t header_u32;  // 4 байта
        uint8_t buffer[256];  // 256 байт (260 всего)
    };

    uint8_t full_buffer[XPROTO_MAX_DATA_SIZE];  // Весь пакет целиком
} packet_t;
#pragma pack(pop)
// ----------------------------------------------------------------------------
/* // Структура пакета команды
#pragma pack(push, 1)
typedef union {
    struct {
        union {
            uint32_t id;
            uint8_t id_raw[4];  // Побайтовый доступ к ID
        };
        uint16_t cmd;
        union {
            uint32_t param;
            uint8_t param_raw[4];  // Побайтовый доступ к параметру
        };
    };
    uint8_t raw[10];  // Весь пакет целиком (4 + 2 + 4 = 10 байт)
} cmd_pack_t;
#pragma pack(pop) */
// -----------------------------------------------------------------------------
// Структура пакета команды (расширенная)
#pragma pack(push, 1)
typedef union {
    struct {
        union {
            uint32_t id;
            uint8_t id_raw[4];  // Побайтовый доступ к ID
        };

        uint16_t cmd;

        union {
            uint32_t param;
            uint8_t param_raw[4];  // Побайтовый доступ к параметру
        };

        union {
            uint32_t ext_param;  // Новое 32-битное поле
            uint8_t ext_raw[4];  // Побайтовый доступ к новому полю
        };
    };

    uint8_t raw[14];  // Весь пакет целиком (4 + 2 + 4 + 4 = 14 байт)
} cmd_packex_t;
#pragma pack(pop)
// -----------------------------------------------------------------------------
// Структура пакета статуса (расширенная)
#pragma pack(push, 1)
typedef union {
    struct {
        uint16_t status;

        union {
            uint32_t param1;
            uint8_t param1_raw[4];
        };

        union {
            uint32_t param2;
            uint8_t param2_raw[4];
        };

        union {
            uint32_t param3;
            uint8_t param3_raw[4];
        };

        union {
            uint32_t param4;
            uint8_t param4_raw[4];
        };
    };

    uint8_t raw[18];  // Весь пакет целиком
} status_packex_t;
#pragma pack(pop)
// -----------------------------------------------------------------------------
uint8_t xproto_processingCommand();

uint8_t xproto_getProtoLastState (void);
uint16_t xproto_RealAvailable(uint8_t *buffer, uint16_t data_size);
uint16_t xproto_GetAvailable();

void xproto_setReadyToReceive();

uint8_t xproto_createTarget (uint8_t DeviceID, uint8_t PackIdent);
uint8_t xproto_createTarget2 (ProtoTarget Target);
ProtoTarget xproto_decodeTarget (uint8_t Value);

void xproto_resetBufferData();
uint16_t xproto_getBufferData (uint8_t *ResultData, uint16_t max_length);

//void xproto_addBufferChar (uint8_t *buffer, volatile uint16_t *buffer_pos, uint16_t data_lenght, uint8_t value);
uint16_t xproto_addBufferChars (uint8_t *buffer, uint16_t data_lenght);

uint8_t xproto_Compare_Str (const uint8_t *fstr, uint16_t length);
uint8_t xproto_CopyStr(uint8_t *buffer, uint16_t index, uint16_t count);

uint8_t xproto_rxDataIsProto(void);
void xproto_createProtoPacket(uint8_t *Data, ProtoHeader Header);
ProtoHeader xproto_decodeProtoPacket(uint8_t *ResultData);
ProtoHeader xproto_createProtoHeader (const uint8_t *ResultData, uint16_t Length, uint8_t Target);

void xproto_createStatusPacket (ProtoHeader header, uint32_t pack_ident, uint8_t status_code);
void xproto_createBINDataPacket (uint8_t *data, uint16_t data_len, uint32_t bin_pack_ident, ProtoTarget target);
void xproto_createBINDataPacket2 (uint16_t *data, uint16_t data_len, uint32_t bin_pack_ident, ProtoTarget target);
packet_t *xproto_decodeBINPacked (uint8_t *pack_data, uint16_t length);

void xproto_createCMDPacket (uint16_t cmd, uint32_t pack_ident, uint32_t param, uint32_t ext_param, ProtoTarget target);
uint8_t xproto_decodeCMDPacket (const uint8_t *data, uint16_t pack_size, cmd_packex_t *result);

void xproto_sendChar(uint8_t value, uint8_t crlf);
void xproto_sendStr(uint8_t *str, uint8_t length, uint8_t crlf);
void xproto_send8data_hex(uint8_t value, uint8_t crlf);
void xproto_send16addr_hex(uint16_t value, uint8_t crlf);
void xproto_send32addr_hex(uint32_t value, uint8_t crlf);
void xproto_sendLongInt(uint32_t value, uint8_t format, uint8_t crlf);
uint16_t xproto_sendTXBufferNow();
void xproto_printTXBufferToDebug();

uint16_t xproto_decode_4HEX_addr (uint8_t start_index);
uint32_t xproto_decode_8HEX_addr (uint8_t start_index);

void xproto_PrintData(const uint8_t* data, uint16_t len);
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------