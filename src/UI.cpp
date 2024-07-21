#include "UI.h"

UI::UI(uint8_t select, uint8_t up, uint8_t down, uint8_t left,uint8_t right):_select(select), 
_up(up), _down(down), _left(left), _right(right),u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE){

}

void UI::begin(){
    u8g2.begin(_select, U8X8_PIN_NONE, _right, _left, _up, _down);
}

void UI::loop(){
    
}