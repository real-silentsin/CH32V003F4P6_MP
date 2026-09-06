/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_pa1_2_lib1.h
 * Author             : vantr
 * Version            : V1.0.0
 * Date               : 2026/01/24
 * Description        : Библиотека использования PA1, PA2 как GPIO (SOP16)
 *********************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 * Библиотека проверена в железе. Допускается к использованию.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#ifndef __SSS_PA1_2__
// -----------------------------------------------------------------------------
#define __SSS_PA1_2__
// -----------------------------------------------------------------------------
#include "ch32v00x.h"
// -----------------------------------------------------------------------------
typedef enum {
    PORT_IN,
	PORT_IN_UP,
    PORT_OUT,
	PORT_OUT_OD
} SSS_PortMode_Types;
// -----------------------------------------------------------------------------
// Переключение пинов PA1, PA2 для работы штатными линиями ввода/вывода
void SSS_PA1_2_GPIO_Init(SSS_PortMode_Types mode)
{
    // 1. Включаем тактирование AFIO и GPIOA
    // В новых библиотеках это обязательный порядок
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    // 2. ПРИНУДИТЕЛЬНОЕ ОТКЛЮЧЕНИЕ HSE (Если этого не сделать, ремап не сработает)
    RCC_HSEConfig(RCC_HSE_OFF);
    
/*  В новых версиях чипов CH32V003A4M6 (SOP16) сделан автоматический
 *  ремап на уровне кремния для пинов PA1 и PA2, которые являются
 *  входами для кварцев. Если библиотека у вас не работает, раскомментируйте
 *  строку пункта 3, включающей ремап. Для новых версий это сломает программу.
 */

    // 3. РЕМАП (освобождаем пины 12 и 13)
    // Внимательно прочитайте комментарий выше этого блока!
    //GPIO_PinRemapConfig(GPIO_Remap_PA1_2, ENABLE);

    // 4. Настройка GPIO
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    
    switch (mode) {
        case PORT_OUT:
            
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
            break;
			
		case PORT_OUT_OD:
            
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
            break;

        case PORT_IN:
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            break;
			
		case PORT_IN_UP:
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
            break;
    }
    
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------