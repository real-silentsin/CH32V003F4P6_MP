/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_spisw_lib1.h
 * Author             : vantr
 * Version            : V1.1.0
 * Date               : 2026/06/08
 * Description        : Драйвер софтверного SPI (CH32V003)
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// ----------------------------------------------------------------------------
#ifndef __SSSPISW_H__
#define __SSSPISW_H__
// ----------------------------------------------------------------------------
#include "ch32x035.h"
#include "app_config.h"
// ----------------------------------------------------------------------------
// Все настройки модуля в <app.config>
// ----------------------------------------------------------------------------
// МАКРОСЫ УПРАВЛЕНИЯ (НЕ ТРОГАТЬ!)
// ----------------------------------------------------------------------------
#define SPISW_SCK_LOW() (SPISW_SCK_PORT.port->BCR = SPISW_SCK_PORT.pin)
#define SPISW_SCK_HIGH() (SPISW_SCK_PORT.port->BSHR = SPISW_SCK_PORT.pin)

#define SPISW_MOSI_LOW() (SPISW_MOSI_PORT.port->BCR = SPISW_MOSI_PORT.pin)
#define SPISW_MOSI_HIGH() (SPISW_MOSI_PORT.port->BSHR = SPISW_MOSI_PORT.pin)

#define SPISW_MISO_READ() ((SPISW_MISO_PORT.port->INDR & SPISW_MISO_PORT.pin) ? 1 : 0)
// ----------------------------------------------------------------------------
void SPISW_Init (void);
#ifdef SPISW_USE_DELAY
void SPISW_SetSpeed (uint32_t target_hz);
#endif

uint8_t SPISW_Transfer (uint8_t data);
// ----------------------------------------------------------------------------
#endif
