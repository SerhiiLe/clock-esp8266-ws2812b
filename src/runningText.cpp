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

#define MAX_LENGTH 1024
// --------------------- ДЛЯ РАЗРАБОТЧИКОВ ----------------------

int16_t currentOffset = WIDTH;
uint32_t _currentColor = 1;

char _runningText[MAX_LENGTH]; // текст, который будет крутиться
bool runningMode; // режим: true - разовый вывод, false - прокрутить строку

// ------------- СЛУЖЕБНЫЕ ФУНКЦИИ --------------

// Отрисовка буквы с учётом выхода за край экрана
// letter - буква, которую надо отобразить
// offset - позиция на экране. Может быть отрицательной, если буква уже уехала или больше ширины, если ещё не доехала
// color - режим цвета (1 - радуга: текст "переливается", 2 - по букве, 3 - режим циферблата: 2+1+2) или цвет в CRGB (прозрачность, красный, зелёный, синий)
// index - порядковый номер буквы в тексте, нужно для подсвечивания разными цветами
int16_t drawLetter(uint32_t letter, int16_t offset, uint32_t color, uint16_t index) {
	uint16_t cn = 0;
	uint8_t metric;
	const uint8_t* pointer;

	// костыль для отрисовки нестандартными шрифтами
	if(letter >= 1 && letter <= 9) { // заменители двоеточия
		metric = 0x84;
		pointer = fontSemicolon[letter - 1];
		goto m1; // пропустить проверку на стандартные шрифты
	}
	else if( letter == 0x7f ) { // заменитель пробела
		metric = 0x84;
		pointer = fontVar[cn]; // пробел это первый символ в массиве
		goto m1;
	}

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
	else if( letter == 0xc2a0 ) // "NO-BREAK SPACE"
		cn = 0;
	else if( letter == 0xe28496 ) // №
		cn = 3; // 3 - # или 46 - N
	else
		cn = 162; // символ не найден, вывести пустой прямоугольник

	// с номером буквы в массиве шрифта определились, возвращаем указатель на неё и метрику
	metric = gs.wide_font ? (LET_HEIGHT << 4) | LET_WIDTH : pgm_read_byte(&(fontVar[cn][LET_WIDTH]));
	pointer = gs.wide_font ? fontFix[cn]: fontVar[cn];

	m1:

	uint8_t LW = metric & 0xF; // ширина буквы
	uint8_t LH = metric >> 4; // высота буквы
	if (LH > HEIGHT) LH = HEIGHT;

	// отрисовка буквы
	drawChar(pointer, offset, TEXT_BASELINE, LW, LH, color, index);

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
		delta += drawLetter(c, currentOffset + delta, _currentColor, j++) + SPACE;
	}

	if(runningMode) {
		screenIsFree = true;
		_runningText[0] = 0;
	} else {
		currentOffset--;
		if(currentOffset < -delta) { // строка полностью пробежала
			screenIsFree = true;
			_runningText[0] = 0;
			last_time_display = millis();
		}
	}
}

void initRunning(uint32_t color, int16_t posX) {
	_currentColor = color > 5 ? maximizeBrightness(color): color;
	runningMode = posX >= 0 && posX <= WIDTH; // если указана позиция и она в рамках экрана - текст не прокручивать
	currentOffset = runningMode ? posX: WIDTH; // установить начальную позицию в указанную или по правому краю матрицы
	if(_runningText[0]==32) currentOffset -= gs.wide_font ? (LET_WIDTH + SPACE) >> 1: LET_WIDTH >> 1;
	if(_runningText[0]==49 && gs.wide_font) currentOffset--;
	screenIsFree = false;
	itsTinyText = false;
	fl_tiny_clock = false;
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
