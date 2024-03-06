// работа с бегущим текстом

#include <Arduino.h>

#include "runningText.h"
#include "fontsF.h"
#include "fontsV.h"
#include "defines.h"

// **************** НАСТРОЙКИ ****************
#define LET_WIDTH 5       // ширина буквы шрифта
#define LET_HEIGHT 8      // высота буквы шрифта
#define SPACE 1           // пробел

#define MAX_LENGTH 256
// --------------------- ДЛЯ РАЗРАБОТЧИКОВ ----------------------

int16_t currentOffset = WIDTH;
uint32_t _currentColor = 1;

char _runningText[MAX_LENGTH]; // текст, который будет крутиться
bool runningMode; // режим: true - по кругу, false - без прокрутки
bool screenIsFree = true; // экран свободен (текст полностью прокручен)

// ------------- СЛУЖЕБНЫЕ ФУНКЦИИ --------------

// интерпретатор кода символа в массиве fontHEX (для Arduino IDE 1.8.* и выше)
// Символы записаны не по строкам, а по колонкам, для удобства отображения
// letter - utf8 код символа, col - колонка, которую надо отобразить
uint8_t getFont(uint32_t letter, uint8_t col) {
	uint16_t cn = 0;

	if(letter >= 1 && letter <= 9) {
		if(col == LET_WIDTH) return 0x84;
		cn = letter - 1;
		return pgm_read_byte(&fontSemicolon[cn][col]);
	}

	if(col == LET_WIDTH && wide_font ) return (LET_HEIGHT << 4) | LET_WIDTH;
	if( letter < 0x7f ) // для английских букв и символов
		cn = letter-32;
	else if( letter >= 0xd090 && letter <= 0xd0bf ) // А-Яа-п (utf-8 символы идут не по порядку, надо собирать из кусков)
		cn = letter - 0xd090 + 95;
	else if( letter >= 0xd180 && letter <= 0xd18f ) // р-я
		cn = letter - 0xd180 + 143;
	else if( letter == 0xd081 ) // Ё
		cn = 159;
	else if( letter == 0xd191 ) // ё
		cn = 160;
	else if( letter >= 0xd084 && letter <= 0xd087 ) // Є-Ї
		cn = letter - 0xd084 + 161;
	else if( letter >= 0xd194 && letter <= 0xd197 ) // є-ї
		cn = letter - 0xd194 + 165;
	else if( letter == 0xd290 || letter == 0xd291 ) // Ґґ
		cn = letter - 0xd290 + 169;
	else if( letter == 0xc2b0 ) // °
		cn = 171;
	else if( letter == 0xc2ab || letter == 0xc2bb || (letter >= 0xe2809c && letter <= 0xe2809f) ) // "
		cn = 2;
	else if( letter >= 0xe28098 && letter <= 0xe2809b ) // '
		cn = 7;
	else if( letter >= 0xe28090 && letter <= 0xe28095 ) // -
		cn = 13;
	else if( letter == 0xe280a6 ) // ...
		cn = 172;
	else 
		cn = 162; // символ не найден, вывести пустой прямоугольник
	if( wide_font )	return pgm_read_byte(&(fontFix[cn][col]));
	return pgm_read_byte(&(fontVar[cn][col]));
}

// Отрисовка буквы с учётом выхода за край экрана
// index - порядковый номер буквы в тексте, нужно для подсвечивания разными цветами
// letter - буква, которую надо отобразить
// offset - позиция на экране. Может быть отрицательной, если буква уже уехала или больше ширины, если ещё не доехала
// color - режим цвета (1 - радуга: текст "переливается", 2 - по букве, 3 - режим циферблата: 2+1+2) или цвет в CRGB (прозрачность, красный, зелёный, синий)
int16_t drawLetter(uint8_t index, uint32_t letter, int16_t offset, uint32_t color) {
	uint8_t t = getFont(letter, LET_WIDTH);
	int8_t LW = t & 0xF; // ширина буквы
	int8_t start_pos = 0, finish_pos = LW;
	int8_t LH = t >> 4; // высота буквы
	if (LH > HEIGHT) LH = HEIGHT;

 	CRGB letterColor;
	if(color == 1) letterColor = CHSV(byte(offset << 3), 255, 255); // цвет в CHSV (прозрачность, оттенок, насыщенность, яркость) (0,0,255 - белый)
	else if(color == 2) letterColor = CHSV(byte(index << 5), 255, 255);
	else if(color == 3) letterColor = show_time_col[index % 5];
	else letterColor = color;

	if( offset < -LW || offset > WIDTH ) return LW; // буква за пределами видимости, пропустить
	if( offset < 0 ) start_pos = -offset;
	if( offset > WIDTH - LW ) finish_pos = WIDTH - offset;

	for (int8_t x = start_pos; x < finish_pos; x++) {
		// отрисовка столбца (x - горизонтальная позиция, y - вертикальная)
		uint8_t fontColumn = getFont(letter, x);
		for (int8_t y = 0; y < LH; y++)
			if(fontColumn & (1 << (LH - 1 - y))) 
				drawPixelXY(offset + x, TEXT_BASELINE + y, led_brightness > 1 && fl_5v ? letterColor: CRGB::Red);
	}
	return LW;
}

// отрисовка содержимого экрана с учётом подготовленных данных:
// runningMode: true - разовый вывод, false - прокрутка текста
// _runningText: буфер который надо отобразить
// currentOffset: позиция с которой надо отобразить
// screenIsFree: отрисовка завершена, экран свободен для нового задания
void drawString() {
	int16_t i = 0, j = 0, delta = 0;
	uint32_t c;
	while (_runningText[i] != '\0' && i < MAX_LENGTH) {
		// Выделение символа UTF-8
		// 0xxxxxxx - 7 бит 1 байт, 110xxxxx - 10 бит 2 байта, 1110xxxx - 16 бит 3 байта, 11110xxx - 21 бит 4 байта
		c = (byte)_runningText[i++];
		if( c > 127  ) {
			if( c >> 5 == 6 ) {
				c = (c << 8) | (byte)_runningText[i++];
			} else if( c >> 4 == 14 ) {
				c = (c << 8) | (byte)_runningText[i++];
				c = (c << 8) | (byte)_runningText[i++];
			} else if( c >> 3 == 30 ) {
				c = (c << 8) | (byte)_runningText[i++];
				c = (c << 8) | (byte)_runningText[i++];
				c = (c << 8) | (byte)_runningText[i++];
			}
		}
		delta += drawLetter(j++, c, currentOffset + delta, _currentColor) + SPACE;
	}

	if(runningMode) {
		screenIsFree = true;
	} else {
		currentOffset--;
		if(currentOffset < -delta) { // строка убежала
			if(runningMode==0)
				currentOffset = WIDTH + LET_WIDTH;
			screenIsFree = true;
		}
	}
}

void initRunning(uint32_t color, int16_t posX) {
	_currentColor = color > 3 ? maximizeBrightness(color): color;
	runningMode = posX >= 0 && posX <= WIDTH;
	currentOffset = runningMode ? posX: WIDTH;
	if(_runningText[0]==32) currentOffset -= wide_font ? (LET_WIDTH + SPACE) >> 1: LET_WIDTH >> 1;
	if(_runningText[0]==49 && wide_font) currentOffset--;
	screenIsFree = false;
}
// Инициализация строки, которая будет отображаться на экране
// txt - сама строка
// color - цвет 
// posX - стартовая позиция строки, если есть, то режим без прокрутки
void initRString(const char *txt, uint32_t color, int16_t posX) {
	strncpy(_runningText, txt, MAX_LENGTH);
	initRunning(color, posX);
}
void initRString(const __FlashStringHelper *txt, uint32_t color, int16_t posX) {
	strncpy_P(_runningText, (const char*)txt, MAX_LENGTH);
	initRunning(color, posX);
}
void initRString(String txt, uint32_t color, int16_t posX) {
	txt.toCharArray(_runningText, MAX_LENGTH);
	initRunning(color, posX);
}
