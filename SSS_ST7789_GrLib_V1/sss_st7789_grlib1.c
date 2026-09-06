/********************************** (C) COPYRIGHT *******************************
 * File Name          : sss_st7789_grlib1.c
 * Author             : vantr
 * Description        : Драйвер для TFT дисплея ST7789 (графика)
 *********************************************************************************
 * Copyright (c) 2026 Vantr Universal Co., Ltd.
 *******************************************************************************/
// ----------------------------------------------------------------------------
#include "ch32v00x.h"
#include "SSS_ST7789_GrLib_V1/sss_st7789_grlib1.h"
#include "matrix_engine.h"
// ----------------------------------------------------------------------------
#include "FONTS.h"
// ----------------------------------------------------------------------------
#include "SSS_Common_Lib_V1/sss_classes.h"
#include "SSS_Common_Lib_V1/sss_math.h"
// ----------------------------------------------------------------------------
#ifdef USE_SPI_HARDWARE
#include "SSS_SPIHW_Lib1/sss_spim3_lib.h"
#else
#include "SSS_SPISW_Lib1/sss_spisw_lib1.h"
#endif
// ----------------------------------------------------------------------------
#ifdef USE_FLASH_W25Q
#include "SSS_W25Qxx_Lib_1/w25qxx.h"
#endif
// ----------------------------------------------------------------------------
// Это макросы управления (не изменять!)
// ----------------------------------------------------------------------------
// Пин CS
#define TFT_CS_LOW() (ST7789_SC_PORT.port->BCR = ST7789_SC_PORT.pin)     // CS = 0
#define TFT_CS_HIGH() (ST7789_SC_PORT.port->BSHR = ST7789_SC_PORT.pin)   // CS = 1
// ----------------------------------------------------------------------------
// Пин DC
#define TFT_DC_LOW() (ST7789_DC_PORT.port->BCR = ST7789_DC_PORT.pin)     // DC = 0
#define TFT_DC_HIGH() (ST7789_DC_PORT.port->BSHR = ST7789_DC_PORT.pin)   // DC = 1
// ----------------------------------------------------------------------------
#ifdef USE_TFT_ST7789_RST
// Пин RST
#define TFT_RST_LOW() ST7789_RST_PORT.port->BCR = ST7789_RST_PORT.pin    // RST = 0
#define TFT_RST_HIGH() ST7789_RST_PORT.port->BSHR = ST7789_RST_PORT.pin  // RST = 1
#endif
// ----------------------------------------------------------------------------
typedef enum {
    cas_empty,
    cas_hi_only
} color_accum_state;
// ----------------------------------------------------------------------------
typedef struct {
    uint8_t hi_byte;
    uint8_t lo_byte;
    color_accum_state state;
} color_accum_t;
// ----------------------------------------------------------------------------
// Цвета фона и пера
ParamsColor _screen_colors;
// ----------------------------------------------------------------------------
color_accum_t pixel_temp;
// ----------------------------------------------------------------------------
// Эти переменные должны быть static или глобальными
static Rect g_clip;
static uint16_t g_fw, g_cx, g_cy;
static int16_t g_sx, g_sy;
static uint8_t scall_factor = 1;
static uint16_t line_buf[64];  // Буфер текущей строки (под ширину 32)
// ----------------------------------------------------------------------------
// Область отрисовки
Rect _client_rect;
// ----------------------------------------------------------------------------
// Параметры асинхронной отрисовки стандартных иконок
Params16 icon_params;
// ----------------------------------------------------------------------------
// Инициализация механизма SPI
void ST7789_SPI_Init (void) {

    GPIO_InitTypeDef GPIO_InitStructure = {0};
#ifdef USE_SPI_HARDWARE
#ifdef TFT_INIT_SPI
    SPI_InitTypeDef SPI_InitStructure = {0};

    // Включаем тактирование GPIOA и SPI1
    RCC_APB2PeriphClockCmd (ST7789_GPIO_CTRL_RCC | SPIHW_GPIO_RCC | RCC_APB2Periph_SPI1, ENABLE);

    // PA5 (SCK) и PA7 (MOSI) - Альтернативная функция (Push-Pull)
    GPIO_InitStructure.GPIO_Pin = SPIHW_PIN_MOSI | SPIHW_PIN_SCK;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (SPIHW_GPIO_PORT, &GPIO_InitStructure);

    // PA10 (DC), PA9 (CS), PA11 (RES) - Обычный выход
    GPIO_InitStructure.GPIO_Pin = ST7789_PIN_CS | ST7789_PIN_DC | ST7789_PIN_RST;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init (ST7789_GPIO_CTRL, &GPIO_InitStructure);

    // MISO не инициализируем вовсе

    // Настройка SPI1
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;  // Только передача
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                         // Mode 3 (обычно для ST7789)
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                        // Mode 3
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;  // Скорость
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
    RCC_APB2PeriphClockCmd (ST7789_SC_PORT.rcc | ST7789_DC_PORT.rcc, ENABLE);
#ifdef USE_TFT_ST7789_RST
    RCC_APB2PeriphClockCmd (ST7789_RST_PORT.rcc, ENABLE);
#endif
    // DC, CS, RST - Обычные выходы
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = ST7789_SC_PORT.pin;
    GPIO_Init (ST7789_SC_PORT.port, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ST7789_DC_PORT.pin;
    GPIO_Init (ST7789_DC_PORT.port, &GPIO_InitStructure);

#ifdef USE_TFT_ST7789_RST
    GPIO_InitStructure.GPIO_Pin = ST7789_RST_PORT.pin;
    GPIO_Init (ST7789_RST_PORT.port, &GPIO_InitStructure);
#endif
    TFT_CS_HIGH();
    // ------------------------------------------------------------------------
#ifdef USE_SPI_SOFTWARE
    // ------------------------------------------------------------------------
    SPISW_Init();
    // ------------------------------------------------------------------------
#endif
    // ------------------------------------------------------------------------
}
// ----------------------------------------------------------------------------
#pragma region Зависимые от передатчика методы
// ----------------------------------------------------------------------------
// Быстрая отправка байта (точка контакта с интерфейсом)
void ST7789_SendByte (uint8_t data) {

#ifdef USE_SPI_HARDWARE
    SPIM3_SendByte(data);
#else
    (void)SPISW_Transfer(data); // Софтовый SPI всегда дуплекс
#endif
}
// ----------------------------------------------------------------------------
// Отправить данные цвета для текущего пикселя
void ST7789_SendColor (uint16_t color) {

    ST7789_SendByte (color >> 8);    // Старший байт
    ST7789_SendByte (color & 0xFF);  // Младший байт
}
// ----------------------------------------------------------------------------
// Отправка команды
void ST7789_WriteCommand (uint8_t cmd) {

    TFT_DC_LOW();  // DC = 0 (Команда)
    TFT_CS_LOW();  // CS = 0

    ST7789_SendByte (cmd);

#ifdef USE_SPI_HARDWARE
    //SPIM3_WaitFor();
#endif

    TFT_CS_HIGH();  // CS = 1
}
// ----------------------------------------------------------------------------
// Отправка данных
void ST7789_WriteData (uint8_t data) {

    TFT_DC_HIGH();  // DC = 1 (Данные)
    TFT_CS_LOW();   // CS = 0

    ST7789_SendByte (data);

#ifdef USE_SPI_HARDWARE
    //SPIM3_WaitFor();
#endif

    TFT_CS_HIGH();  // CS = 1
}
// ----------------------------------------------------------------------------
// 1. Команды управления сессией - открыть передачу данных
void ST7789_OpenSession() {

    TFT_DC_LOW();            // DC = 1 (Данные)
    TFT_CS_LOW();            // CS = 0
    ST7789_SendByte (0x2C);  // RAMWR
    
#ifdef USE_SPI_HARDWARE
    //SPIM3_WaitFor();
#endif

    TFT_DC_HIGH();
}
// ----------------------------------------------------------------------------
// 2. Команды управления сессией - закрыть передачу данных
void ST7789_CloseSession() {

#ifdef USE_SPI_HARDWARE
    //SPIM3_WaitFor();
#endif

    TFT_CS_HIGH();
}
// ----------------------------------------------------------------------------
// Установка области отрисовки (актуально только для текущей версии дисплея из-за его физики!)
// Внимание! Метод закрывает текущую сессию! Чтобы передавать данные в дисплей, откройте её снова!
void ST7789_SetAddressWindow (uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

    // 1. Колонки (X: 0...319)
    ST7789_WriteCommand (0x2A);
    TFT_DC_HIGH();
    TFT_CS_LOW();
    ST7789_SendByte (x0 >> 8);
    ST7789_SendByte (x0 & 0xFF);
    ST7789_SendByte (x1 >> 8);
    ST7789_SendByte (x1 & 0xFF);
    TFT_CS_HIGH();

    // 2. Строки (Y: 0...169 + OFFSET)
    ST7789_WriteCommand (0x2B);
    TFT_DC_HIGH();
    TFT_CS_LOW();
    ST7789_SendByte ((y0 + OFFSET_Y) >> 8);
    ST7789_SendByte ((y0 + OFFSET_Y) & 0xFF);
    ST7789_SendByte ((y1 + OFFSET_Y) >> 8);
    ST7789_SendByte ((y1 + OFFSET_Y) & 0xFF);
    TFT_CS_HIGH();
}
// ----------------------------------------------------------------------------
#pragma endregion
// ----------------------------------------------------------------------------
// Вывод одного пикселя
void ST7789_putPixel (uint16_t x, uint16_t y, uint32_t color) {

    if (x >= TFT_DISPLAY_WIDTH || y >= TFT_DISPLAY_HEIGHT) return;
    // 1. Ограничиваем область до одного пикселя
    // Передаем (x, y) как начало и как конец области
    ST7789_SetAddressWindow (x, y, x, y);

    // 2. Открываем сессию (опускаем CS)
    ST7789_OpenSession();

    // 3. Отправляем 16-битный цвет
    ST7789_SendColor (color);

    // 4. Закрываем сессию (поднимаем CS)
    // Это гарантирует, что контроллер примет координату и "забудет" о ней
    ST7789_CloseSession();
}
// ----------------------------------------------------------------------------
void ST7789_putPixelFC (uint16_t x, uint16_t y) {

    ST7789_putPixel(x, y, _screen_colors.fore.full);
}
// ----------------------------------------------------------------------------
// Установка цвета пикселя
void ST7789_setForeColor (uint16_t color) {

    _screen_colors.fore.full = color;
}
// ----------------------------------------------------------------------------
// Установка цвета фона
void ST7789_setBackColor (uint16_t color) {

    _screen_colors.back.full = color;
}
// ----------------------------------------------------------------------------
// Установка цвета пикселя
uint16_t ST7789_getForeColor () {

    return _screen_colors.fore.full;
}
// ----------------------------------------------------------------------------
// Установка цвета фона
uint16_t ST7789_getBackColor () {

    return _screen_colors.back.full;
}
// ----------------------------------------------------------------------------
/*  Как это использовать в интерфейсе:

    Для выделения строки в меню:
    ST7789_DrawGridTile(col, row, ST7789_Darken(CL_BLUE, 2), CL_BLACK); — получишь глубокий темно-синий.
    Для теней под текстом:
    Нарисуй текст цветом Darken(CL_GRAY, 2) со сдвигом в 1 пиксель, а сверху обычный текст. Выглядит объемно!
    Градиентная заливка:
    Запусти цикл по строкам и в каждой следующей строке чуть-чуть меняй alpha в MixColors между синим и черным. Получишь эффект неба.
*/
// ----------------------------------------------------------------------------
// Затемнение указанного цвета для меню
uint16_t ST7789_DarkenColor (uint16_t color, uint8_t factor) {

    if (factor == 0)
        return color;

    // Распаковываем RGB565
    uint16_t r = (color >> 11) & 0x1F;
    uint16_t g = (color >> 5) & 0x3F;
    uint16_t b = color & 0x1F;

    // Делим каждый канал
    r /= factor;
    g /= factor;
    b /= factor;

    // Запаковываем обратно
    return (r << 11) | (g << 5) | b;
}
// ----------------------------------------------------------------------------
// Смешение цветов
uint16_t ST7789_MixColors (uint16_t color1, uint16_t color2, uint8_t alpha) {

    // alpha от 0 (color2) до 32 (color1)
    if (alpha > 32)
        alpha = 32;
    uint8_t beta = 32 - alpha;

    uint16_t r = ((color1 >> 11) * alpha + (color2 >> 11) * beta) >> 5;
    uint16_t g = (((color1 >> 5) & 0x3F) * alpha + ((color2 >> 5) & 0x3F) * beta) >> 5;
    uint16_t b = ((color1 & 0x1F) * alpha + (color2 & 0x1F) * beta) >> 5;

    return (r << 11) | (g << 5) | b;
}
// ----------------------------------------------------------------------------
// Погасить экран (содержимое памяти сохраняется)
void ST7789_DisplayOff (void) {

    ST7789_WriteCommand (0x28);  // Display OFF
    // Если есть управление пином BLK, можно его тоже в LOW
}
// ----------------------------------------------------------------------------
// Включить экран
void ST7789_DisplayOn (void) {

    ST7789_WriteCommand (0x29);  // Display ON
    // Если есть управление пином BLK, вернуть в HIGH
}
// ----------------------------------------------------------------------------
// Глубокий сон (минимальное потребление, память может стереться)
void ST7789_Sleep (void) {

    ST7789_WriteCommand (0x10);  // Enter Sleep Mode
}
// ----------------------------------------------------------------------------
// Побудка дисплея из состояния сна
void ST7789_Wakeup (void) {

    ST7789_WriteCommand (0x11);  // Exit Sleep Mode
    Delay_Ms (120);              // Обязательная пауза
    //ST7789_WriteCommand (0x29);  // Display ON
    ST7789_DisplayOn();
}
// ----------------------------------------------------------------------------
// Инициализация альбомного режима дисплея ST7789 320x170
void ST7789_Init_Landscape (void) {
    //Delay_Ms (1000);
#ifdef USE_TFT_ST7789_RST
    // 1. Аппаратный сброс
    TFT_RST_LOW();
    Delay_Ms (100);
    TFT_RST_HIGH();
    Delay_Ms (120);
#else
    // 1. Программный сброс
    ST7789_WriteCommand (0x01);  // Команда SWRESET
    Delay_Ms (500);
#endif    

    // 3. Выход из спящего режима
    ST7789_WriteCommand (0x11);
    Delay_Ms (120);

    // 4. Настройка ориентации (LANDSCAPE)
    ST7789_WriteCommand (0x36);  // MADCTL
    ST7789_WriteData (0x60);     // Режим Landscape (MY=0, MX=1, MV=1, ML=0)

    // 5. Формат цвета (RGB565)
    ST7789_WriteCommand (0x3A);
    ST7789_WriteData (0x05);

    // 6. Инверсия (Обязательно для большинства 170x320 IPS)
    ST7789_WriteCommand (0x21);

    // 7. Оптимизация таймингов (для стабильности на высокой частоте SPI)
    ST7789_WriteCommand (0xB2);
    ST7789_WriteData (0x0C);
    ST7789_WriteData (0x0C);
    ST7789_WriteData (0x00);
    ST7789_WriteData (0x33);
    ST7789_WriteData (0x33);

    // 8. Включение дисплея
    ST7789_WriteCommand (0x29);
    Delay_Ms (20);

    // Установка цветов по умолчанию
    _screen_colors.fore.full = CL_WHITE;
    _screen_colors.back.full = CL_BLACK;

    TFT_matrix_frame_init();

    Rect cli_rect = TFT_matrix_get_clientrect();
    // Очистка экрана черным цветом сразу после включения
    TFT_matrix_fillRect (cli_rect, BackGround);
}
// -----------------------------------------------------------------------------
#ifdef TFT_USE_PRINTEX
// Установка ограничений для печати целых значений
void ST7789_SetPrIntRestrict (TFT_PRINTINT_RESTRICT_MODES mode) {

    _current_intrestr = mode;
}

// -----------------------------------------------------------------------------
// Печать целого десятичного (до 10 цифр [0..4 294 967 295]) числа с текущей позиции курсора
// format = 0, печать полного числа, включающего незначащие нули
// format > 0, незначащие ведущие нули будут опущены
// format = 0xFF, как при format = 0, но ведущие нули будут заменены на пробелы
// point - позиция децимальной точки, 0 - если точка не нужна
// На количество символов влияет значение в регистре _current_intrestr!
void ST7789_PrintLongInt (uint32_t value, uint8_t format, uint8_t point, uint16_t style) {

    uint8_t tmp_buffer[10] = {0};
    fill_buffer (value, tmp_buffer, 10);

    uint8_t tx_enable = 0;

    for (uint8_t rx_index = 0; rx_index < 10; rx_index++) {

        if (_current_intrestr != REST_0) {

            if (rx_index < (10 - (uint8_t)_current_intrestr)) {
                continue;
            }
        }

        if ((format == 1) && (tmp_buffer[9 - rx_index]) != 0) {
            tx_enable = 1;
        }

        // Вывод символа, если разрешено
        if (tx_enable == 1 || format == 0) {

            tx_enable = 1;
            ST7789_BASE_PutSymbolEx ((char)(tmp_buffer[9 - rx_index] + 0x30), style);
        } else {

            if (point > 0 && rx_index == (9 - point)) {
            } else {

                if (format == 0xFF) {

                    if (tmp_buffer[9 - rx_index] != 0) {

                        tx_enable = 1;
                        ST7789_BASE_PutSymbolEx ((char)(tmp_buffer[9 - rx_index] + 0x30), style);
                    } else {


                        ST7789_BASE_PutSymbolEx (' ', style);
                    }
                }
            }
        }

        // Если точка, то вывод будет происходить обязательно
        if (point > 0 && rx_index == (9 - point)) {

            if (tx_enable == 0) {

                ST7789_BASE_PutSymbolEx (tmp_buffer[9 - rx_index] + 0x30, style);
            }

            ST7789_BASE_PutSymbolEx ('.', style);
            tx_enable = 1;
        }
    }

    // Если здесь все еще не был разрешен вывод, печатаем ноль...
    if (tx_enable == 0) {

        ST7789_BASE_PutSymbolEx ('0', style);
    }
}

// -----------------------------------------------------------------------------
// Печать 8-битного числа в HEX формате
void ST7789_Print8_hex (uint8_t value, uint16_t style) {

    Params t_hrx_data = _int_to_hex (value);
    ST7789_BASE_PutSymbolEx (t_hrx_data.value1, style);
    ST7789_BASE_PutSymbolEx (t_hrx_data.value2, style);
}

// -----------------------------------------------------------------------------
// Печать 16-битного числа в HEX формате
void ST7789_Print16_hex (uint16_t value, uint16_t style) {

    uint8_t buffer16[4] = {0};

    fill_buffer_hex16 (value, (uint8_t *)buffer16);

    for (uint8_t index = 0; index < sizeof (buffer16); index++) {

        ST7789_BASE_PutSymbolEx (buffer16[index], style);
    }
}

// -----------------------------------------------------------------------------
// Печать 32-битного числа в HEX формате
void ST7789_Print32_hex (uint32_t value, uint16_t style) {

    uint8_t buffer32[8] = {0};

    fill_buffer_hex32 (value, (uint8_t *)buffer32);

    for (uint8_t index = 0; index < sizeof (buffer32); index++) {

        ST7789_BASE_PutSymbolEx (buffer32[index], style);
    }
}
#endif
// ----------------------------------------------------------------------------
#ifdef USE_FLASH_W25Q
// ----------------------------------------------------------------------------
// Переопределенный метод постраничного чтения файлов драйвером флеш-памяти
uint8_t W25_OnFileReadCH (const uint32_t address, const uint16_t segment_num, const uint8_t *data_chunk, const uint16_t len) {

    uint16_t i = 0;

    // 1. АКТИВИРУЕМ ДИСПЛЕЙ (CS = 0)
    TFT_CS_LOW();

    if (segment_num == 0) {

        switch (icon_params.param4) {
            
            case 2:
                scall_factor = 2;

                break;

            default:
                scall_factor = 1;

                break;
        }

        // 1. Читаем размеры из заголовка (Little Endian, как пишет C#)
        g_fw = (uint16_t)(data_chunk[0] | (data_chunk[1] << 8));
        uint16_t fh = (uint16_t)(data_chunk[2] | (data_chunk[3] << 8));

        if (g_fw > 32) {

            scall_factor = 1;
        } else {

            g_fw = g_fw * scall_factor;
            fh = fh * scall_factor;
        }

        g_sx = (int16_t)icon_params.param2;  // Начальный X иконки
        g_sy = (int16_t)icon_params.param3;  // Начальный Y иконки

        // 2. Считаем пересечение с экраном (магия обрезки)
        Rect icon_rect = TFT_matrix_create_rect2 (g_sx, g_sy, g_fw, fh);
        g_clip = TFT_matrix_intersect_rects (TFT_matrix_get_clientrect(), icon_rect);

        // Если иконка полностью вне экрана — выходим
        if (g_clip.ClientSize.Width <= 0 || g_clip.ClientSize.Height <= 0)
            return 1;

        // 3. Настраиваем окно дисплея ТОЛЬКО на видимую часть
        ST7789_SetAddressWindow (g_clip.Location.X, g_clip.Location.Y,
                                 g_clip.Location.X + g_clip.ClientSize.Width - 1,
                                 g_clip.Location.Y + g_clip.ClientSize.Height - 1);

        // 4. Открываем сессию (один раз шлем 0x2C)
        ST7789_OpenSession();

        g_cx = 0;
        g_cy = 0;
        pixel_temp.state = cas_empty;

        i = 4;  // Пропускаем заголовок, дальше — пиксели
    } else {

        // 3. Для всех следующих чанков — ТОЛЬКО физический выбор чипа
        // БЕЗ команды 0x2C, чтобы дисплей продолжал лить в то же место
        TFT_CS_LOW();
        TFT_DC_HIGH();

        // Режим данных
    }

    uint16_t color;

    // Просто льем байты в дисплей один за другим
    for (; i < len; i++) {
        
        switch (pixel_temp.state) {
            
            case cas_empty:

                pixel_temp.hi_byte = data_chunk[i];
                pixel_temp.state = cas_hi_only;
                
                break;

            case cas_hi_only:

                pixel_temp.lo_byte = data_chunk[i];

                color = (uint16_t)((pixel_temp.hi_byte << 8) | pixel_temp.lo_byte);
                pixel_temp.state = cas_empty;

                if (scall_factor == 1) {

                    ST7789_SendColor (color);
                } else {

                    // 1. Сохраняем пиксель в буфер для повтора строки
                    line_buf[g_cx] = color;

                    // 2. Растягиваем ПО ГОРИЗОНТАЛИ: шлем один пиксель дважды
                    ST7789_SendColor (color);
                    ST7789_SendColor (color);

                    // 3. Проверяем конец строки исходной иконки
                    if (++g_cx >= g_fw) {

                        // 4. Растягиваем ПО ВЕРТИКАЛИ: дублируем всю накопленную строку
                        // Дисплей послушно заполнит следующую строку своего окна (уже настроенного на 64)
                        for (uint16_t x = 0; x < g_fw; x++) {
                            ST7789_SendColor (line_buf[x]);
                            ST7789_SendColor (line_buf[x]);
                        }

                        g_cx = 0;
                        g_cy++;
                    }
                }

                break;
        }
    }

    // 5. ВАЖНО: Закрываем физическую сессию, чтобы флешка могла читать
    TFT_CS_HIGH();

    return 0;
}
// ----------------------------------------------------------------------------
// Рисование стандартной картинки из флеш-памяти (протокол)
void ST7789_drawIconFF (uint16_t file_id, uint16_t x, uint16_t y, uint8_t scall_factor) {

    icon_params.param1 = file_id;   // ID файла с изображением инонки
    icon_params.param2 = x;
    icon_params.param3 = y;
    icon_params.param4 = scall_factor;         // scall_factor

    pixel_temp.state = cas_empty;
    W25_ReadFileByIDEx (file_id, W25_OnFileReadCH);
    ST7789_CloseSession();
}
// ----------------------------------------------------------------------------
// Рисование стандартной картинки из флеш-памяти (протокол)
void ST7789_drawContIconFF (uint16_t file_id, uint16_t x, uint16_t y, uint8_t scall_factor) {

    icon_params.param1 = file_id;   // ID файла с изображением инонки
    icon_params.param2 = x;
    icon_params.param3 = y;
    icon_params.param4 = scall_factor;         // scall_factor

    pixel_temp.state = cas_empty;
    W25_ReadContFileByIDEx (file_id, W25_OnFileReadCH);
    ST7789_CloseSession();
}
// ----------------------------------------------------------------------------
// Получение размеров стандартной картинки по её file_id
Params16 ST7789_getImageProps(uint16_t file_id) {

    uint32_t first_segment_addr;

    Params16 result = { 0, 0, 0, 0 };

    // Пытаемся построить карту, запросив всего 1 сегмент (голову)
    if (W25_BuildFileMap_SinglePass (file_id, &first_segment_addr, 1) > 0) {
        // Карта построена (хотя бы частично) -> файл есть

        sectorPage_t *page_tmp = (W25_ReadPageEx (first_segment_addr, W25Q_PAGE_SIZE));

        if (page_tmp) {

            // 1. Читаем размеры из заголовка (Little Endian, как пишет C#)
            result.param1 = (uint16_t)(page_tmp->data[0] | (page_tmp->data[1] << 8));
            result.param2 = (uint16_t)(page_tmp->data[2] | (page_tmp->data[3] << 8));

            if (result.param1 == 0xFFFF || result.param2 == 0xFFFF) {

                result.param1 = 0;
                result.param2 = 0;
            }
        }
    }

    return result;
}
// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------
