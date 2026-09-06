// -----------------------------------------------------------------------------
#ifndef __SSS_IRNEKEYES_H__
#define __SSS_IRNEKEYES_H__
// -----------------------------------------------------------------------------
#define KEY_0           0x52
#define KEY_1           0x16
#define KEY_2           0x19
#define KEY_3           0x0D
#define KEY_4           0x0C
#define KEY_5           0x18
#define KEY_6           0x5E
#define KEY_7           0x08
#define KEY_8           0x1C
#define KEY_9           0x5A
// -----------------------------------------------------------------------------
#define KEY_UP          0x46
#define KEY_DOWN        0x15
#define KEY_LEFT        0x44
#define KEY_RIGHT       0x43
// -----------------------------------------------------------------------------
#define KEY_OK          0x40
// -----------------------------------------------------------------------------
#define KEY_ASTERISK    0x42 // *
#define KEY_SHARP       0x4A // #
// -----------------------------------------------------------------------------
#define KEY_UNDEFINED   0
// -----------------------------------------------------------------------------
#define KEYES_IR_ADDR   0    // Адрес (первый байт) для пультов KEYES
// -----------------------------------------------------------------------------
// Перекодирование сырых кодов пульта KEYES (IR NEC 38 KHZ) в "нормальные"
uint8_t keyes_get_normal_code(uint8_t raw_code) {
    
    switch (raw_code) {
        
        case KEY_0: return 0;
        case KEY_1: return 1;
        case KEY_2: return 2;
        case KEY_3: return 3;
        case KEY_4: return 4;
        case KEY_5: return 5;
        case KEY_6: return 6;
        case KEY_7: return 7;
        case KEY_8: return 8;
        case KEY_9: return 9;
        
        case KEY_UP: return 10;
        case KEY_DOWN: return 11;
        case KEY_LEFT: return 12;
        case KEY_RIGHT: return 13;
        
        case KEY_OK: return 14;
        case KEY_SHARP: return 15;
        case KEY_ASTERISK: return 16;
    }
    
    return KEY_UNDEFINED;
}
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------