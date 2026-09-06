/********************************** (C) COPYRIGHT *******************************
 * File Name          : MCP4725.h
 * Author             : vantr
 * Version            : V2.0.0
 * Date               : 2026/01/08
 * Description        : Драйвер DAC MCP4725 (аппаратный I2C)
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 * Библиотека не проверена в железе.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef __SSS_MCP4725HWI2C__
// -----------------------------------------------------------------------------
#define __SSS_MCP4725HWI2C__
// -----------------------------------------------------------------------------
#include "app_config.h"
// -----------------------------------------------------------------------------
// Допустимые режимы режима POWER-DOWN
#define PDM_NORMAL          0x00
#define PDM_1K2GND          0x01
#define PDM_100K2GND        0x02
#define PDM_500K2GND        0x03
// -----------------------------------------------------------------------------
// Адрес полной записи в DAC (второй байт для полного режима)
//                             C0..C2 ---  
//                             PD1, PD0    -- 
//                                    210XX10X
#define DAC_WRITE_DAC       0x40 // 0b01000000 // Запись только в DAC
#define DAC_WRITE_DAC_EPR   0x60 // 0b01100000 // Запись в DAC и EEPROM
// -----------------------------------------------------------------------------
void DAC_Init(uint8_t chip_number, uint8_t zero_start);

void DAC_SetPDM(uint8_t value);
uint8_t DAC_GetPDM();

uint32_t DAC_FastWrite(uint8_t chip_number, uint16_t out_voltage);
#ifdef DAC_FULLMODE_ENABLED
uint32_t DAC_Write(uint8_t chip_number, uint16_t out_voltage, uint8_t eeprom_enabled);
void DAC_SetMainMode(uint8_t chip_number);
#endif
uint8_t DAC_OptionsRead(uint8_t chip_number);

uint16_t DAC_GetValue(uint16_t out_mV);
uint16_t DAC_GetVoltage(uint16_t value_mv);

uint32_t DAC_getLastError();
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------