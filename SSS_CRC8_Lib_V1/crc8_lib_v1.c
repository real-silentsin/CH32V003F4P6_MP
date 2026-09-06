/********************************** (C) COPYRIGHT *******************************
 * File Name          : crc8_Lib_V1.c
 * Author             : vantr
 * Description        : Библиотека рассчета контрольной суммы CRC8
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
#include "crc8_Lib_V1.h"
#include <stdint.h>
// -----------------------------------------------------------------------------
uint8_t crc8_update (uint8_t crc, uint8_t data) {

    crc ^= data;
    
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) {
    
            crc = (crc << 1) ^ 0x07;  // 0x07 — полином
        } else {
    
            crc <<= 1;
        }
    }
    
    return crc;
}
// -----------------------------------------------------------------------------
// Рассчет контрольной суммы CRC8
uint8_t crc8_calculate (const uint8_t *data, uint16_t len) {

    uint8_t crc = 0x00;  // Начальное значение (Init)
    
    for (uint16_t i = 0; i < len; i++) {
        
        crc = crc8_update (crc, data[i]);
    }

    return crc;
}
// -----------------------------------------------------------------------------
// Рассчет контрольной суммы CRC8
uint8_t crc8_calculate_ex (const uint8_t *data, uint16_t begin_index, uint16_t len) {

    uint8_t crc = 0x00;  // Начальное значение (Init)

    for (uint16_t i = begin_index; i < len; i++) {

        crc = crc8_update (crc, data[i]);
    }

    return crc;
}
// -----------------------------------------------------------------------------
