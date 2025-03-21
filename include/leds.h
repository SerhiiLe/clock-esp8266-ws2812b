#ifndef leds_h
#define leds_h

uint16_t getPixelNumber(int8_t x, int8_t y);
void set_brightness(uint8_t);
uint32_t maximizeBrightness(uint32_t color, uint8_t limit = 255);
uint32_t getPixColorXY(int8_t x, int8_t y);
uint32_t getPixColor(int thisSegm);
void drawPixelXY(int8_t x, int8_t y, CRGB color);
CRGB drawChar(const uint8_t* pointer, int16_t startX, int16_t startY, uint8_t LH, uint8_t LW, uint32_t color=1, uint16_t index=0);
void fillAll(CRGB color);
void clearALL();
void display_setup();
void display_tick(bool clear = true);

extern bool itsTinyText;

#endif
