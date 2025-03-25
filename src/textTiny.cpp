/*
Вывод циферблата или других надписей крошечным и корявым шрифтом 3х5
*/

#include <Arduino.h>
#include "defines.h"
#include "leds.h"
#include "textTiny.h"
#include "fontTini.h"

#define MAX_LENGTH 256	// максимальная длина пачки слайдов
#define SPACE 1			// отступ между буквами
#define LET_HEIGHT 5      // высота буквы шрифта

char _outText[MAX_LENGTH]; // текст, который будет выводится в виде слайдов
int16_t _baseX = 0, _baseY = 0;
unsigned long _prevDraw = 0; // время последнего обновления экрана
bool _oneSlide = true; // вывести слайд без ожидания времени показа слайда
int16_t _curPosition = 0; // текущая позиция в тексте
int16_t _lastPosition = 0; // последняя позиция в тексте
int16_t _curY = 0; // текущее смещение по Y
uint32_t _tinyColor = 1;

// x - начальная позиция по горизонтали
// y - начальная позиция по вертикали
// c - символ/буква
// color - цвет
// index - символа в слайде
int16_t drawTinyLetter(int16_t x, int16_t y, uint32_t c, uint32_t color, int16_t index) {
	byte dots;
	uint8_t cn = 0;
	uint8_t fw = 3;
	if( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ) // A-Z maps to 1-26
		cn = c & 0x1F;
	else if (c >= 0x21 && c <= 0x40)
		cn = c - 0x21 + 27;
	else if (c == ' ') // space
		cn = 0;
	else if (c>=0xd090 && c<=0xd0af) // А-Я
		cn = c - 0xd090 + 59;
	else if (c>=0xd0b0 && c<=0xd0bf) // а-п
		cn = c - 0xd0b0 + 59;
	else if (c>=0xd180 && c<=0xd18f) // р-я
		cn = c - 0xd180 + 75;
	else if (c==0xd081 || c==0xd191) // Ёё
		cn = 64;
	else if (c==0xd084 || c==0xd194) // Єє
		cn = 91;
	else if (c==0xd086 || c==0xd196) // Іі
		cn = 9;
	else if (c==0xd087 || c==0xd197) // Її
		cn = 92;
	else if (c==0xd290 || c==0xd291) // Ґґ
		cn = 93;
	else if (c==0xc2b0) // °
		cn = 94;

	if( cn==27 || cn==33 || cn==38 || cn==40 || cn==52 || cn==53 )
		fw = 1;
	if( cn==34 || cn==35 || cn==0 )
		fw = 2;
	if( c==0x7f ) {
		fw = 1; cn = 0;
	}

	drawChar(fontTiny[cn], x, y, fw, LET_HEIGHT, color, index);

	// костыль для отрисовки буквы Ю, единственной которая ну совсем не лезет в три пикселя ширины, хотя сказать, что другие сильно хорошо выглядят тоже неправда.
	if( cn==89 ) {
		drawChar(fontTinyU, x+3, y, 1, LET_HEIGHT, color, index);
		fw++;
	}

	return fw;
}

void drawSlide() {
	int16_t i = _curPosition, j = _curPosition, delta = 0;
	uint32_t c;
	bool fl_draw = false;
	// сдвиг содержимого экрана на одну строку вниз (ужасно не эффективно и медленно, но работает)
	// только если строка ещё не достигла "базовой линии"
	if(_curY <= _baseY && ! _oneSlide) {
		for(int16_t x=0; x<WIDTH; x++) {
			for(int16_t y=HEIGHT-1; y>0 || y>_curY+5; y--)
				drawPixelXY(x,y,getPixColorXY(x,y-1));
		}
		for(int16_t x=0; x<WIDTH; x++)
			drawPixelXY(x,0,0);
	}
	// отрисовка последовательности символов до конца строки (\0) или перевода строки (\n)
	// или пропустить если строка выведена и находится на базовой линии
	while (_outText[i] != '\0' && i < MAX_LENGTH && _outText[i] != '\n' && _curY <= _baseY) {
		// Выделение символа UTF-8
		// 0xxxxxxx - 7 бит 1 байт, 110xxxxx - 10 бит 2 байта, 1110xxxx - 16 бит 3 байта, 11110xxx - 21 бит 4 байта
		c = (byte)_outText[i++];
		if( c > 127  ) {
			if( c >> 5 == 6 ) {
				c = (c << 8) | (byte)_outText[i++];
			} else if( c >> 4 == 14 ) {
				c = (c << 8) | (byte)_outText[i++];
				c = (c << 8) | (byte)_outText[i++];
			} else if( c >> 3 == 30 ) {
				c = (c << 8) | (byte)_outText[i++];
				c = (c << 8) | (byte)_outText[i++];
				c = (c << 8) | (byte)_outText[i++];
			}
		}
		delta += drawTinyLetter(_baseX + delta, _curY, c, _tinyColor, j++) + SPACE;
		fl_draw = true;
	}

	// это разовая отрисовка - обычно циферблат
	if(_oneSlide) {
		screenIsFree = true;
		return;
	}

	// фиксация времени последней отрисовки, нужно для задержки при показе слайда
	if(fl_draw) {
		_prevDraw = millis();
		_curY++;
		_lastPosition = i;
	}

	// проверка времени задержки показа слайда
	if(millis() - _prevDraw < 1000ul * gs.slide_show) return;

	// переход на новый слайд (\n) или завершение
	if(_outText[_lastPosition] != 0 && _outText[_lastPosition+1] != 0) {
		_curPosition = _lastPosition+1;
		_curY = _baseY - HEIGHT;
	} else {
		screenIsFree = true;
		last_time_display = millis();
	}
}

/*
*txt - text to draw
color - color of text
posX - start position
instant - only draw, without animation
clear - clear buffer before draw
*/
void printTinyText(const char *txt, uint32_t color, int16_t posX, bool instant, bool clear) {
	strncpy(_outText, txt, MAX_LENGTH);
	_baseX = posX;
	if(instant) {
		if(_outText[0] == ' ') _baseX -= 1;
		if(_outText[0] == '1' && gs.tiny_clock == FONT_TINY) _baseX -= 1;
	}
	_baseY = 1;
	_curY = instant ? _baseY: _baseY - HEIGHT;
	_curPosition = 0;
	itsTinyText = true;
	screenIsFree = false;
	_oneSlide = instant;
	_tinyColor = color;
	if( clear ) clearALL();
}