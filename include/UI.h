#ifndef UI_H
#define UI_H
#include <Arduino.h>
#include <U8g2lib.h>

class UI {

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2;
uint8_t _select;
uint8_t _up;
uint8_t _down;
uint8_t _left;
uint8_t _right;


public:
    UI(uint8_t select, uint8_t up, uint8_t down, uint8_t left,uint8_t right);
    void begin();
    void loop();
}; 
#endif