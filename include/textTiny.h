#ifndef textTiny_h
#define textTiny_h

void drawSlide();
void printTinyText(const char *txt, uint32_t color = 1, int16_t posX = 0, bool instant=false, bool clear=false);

#define FONT_TINY 8
// + остальные в include/digitsOnly.h

#endif