/*
Отрисовка только части циферблата средним по размеру шрифтом 8x4, в комбинации с 5x3 шрифтом даёт крупные часи с секундами.
Только цифры и разделители двоеточие и пробел
*/

#include <Arduino.h>
#include "defines.h"
#include "leds.h"
#include "digitsOnly.h"
#include "ntp.h"

#define LET_HEIGHT 8      // высота буквы шрифта

bool fl_tiny_clock = false;

const byte fontSemicolon[][4] PROGMEM = {
	{0x00, 0x42, 0x24, 0x00}, // : 1 1st 0
	{0x00, 0x24, 0x42, 0x00}, // : 2 2st 1
	{0x00, 0x20, 0x04, 0x00}, // : 3 1st 2
	{0x00, 0x04, 0x20, 0x00}, // : 4 2st 3
	{0x00, 0x46, 0x46, 0x00}, // : 5 1st 4
	{0x00, 0x4A, 0x4A, 0x00}, // : 6 2st
	{0x00, 0x52, 0x52, 0x00}, // : 7 3 st
	{0x00, 0x62, 0x62, 0x00}, // : 8 4st 7
	{0x00, 0x66, 0x66, 0x00}, // : 9 ":"
};

const byte fontBold[][6] PROGMEM = {
	{0x7E, 0xFF, 0x81, 0x81, 0xFF, 0x7E}, // 0 48
	{0x00, 0x82, 0xFF, 0xFF, 0x80, 0x00}, // 1
	{0xC2, 0xE3, 0xB1, 0x99, 0x8F, 0x86}, // 2
	{0x41, 0xC1, 0x89, 0x9D, 0xFF, 0x63}, // 3
	{0x38, 0x3C, 0x26, 0x23, 0xFF, 0xFF}, // 4
	{0x4F, 0xCF, 0x89, 0x89, 0xF9, 0x71}, // 5
	{0x7E, 0xFF, 0x91, 0x91, 0xF1, 0x60}, // 6
	{0x01, 0x01, 0xF1, 0xF9, 0x0F, 0x07}, // 7
	{0x76, 0xFF, 0x89, 0x89, 0xFF, 0x76}, // 8
	{0x0E, 0x9F, 0x91, 0x91, 0xFF, 0x7E}, // 9 57
	{0x00, 0x66, 0x66, 0x00, 0x00, 0x00}, // : 58
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' 32 11
};
const byte fontMediumScript[][4] PROGMEM = {
	{0x3E, 0x41, 0x41, 0x3E}, // 0 48
	{0x00, 0x42, 0x7F, 0x40}, // 1
	{0x62, 0x51, 0x49, 0x46}, // 2
	{0x22, 0x41, 0x49, 0x36}, // 3
	{0x18, 0x14, 0x12, 0x7F}, // 4
	{0x27, 0x45, 0x45, 0x39}, // 5
	{0x3E, 0x49, 0x49, 0x30}, // 6
	{0x01, 0x79, 0x05, 0x03}, // 7
	{0x36, 0x49, 0x49, 0x36}, // 8
	{0x06, 0x49, 0x49, 0x3E}, // 9 57
	{0x00, 0x24, 0x00, 0x00},  // : 58
};
const byte fontMediumDigit[][4] PROGMEM = {
	{0x7F, 0x41, 0x41, 0x7F}, // 0 48
	{0x00, 0x00, 0x7F, 0x00}, // 1
	{0x79, 0x49, 0x49, 0x4F}, // 2
	{0x41, 0x49, 0x49, 0x7F}, // 3
	{0x0F, 0x08, 0x08, 0x7F}, // 4
	{0x4F, 0x49, 0x49, 0x79}, // 5
	{0x7F, 0x49, 0x49, 0x79}, // 6
	{0x01, 0x01, 0x01, 0x7F}, // 7
	{0x7F, 0x49, 0x49, 0x7F}, // 8
	{0x4F, 0x49, 0x49, 0x7F}, // 9 57
	{0x00, 0x24, 0x00, 0x00},  // : 58
};
const byte fontTinyDigit[][3] PROGMEM = {
	{0x1F, 0x11, 0x1F},	// 0 42 0x30
	{0x00, 0x1F, 0x00},	// 1 43
	{0x1D, 0x15, 0x17},	// 2 44
	{0x11, 0x15, 0x1F},	// 3 45
	{0x07, 0x04, 0x1F},	// 4 46
	{0x17, 0x15, 0x1D},	// 5 47
	{0x1F, 0x15, 0x1D},	// 6 48
	{0x01, 0x01, 0x1F},	// 7 49
	{0x1F, 0x15, 0x1F},	// 8 50
	{0x17, 0x15, 0x1F},	// 9 51 0x39
	{0x0A, 0x00, 0x00},	// : 52 0x3a
};

// отрисовка одной буквы нестандартным шрифтом
int16_t drawMedium(const char c, int16_t x, CRGB color, uint8_t font_style) {
	byte dots;
	uint8_t cn = 0, cw = 4;
	uint8_t fontWidth = 4;
	int8_t string_offset = 0;
	byte* font;
	if(c >= 1 && c <= 9) {
		cn = c - 1;
		font = (byte*)fontSemicolon;
	} else {
		// сопоставление ascii кода символа и номера в таблице символов
		if(c >= '0' && c <= ':') // 0-9: maps to 0-10
			cn = c - '0';
		else if(c == ' ') {
			cn = 1; cw = 1;
		}
		if(c == ':') cw = 3;
		// выбор шрифта
		switch (font_style) {
		case FONT_WIDE:
			font = (byte*)fontBold;
			fontWidth = 6;
			cw = 6;
			if(c == ' ') {
				cn = 11; cw = 4;
			}
			if(c == ':') cw = 4;
			break;
		case FONT_NARROW:
			font = (byte*)fontMediumScript;
			break;
		case FONT_DIGIT:
			font = (byte*)fontMediumDigit;
			break;
		case FONT_TINY:
			font = (byte*)fontTinyDigit;
			fontWidth = 3;
			if(c == ' ' || c == ':' ) {
				cw = 1;
			} else
				cw = 3;
			string_offset = -2;
			break;
		default:
			return 0;
			break;
		}
	}

	for(uint8_t col = 0; col < cw; col++) {
		if(col + x > WIDTH) return cw;
		dots = pgm_read_byte(font + cn * fontWidth + col);
		for(uint8_t row = 0; row < LET_HEIGHT; row++) {
			if(row + string_offset >= 0 && row + string_offset < HEIGHT)
				if( dots & (1 << (LET_HEIGHT - 1 - row)) ) // в этой матрице отрисовка снизу вверх, шрифты записаны сверху вниз
				drawPixelXY(x + col, TEXT_BASELINE + string_offset + row, led_brightness > 1 && fl_5v ? color: CRGB::Red);
		}
	}
	return cw;
}

// отрисовка циферблата нестандартными шрифтами
int16_t printMedium(const char* txt, uint8_t font, int16_t pos, uint8_t limit, uint8_t start_char) {
	int16_t i = start_char;
	if( txt[i]==' ' ) pos += tiny_clock == FONT_WIDE ? -1: 1;
	if( txt[i] == '1' && tiny_clock == FONT_TINY) pos -= 1;
	while (txt[i] != '\0' && i<limit) {
		CRGB letterColor;
		if(show_time_color == 1) letterColor = CHSV(byte(pos << 3), 255, 255); // цвет в CHSV (прозрачность, оттенок, насыщенность, яркость) (0,0,255 - белый)
		else if(show_time_color == 2) letterColor = CHSV(byte(i << 5), 255, 255);
		else if(show_time_color == 3) letterColor = show_time_col[i % 8];
		else letterColor = show_time_color0;
		pos += drawMedium(txt[i++], pos, letterColor, font) + 1;
	}
	screenIsFree = false;
	fl_tiny_clock = true;

	return pos;
}

// изменение символа ":" между часами и минутами на другой стиль
const char* changeDots(char* txt) {
	static uint8_t stage = 1;
	switch (dots_style) {
		case 1: // Обычный 1
			txt[2] = stage & 2 ? ':': ' ';
			break;
		case 2: // Качели 0.5
			txt[2] = txt[2] == ':' ? 1: 2;
			break;
		case 3: // Качели 1
			txt[2] = stage & 2 ? 1: 2;
			break;
		case 4: // Биение 0.5
			txt[2] = txt[2] == ':' ? 3: 4;
			break;
		case 5: // Биение 1
			txt[2] = stage & 2 ? 3: 4;
			break;
		case 6: // Семафор 0.5
			txt[2] = txt[2] == ':' ? 5: 8;
			break;
		case 7: // Семафор 1
			txt[2] = stage & 2 ? 5: 8;
			break;
		case 8: // Капля
			txt[2] = stage + 4;
			break;
		case 9: // Четверть
			txt[2] = txt[2] == ':' ? getTime().tm_sec/15 + 5: ' ';
			break;
		case 10: // Мозаика
			txt[2] = stage == 1 ? 9:
					stage == 2 ? 1:
					stage == 3 ? 9: 2;
			break;
		case 11: // Статичный
			txt[2] = ':';
			break;
	}
	stage++;
	if(stage>4) stage = 1;
	return txt;
}