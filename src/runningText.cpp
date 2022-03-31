// работа с бегущим текстом

#include <Arduino.h>

#include "runningText.h"
#include "fonts.h"
#include "defines.h"

// **************** НАСТРОЙКИ ****************
#define MIRR_V 0          // отразить текст по вертикали (0 / 1)
#define MIRR_H 0          // отразить текст по горизонтали (0 / 1)

#define TEXT_HEIGHT 0     // высота, на которой бежит текст (от низа матрицы)
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
	if( letter < 0x7f ) // для английских букв и символов
		return pgm_read_byte(&(fontHEX[letter-32][col]));
	else if( letter >= 0xd090 && letter <= 0xd0bf ) // А-Яа-п (utf-8 символы идут не по порядку, надо собирать из кусков)
		return pgm_read_byte(&(fontHEX[letter - 0xd090 + 95][col]));
	else if( letter >= 0xd180 && letter <= 0xd18f ) // р-я
		return pgm_read_byte(&(fontHEX[letter - 0xd180 + 143][col]));
	else if( letter == 0xd081 ) // Ё
		return pgm_read_byte(&(fontHEX[159][col]));
	else if( letter == 0xd191 ) // ё
		return pgm_read_byte(&(fontHEX[160][col]));
	else if( letter >= 0xd084 && letter <= 0xd087 ) // Є-Ї
		return pgm_read_byte(&(fontHEX[letter - 0xd084 + 161][col]));
	else if( letter >= 0xd194 && letter <= 0xd197 ) // є-ї
		return pgm_read_byte(&(fontHEX[letter - 0xd194 + 165][col]));
	else if( letter == 0xd290 || letter == 0xd291 ) // Ґґ
		return pgm_read_byte(&(fontHEX[letter - 0xd290 + 169][col]));
	else if( letter == 0xb0 ) // °
		return pgm_read_byte(&(fontHEX[171][col]));
	return pgm_read_byte(&(fontHEX[162][col])); // символ не найден, вывести пустой прямоугольник
}

// Отрисовка буквы с учётом выхода за край экрана
// index - порядковый номер буквы в тексте, нужно для подсвечивания разными цветами
// letter - буква, которую надо отобразить
// offset - позиция на экране. Может быть отрицательной, если буква уже уехала или больше ширины, если ещё не доехала
// color - режим цвета (1 - радуга, 2 - по букве) или номер цвета в CHSV (оттенок, насыщенность, яркость) (0,0,255 - белый)
void drawLetter(uint8_t index, uint32_t letter, int16_t offset, uint32_t color) {
	int8_t start_pos = 0, finish_pos = LET_WIDTH;
	int8_t LH = LET_HEIGHT;
	if (LH > HEIGHT) LH = HEIGHT;
	int8_t offset_y = (HEIGHT - LH) >> 1;     // по центру матрицы по высоте

 	CRGB letterColor;
	if(color == 1) letterColor = CHSV(byte(offset << 3), 255, 255);
	else if(color == 2) letterColor = CHSV(byte(index << 5), 255, 255);
	else if(color == 3) letterColor = show_time_col[index];
	else letterColor = color;

	if (offset < -LET_WIDTH || offset > WIDTH) return; // буква за пределами видимости, пропустить
	if (offset < 0) start_pos = -offset;
	if (offset > (WIDTH - LET_WIDTH)) finish_pos = WIDTH - offset;

	for (byte i = start_pos; i < finish_pos; i++) {
		byte thisByte;
		if (MIRR_V) thisByte = getFont(letter, LET_WIDTH - 1 - i);
		else thisByte = getFont(letter, i);

		for (byte j = 0; j < LH; j++) {
			bool thisBit;

			if (MIRR_H)	thisBit = thisByte & (1 << j);
			else thisBit = thisByte & (1 << (LH - 1 - j));

			// рисуем столбец (i - горизонтальная позиция, j - вертикальная)
			if (thisBit) drawPixelXY(offset + i, offset_y + TEXT_HEIGHT + j, led_brightness > 1 && fl_5v ? letterColor: CRGB::Red);
		}
	}
}

// отрисовка содержимого экрана с учётом подготовленных данных:
// runningMode: true - разовый вывод, false - прокрутка текста
// _runningText: буфер который надо отобразить
// currentOffset: позиция с которой надо отобразить
// screenIsFree: есть ли подготовленные данные, иначе пропускать
bool drawString() {
	int16_t i = 0, j = 0;
	uint32_t c;
	while (_runningText[i] != '\0') {
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
		drawLetter(j, c, currentOffset + j * (LET_WIDTH + SPACE), _currentColor);
		j++;
	}

	if(runningMode) {
		screenIsFree = true;
	} else {
		currentOffset--;
		if(currentOffset < -j * (LET_WIDTH + SPACE)) {    // строка убежала
			if(runningMode==0)
				currentOffset = WIDTH + LET_WIDTH;
			screenIsFree = true;
		}
	}
	return true;
}

void initRunning(uint32_t color, int16_t posX) {
	_runningText[MAX_LENGTH-1] = 0;
	_currentColor = color > 3 ? maximizeBrightness(color): color;
	runningMode = posX >= 0 && posX <= WIDTH;
	currentOffset = runningMode ? posX: WIDTH;
	if(_runningText[0]==32) currentOffset -= (LET_WIDTH + SPACE) >> 1;
	if(_runningText[0]==49) currentOffset--;
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
