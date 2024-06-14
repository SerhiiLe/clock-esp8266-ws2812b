#ifndef digitsOnly_h
#define digitsOnly_h

int16_t printMedium(const char* txt, uint8_t font, int16_t pos, uint8_t limit=5, uint8_t start_char=0);
const char* changeDots(char* txt);

#define FONT_NORMAL 0
#define FONT_HIGHT 1
#define FONT_BOLD 2
#define FONT_WIDE 3
#define FONT_NARROW 4
#define FONT_NARROW2 5
#define FONT_DIGIT 6
#define FONT_DIGIT2 7
#define FONT_TINY 8

#endif