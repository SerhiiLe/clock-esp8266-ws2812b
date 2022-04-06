#ifndef runningText_h
#define runningText_h

void drawString();
void initRString(const char *txt, uint32_t color = 1, int16_t posX = -1);
void initRString(const __FlashStringHelper *txt, uint32_t color = 1, int16_t posX = -1);
void initRString(String txt, uint32_t color = 1, int16_t posX = -1);

extern bool screenIsFree;

#endif