/********************************** (C) COPYRIGHT ******************************
 * File Name          : SSD1306.h
 * Author             : vantr
 * Description        : Библиотека драйвера дисплея SSD1306 SPI
 *******************************************************************************
 * Copyright (c) 2025 Vantr Universal Co., Ltd.
 *******************************************************************************/
// -----------------------------------------------------------------------------
#include "app_config.h"

#include "SSD1306_GR_SPI_V1/oled_spi_matrix_engine.h"
#include "SSS_W25Qxx_Lib_1/w25qxx.h"

#include "SSS_Common_Lib_V1/sss_classes.h"

#ifdef USE_SPI_HARDWARE
#include "SSS_SPIHW_Lib1/sss_spim3_lib.h"
#else
// Здесь будет ссылка на программный драйвер SPI
#endif

#include "OLED_SPI_SSD1306.h"
#include "FONTS.h"
// ----------------------------------------------------------------------------
// Это макросы управления (не изменять!)
// ----------------------------------------------------------------------------
// Пин CS
#define OLED_CS_LOW() (SSD1306_SC_PORT.port->BCR = SSD1306_SC_PORT.pin)     // CS = 0
#define OLED_CS_HIGH() (SSD1306_SC_PORT.port->BSHR = SSD1306_SC_PORT.pin)   // CS = 1
// ----------------------------------------------------------------------------
// Пин DC
#define OLED_DC_LOW() (SSD1306_DC_PORT.port->BCR = SSD1306_DC_PORT.pin)     // DC = 0
#define OLED_DC_HIGH() (SSD1306_DC_PORT.port->BSHR = SSD1306_DC_PORT.pin)   // DC = 1
// ----------------------------------------------------------------------------
// Пин RST
#ifdef USE_OLED_SSD1306_RST
#define OLED_RST_LOW() (SSD1306_RST_PORT.port->BCR = SSD1306_RST_PORT.pin)    // RST = 0
#define OLED_RST_HIGH() (SSD1306_RST_PORT.port->BSHR = SSD1306_RST_PORT.pin)  // RST = 1
#endif
// -----------------------------------------------------------------------------
//uint8_t ack_temp = 0;
Params OLED_SPI_temp_data;

// value1 - X столбец [0..DISPLAY_WIDTH_SYM - 1]
// value2 - Y строка [0..DISPLAY_HEIGHT_ROW - 1]
Params OLED_SPI_screen_options;
//Params caret_tmp;
// -----------------------------------------------------------------------------
// Режим переноса при переполнении
SSD1306_WRAP_MODE OLED_SPI_current_wrap = FULLWRAP;

// Настройки упаковки данных (настрой под свой конвертер картинок)
#define BIT_ORDER_MSB 1    // 1 - от 7-го бита к 0-му (обычно), 0 - от 0-го к 7-му
#define ROW_PADDED_BYTE 0  // 1 - каждая строка картинки начинается с нового байта (есть выравнивание)
// -----------------------------------------------------------------------------
// Переменные состояния для сборки картинки
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t start_x = 0;
static uint8_t start_y = 0;
static uint8_t img_width = 0;   // 1 байт (до 255 пикселей)
static uint8_t img_height = 0;  // 1 байт (до 255 пикселей)
static uint8_t scale_factor_x = 1;
// -----------------------------------------------------------------------------
#ifdef USE_SPI_HARDWARE
// ----------------------------------------------------------------------------
// Часть аппаратного SPI
// ----------------------------------------------------------------------------
void OLED_SPI_WriteData (uint8_t data) {

    OLED_DC_HIGH();  // Пин DC = 1 (режим данных)

    __NOP();
    //__NOP();
    //__NOP();
    
    OLED_CS_LOW();   // Выбираем дисплей

    SPIM3_SendByte (data);

    OLED_CS_HIGH();  // Поднимаем CS
}

// ----------------------------------------------------------------------------
void OLED_SPI_WriteCmd (uint8_t cmd) {

    OLED_DC_LOW();  // Режим команды

    __NOP();
    //__NOP();
    //__NOP();

    OLED_CS_LOW();  // Выбираем чип

    SPIM3_SendByte (cmd);

    OLED_CS_HIGH();  // Отпускаем чип
}
#endif
// -----------------------------------------------------------------------------
// Запись команды/данных в чип контроллера
uint8_t OLED_SPI_SSD1306_BASE_SEND (uint8_t mode, uint8_t data) {

    switch (mode) {

    case SSD1306_CMD_WRITECMD:

        OLED_SPI_WriteCmd (data);
        break;

    case SSD1306_CMD_WRITEDATA:

        OLED_SPI_WriteData (data);
        break;
    }

    return 1;
}
// -----------------------------------------------------------------------------
// Инициализация механизма OLED
void OLED1306_SPI_Init (void) {

    GPIO_InitTypeDef GPIO_InitStructure = {0};
#ifdef USE_SPI_HARDWARE
#ifdef OLED_INIT_SPI
    SPI_InitTypeDef SPI_InitStructure = {0};

    // Включаем тактирование GPIOA и SPI1
    RCC_APB2PeriphClockCmd (SSD1306_GPIO_CTRL_RCC | SPIHW_GPIO_RCC | RCC_APB2Periph_SPI1, ENABLE);

    // PA5 (SCK) и PA7 (MOSI) - Альтернативная функция (Push-Pull)
    GPIO_InitStructure.GPIO_Pin = SPIHW_PIN_MOSI | SPIHW_PIN_SCK;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (SPIHW_GPIO_PORT, &GPIO_InitStructure);

    // PA10 (DC), PA9 (CS), PA11 (RES) - Обычный выход
    GPIO_InitStructure.GPIO_Pin = SSD1306_PIN_CS | SSD1306_PIN_DC | SSD1306_PIN_RST;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init (SSD1306_GPIO_CTRL, &GPIO_InitStructure);

    // MISO не инициализируем вовсе

    // Настройка SPI1
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;  // Только передача
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                         // Mode 3 (обычно для OLED)
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                        // Mode 3
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  // Скорость
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init (SPIHW_DEVICE, &SPI_InitStructure);

    // 8. КЛЮЧЕВОЙ МОМЕНТ: устанавливаем SSI (Internal Slave Select)
    // Это "обманывает" контроллер, заставляя его думать, что на NSS высокий уровень,
    // при этом физический пин PC4 остается свободным для I2C SDA.
    SPI_NSSInternalSoftwareConfig (SPIHW_DEVICE, SPI_NSSInternalSoft_Set);

    SPI_Cmd (SPIHW_DEVICE, ENABLE);
#endif
#endif
    // ------------------------------------------------------------------------
    // -- Общий для всех случаев код ------------------------------------------
    // ------------------------------------------------------------------------
    // Включаем тактирование GPIOA
    RCC_APB2PeriphClockCmd (SSD1306_SC_PORT.rcc | SSD1306_DC_PORT.rcc, ENABLE);
#ifdef USE_OLED_SSD1306_RST
    RCC_APB2PeriphClockCmd (SSD1306_RST_PORT.rcc, ENABLE);
#endif
    // DC, CS, RST - Обычные выходы
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = SSD1306_SC_PORT.pin;
    GPIO_Init (SSD1306_SC_PORT.port, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = SSD1306_DC_PORT.pin;
    GPIO_Init (SSD1306_DC_PORT.port, &GPIO_InitStructure);

#ifdef USE_OLED_SSD1306_RST
    GPIO_InitStructure.GPIO_Pin = SSD1306_RST_PORT.pin;
    GPIO_Init (SSD1306_RST_PORT.port, &GPIO_InitStructure);
#endif
    // ------------------------------------------------------------------------
    OLED_CS_HIGH();    
    // ------------------------------------------------------------------------
#ifdef USE_SPI_SOFTWARE
    // ------------------------------------------------------------------------
    SPISW_Init();
    // ------------------------------------------------------------------------
#endif
    // ------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Установка контрастности [0..FF], где 0x7F - по умолчанию после включения
void OLED_SPI_SSD1306_BASE_CONTRAST(uint8_t value) {

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMD_SETCONTRAST);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, value);
}
// -----------------------------------------------------------------------------
// Включение дисплея
void OLED_SPI_SSD1306_BASE_ON(void) {

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x14);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAF);
}
// -----------------------------------------------------------------------------
// Полное отключение дисплея
void OLED_SPI_SSD1306_BASE_OFF(void) {

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x10);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAE);
}
// -----------------------------------------------------------------------------
// Очистка экрана
void OLED_SPI_SSD1306_BASE_ClearScreen(uint8_t on_by_complete) {
    // -------------------------------------------------------------------------
    OLED_SPI_SSD1306_BASE_OFF();
    // -------------------------------------------------------------------------
    // Сброс координат вывода
    OLED_SPI_screen_options.value1 = 0;
    OLED_SPI_screen_options.value2 = 0;
    // -------------------------------------------------------------------------
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDCOLADDR_SETCOL);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x7F);
    // -------------------------------------------------------------------------
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDPAGEADDR_SETPAGE);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x07);  // 128x64
    // -------------------------------------------------------------------------
    for (int index = 0; index < DISPLAY_HARDWARE_BUFFER_SIZE; index++) {

        OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, 0x00);
    }
    // -------------------------------------------------------------------------
    if (on_by_complete > 0) {
    
        OLED_SPI_SSD1306_BASE_ON();
    }
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
// Отправка буфера с графикой в дисплей
void OLED_SPI_SSD1306_BASE_SendBuffer(uint8_t *buffer, uint16_t count, uint8_t clear_screen) {
    // -------------------------------------------------------------------------
    if (clear_screen > 0) {
        
        OLED_SPI_SSD1306_BASE_ClearScreen(0);
    }
    // -------------------------------------------------------------------------
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDCOLADDR_SETCOL);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x7F);
    // -------------------------------------------------------------------------
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, SSD1306_CMDPAGEADDR_SETPAGE);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x07);  // 128x64
    // -------------------------------------------------------------------------
    for (int index = 0; index < count; index++) {

        OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITEDATA, buffer[index]);
    }
    // -------------------------------------------------------------------------
    OLED_SPI_SSD1306_BASE_ON();
    // -------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
// Обработчик потока данных из флеш-памяти
uint8_t W25_OnFileReadCH (const uint32_t address, const uint16_t segment_num, const uint8_t *data_chunk, const uint16_t len) {

    uint16_t data_start_index = 0;

    // ЕСЛИ ЭТО САМЫЙ ПЕРВЫЙ ЧАНК ФАЙЛА
    if (segment_num == 0) {
        cursor_x = 0;
        cursor_y = 0;

        // Забираем первые 2 байта как размеры картинки
        img_width = data_chunk[0];
        img_height = data_chunk[1];

        // Полезные графические данные в первом чанке начинаются с индкса 2
        data_start_index = 2;
    } else {
        // Во всех остальных чанках заголовка нет, фигачим данные с самого начала
        data_start_index = 0;
    }

    // ОТРИСОВКА ПИКСЕЛЕЙ
    for (uint16_t i = data_start_index; i < len; i++) {
        uint8_t byte = data_chunk[i];

        for (uint8_t b = 0; b < 8; b++) {
#if BIT_ORDER_MSB
            uint8_t bit_pos = 7 - b;
#else
            uint8_t bit_pos = b;
#endif

            uint8_t pixel_value = (byte >> bit_pos) & 0x01;

            if (cursor_y < img_height) {
                uint8_t target_x = start_x + cursor_x * scale_factor_x;
                uint8_t target_y = start_y + cursor_y * scale_factor_x;

                for (uint8_t sx = 0; sx < scale_factor_x; sx++) {
                    for (uint8_t sy = 0; sy < scale_factor_x; sy++) {
                        uint8_t final_x = target_x + sx;
                        uint8_t final_y = target_y + sy;

                        OLED_SPI_matrix_setPixel (final_x, final_y, pixel_value);
                    }
                }
            }

            cursor_x++;
            if (cursor_x >= img_width) {
                cursor_x = 0;
                cursor_y++;

#if ROW_PADDED_BYTE
                break;
#endif
            }
        }
    }

    return 0;
}
// ----------------------------------------------------------------------------
// Рисование битовых картинок на OLED дисплее из флеш-памяти (протокол)
uint8_t OLED_SPI_SSD1306_BASE_DrawProtoIcon (uint16_t file_id, uint8_t x, uint8_t y, uint8_t scale_factor) {

    start_x = x;
    start_y = y;

    /* Params16 icon_params;
    icon_params.param1 = file_id;
    icon_params.param2 = x;
    icon_params.param3 = y;
    icon_params.param4 = scale_factor; */
    scale_factor_x = scale_factor;

    uint8_t result = 0;

    if (W25_ReadFileByIDEx (file_id, W25_OnFileReadCH) > 0) {
        result = 1;
    };

    OLED_SPI_matrix_refresh();

    return result;
}
// ----------------------------------------------------------------------------
/* // Переопределенный метод постраничного чтения файлов драйвером флеш-памяти
uint8_t W25_OnFileReadCH2 (const uint32_t address, const uint16_t segment_num, const uint8_t *data_chunk, const uint16_t len) {

    // Сброс локальных курсоров в начале нового файла
    if (segment_num == 0) {
        cursor_x = 0;
        cursor_y = 0;
    }

    // Обработка потока байт из чанка
    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = data_chunk[i];

        // Разбор байта побитово
        for (uint8_t b = 0; b < 8; b++) {

// Выбираем порядок извлечения бита (MSB First или LSB First)
#if BIT_ORDER_MSB
            uint8_t bit_pos = 7 - b;
#else
            uint8_t bit_pos = b;
#endif

            // Получаем значение пикселя (0 или 1)
            uint8_t pixel_value = (byte >> bit_pos) & 0x01;

            // Вычисляем финальные координаты в виртуальном буфере
            uint8_t target_x = start_x + cursor_x;
            uint8_t target_y = start_y + cursor_y;

            // Рисуем пиксель в буфер
            matrix_setPixel (target_x, target_y, pixel_value);

            // Сдвигаем курсор по ширине
            cursor_x++;

            // Если достигли правого края иконки
            if (cursor_x >= img_width) {
                cursor_x = 0;
                cursor_y++;

// Если в bitmap каждая строка выровнена по границе байта,
// остаток текущего байта «выбрасывается». Переходим к следующему байту чанка.
#if ROW_PADDED_BYTE
                break;
#endif
            }
        }
    }

    return 0;
}
// -----------------------------------------------------------------------------
// Рисование бинарной иконки из потока
void OLED_SPI_SSD1306_BASE_DrawProtoIcon (uint16_t file_id, uint8_t x, uint8_t y, uint8_t width, uint8_t height) {

    // Фиксируем геометрию и стартовую позицию для callback-функции
    start_x = x;
    start_y = y;
    img_width = width;

    Params16 icon_params;

    icon_params.param1 = file_id;  // ID файла с изображением инонки
    icon_params.param2 = x;
    icon_params.param3 = y;
    icon_params.param4 = 1;  // scall_factor

    W25_ReadFileByIDEx (file_id, CB_PROTO_CH2);

    matrix_refresh();
} */
// -----------------------------------------------------------------------------
// Инициализация контроллера и дисплея
// Classic
void OLED_SPI_SSD1306_BASE_Init (uint8_t init_contrast) {
    // -------------------------------------------------------------------------
    OLED1306_SPI_Init();
    // -------------------------------------------------------------------------
    // Init LCD
    // -------------------------------------------------------------------------
#ifdef USE_OLED_SSD1306_RST
    OLED_RST_LOW();
    Delay_Ms (100);
    OLED_RST_HIGH();
#endif
    Delay_Ms (500);
    // -------------------------------------------------------------------------
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAE);  // Дисплей выключен

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x20);  // Режим автоматической адресации
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);  // 0x00 - по горизонтали с переходом на новую страницу (строку)
                                                              // 0x01 - по вертикали с переходом на новую строку
                                                              // 0x02 - только по выбранной странице без перехода

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xB0);  // Старновая страница GDDRAM [0..7]

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xC8);  // Режим сканирования озу дисплея для изменения системы координат
                                                              // С0 - снизу/верх (начало нижний левый угол)
                                                              // С8 - сверху/вниз (начало верний левый угол)

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);  // Установка начального адреса колонки
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x10);  // Установка конечного адреса колонки

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x40);  // Установка начального адреса строки [0..63]

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x81);  // Установка яркости (контрастности) дисплея
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, init_contrast);

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA1);  // set segment re-map 0 to 127

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA6);  //--set normal display
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA8);  //--set multiplex ratio(1 to 64)
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x1F);  //
#endif
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x3F);
#endif

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xA4);  // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD3);  //-set display offset
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x00);  //-not offset

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD5);  //--set display clock divide ratio/oscillator frequency
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xF0);  //--set divide ratio

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xD9);  //--set pre-charge period
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x22);  //

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xDA);  //--set com pins hardware configuration
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X32
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x02);
#endif
#ifdef SSD1306_DISPLAY_DIMENSIONS_128X64
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x12);
#endif

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xDB);  //--set vcomh
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x20);  // 0x20,0.77xVcc

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x8D);  //--set DC-DC enable
    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0x14);  //

    OLED_SPI_SSD1306_BASE_SEND (SSD1306_CMD_WRITECMD, 0xAF);  //--turn on SSD1306 panel

    OLED_SPI_SSD1306_BASE_ClearScreen (1);
    // -------------------------------------------------------------------------
}
// -----------------------------------------------------------------------------
