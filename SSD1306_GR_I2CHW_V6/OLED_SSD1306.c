/********************************** (C) COPYRIGHT ******************************
 * File Name          : SSD1306.h
 * Author             : vantr
 * Description        : Библиотека текстового вывода на дисплеи SSD1306 (ПОЛНАЯ)
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "debug.h"
#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_I2CHW_MLib_V4.4/i2c.h"
#include "OLED_SSD1306.h"
#ifdef SSD1306_USE_EEPROM_FONTS
#include "SSS_EPR24CXX_Lib_V1/EPR24Cxx.h"
#else
#include "FONTS.h"
#endif
// -----------------------------------------------------------------------------
//uint8_t ack_temp = 0;
Params OLED_temp_data;

// value1 - X столбец [0..DISPLAY_WIDTH_SYM - 1]
// value2 - Y строка [0..DISPLAY_HEIGHT_ROW - 1]
Params OLED_screen_options;
//Params caret_tmp;
// -----------------------------------------------------------------------------
// Режим переноса при переполнении
OLED_SSD1306_WRAP_MODE OLED_current_wrap = FULLWRAP2;
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_EEPROM_FONTS
// Буфер для одиночного символа
volatile uint8_t  SYM_IMAGE[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
// -----------------------------------------------------------------------------
// Сброс буферов конвертора
// buffer - ссылка на очищаемый байтовый буфер
// buffer_size - размер очищаемого буфера
void reset_buffer2(uint8_t * buffer, uint8_t buffer_size) {

    for (int i = 0; i < buffer_size; i++) {

        buffer[i] = 0;
    }
}
#endif
// -----------------------------------------------------------------------------
// Запись команды/данных в чип контроллера
uint8_t OLED_SSD1306_BASE_SEND(uint8_t mode, uint8_t data) {

    uint8_t tmp_buf[2] = { mode, data };
    return i2c_write(SSD1306_CHIP_SAVEADDR, tmp_buf, sizeof(tmp_buf));
}
// -----------------------------------------------------------------------------
// Установка контрастности [0..FF], где 0x7F - по умолчанию после включения
void OLED_SSD1306_BASE_CONTRAST(uint8_t value) {

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMD_SETCONTRAST);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, value);
}
// -----------------------------------------------------------------------------
// Включение дисплея
void OLED_SSD1306_BASE_ON(void) {

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x14);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAF);
}
// -----------------------------------------------------------------------------
// Полное отключение дисплея
void OLED_SSD1306_BASE_OFF(void) {

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x10);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAE);
}
// -----------------------------------------------------------------------------
// Очистка экрана
void OLED_SSD1306_BASE_ClearScreen(uint8_t on_by_complete) {
    // -------------------------------------------------------------------------
    OLED_SSD1306_BASE_OFF();
    // -------------------------------------------------------------------------
    // Сброс координат вывода
    OLED_screen_options.value1 = 0;
    OLED_screen_options.value2 = 0;
    // -------------------------------------------------------------------------
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDCOLADDR_SETCOL);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x7F);
    // -------------------------------------------------------------------------
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDPAGEADDR_SETPAGE);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x07);  // 128x64
    // -------------------------------------------------------------------------
    for (int index = 0; index < DISPLAY_HARDWARE_BUFFER_SIZE; index++) {

        OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, 0x00);
    }
    // -------------------------------------------------------------------------
    if (on_by_complete > 0) {
    
        OLED_SSD1306_BASE_ON();
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Отправка буфера с графикой в дисплей
void OLED_SSD1306_BASE_SendBuffer(uint8_t *buffer, uint16_t count, uint8_t clear_screen) {
    // -------------------------------------------------------------------------
    if (clear_screen > 0) {
        
        OLED_SSD1306_BASE_ClearScreen(0);
    }
    // -------------------------------------------------------------------------
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDCOLADDR_SETCOL);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x7F);
    // -------------------------------------------------------------------------
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDPAGEADDR_SETPAGE);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x07);  // 128x64
    // -------------------------------------------------------------------------
    for (int index = 0; index < count; index++) {

        OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, buffer[index]);
    }
    // -------------------------------------------------------------------------
    OLED_SSD1306_BASE_ON();
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_EEPROM_FONTS
// Загрузка символа 8x8 из EEPROM (24Cxx) по индексу символа
// Выход: успешность выполненой операции (bool)
uint8_t OLED_SSD1306_LoadSymbol(uint16_t sym_index) {
    
    // Очистка символьного буфера
    reset_buffer2((uint8_t *)SYM_IMAGE, 8);
    //for (int s_index = 0; s_index < 8; s_index++) { SYM_IMAGE[s_index] = 0; }
    
    if (sym_index == 0xFFFF) { return TRUE; }
    
#ifdef USE_MY_STANDARD_FONTS
    // Windows-1251
    // sym_code < 0x80 = (sym_code - 0x20)
    // sym_code < 0xC0 = ничего (0)
    // sym_code > 0xBF = (sym_code - 0x60)
    if (sym_index < 0x80) { sym_index -= 0x20; } else {
        
        if (sym_index < 0xC0) { sym_index = 0; } else {
            
            if (sym_index > 0xBF) { sym_index -= 0x60; }
        }
    }
#else
    sym_index -= 0x20;
#endif
    
    if (sym_index >= SYMBOLS_COUNT_IN_EPROM) { return FALSE; }

    uint8_t result = EPR_PRead((sym_index * 8), 8, (uint8_t *)SYM_IMAGE);
    return (result == 8) ? TRUE : FALSE;
}
#endif
// -----------------------------------------------------------------------------
#ifdef SSD1306_USE_NEW_INIT
// Инициализация контроллера и дисплея
// New - Free 3128
void OLED_SSD1306_BASE_Init(uint8_t init_contrast) {
    // -------------------------------------------------------------------------
    uint8_t init_byte = 0;
    // -------------------------------------------------------------------------
    I2CInit();    
    __delay_ms(500);
    // -------------------------------------------------------------------------
    /* Init LCD */	
    // -------------------------------------------------------------------------
    // Первый, проверенный вариант
    for (int init_index = 0; init_index < 28; init_index++) {
     
        init_byte = display_init_data[init_index];
        if (init_index == 9) {
         
            init_byte = init_contrast;
        }
        
        if (init_index == 27) {

            OLED_SSD1306_BASE_ClearScreen(1);
        }
        
        SSD1306_BASE_SEND(SSD1306_CMD_WRITECMD, init_byte);
    }
    // -------------------------------------------------------------------------
}
#else
// -----------------------------------------------------------------------------
// Инициализация дисплея
void OLED_SSD1306_BASE_Init(uint8_t init_contrast) {
    // -------------------------------------------------------------------------
/* #ifdef I2C_SPEED_STANDARD_MODE
    IIC_Init(I2C_SPEED_STANDARD_MODE, I2C_SELF_ADDRESS);
#endif
#ifdef I2C_SPEED_FAST_MODE
    IIC_Init(I2C_SELF_ADDRESS);
#endif */
    // -------------------------------------------------------------------------
    // Init LCD
    // -------------------------------------------------------------------------
    Delay_Ms(500);

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAE);  // Дисплей выключен

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x20);  // Режим автоматической адресации
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);  // 0x00 - по горизонтали с переходом на новую страницу (строку)
                                                          // 0x01 - по вертикали с переходом на новую строку
                                                          // 0x02 - только по выбранной странице без перехода

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xB0);  // Старновая страница GDDRAM [0..7]

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xC8);  // Режим сканирования озу дисплея для изменения системы координат
                                                          // С0 - снизу/верх (начало нижний левый угол)
                                                          // С8 - сверху/вниз (начало верний левый угол)

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);  // Установка начального адреса колонки
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x10);  // Установка конечного адреса колонки

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x40);  // Установка начального адреса строки [0..63]

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x81);  // Установка яркости (контрастности) дисплея
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, init_contrast);

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA1);                     // set segment re-map 0 to 127

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA6);                     //--set normal display
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA8);                     //--set multiplex ratio(1 to 64)
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMD_WRITECMD, 0x1F);  //
#endif
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x3F);
#endif

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA4);                   // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD3);                   //-set display offset
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);                   //-not offset

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD5);                   //--set display clock divide ratio/oscillator frequency
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xF0);                   //--set divide ratio

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD9);                   //--set pre-charge period
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x22);                   //

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xDA);                   //--set com pins hardware configuration
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x02);
#endif
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x12);
#endif

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xDB);  //--set vcomh
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x20);  // 0x20,0.77xVcc

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);  //--set DC-DC enable
    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x14);  //

    OLED_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAF);  //--turn on SSD1306 panel

    OLED_SSD1306_BASE_ClearScreen(1);
    // -------------------------------------------------------------------------
}
#endif
// -----------------------------------------------------------------------------
