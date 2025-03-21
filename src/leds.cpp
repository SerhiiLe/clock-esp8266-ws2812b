// разные функции для работы с LED матрицей

#include <Arduino.h>
// #include <FastLED.h>
#include "defines.h"
#include "leds.h"
#include "runningText.h"
#include "textTiny.h"

CRGB leds[NUM_LEDS];

bool screenIsFree = true; // экран свободен (текст полностью прокручен, слайды выведены)
bool itsTinyText = false; // сейчас время выводить слайды (tiny) вместо бегущей строки

// залить все
void fillAll(CRGB color) {
	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i] = color;
	}
}

// очистить всё
void clearALL() {
	FastLED.clearData();
}

/*
отрисовка буквы
pointer - указатель на первый байт буквы в массиве шрифта
startX - начальная позиция по горизонтали, 0 левый край
startY - начальная позиция по вертикали, 0 нижний край
LH - высота буквы
LW - ширина буквы
color - цвет или режим цвета
index - порядковый номер буквы в тексте, нужно для подсвечивания разными цветами
*/
CRGB drawChar(const uint8_t* pointer, int16_t startX, int16_t startY, uint8_t LH, uint8_t LW, uint32_t color, uint16_t index) {
	uint8_t start_col = 0, finish_col = LW;
	CRGB pixel_color, letterColor;

	uint8_t hs = gs.hue_shift ? hue_shift: 0;
	if(color == 1) letterColor = CHSV(byte((startX << 3) + hs), 255, 255); // цвет в CHSV (прозрачность, оттенок, насыщенность, яркость) (0,0,255 - белый)
	else if(color == 3) letterColor = CHSV(byte((index << 5) + hs), 255, 255);
	else if(color == 5) letterColor = gs.show_time_col[index & 0x07];
	else letterColor = color;

	if( startX < -LW || startX > WIDTH ) return CRGB::Black; // буква за пределами видимости, пропустить
	if( startX < 0 ) start_col = -startX;
	if( startX > WIDTH - LW ) finish_col = WIDTH - startX;

	for (int8_t x = start_col; x < finish_col; x++) {
		// отрисовка столбца (x - горизонтальная позиция, y - вертикальная)
		if(color == 2) letterColor = CHSV(byte(((startX + x) << 3) + hs), 255, 255);
		if(color == 4) letterColor = CHSV(byte(((index * LW + x) << 2) + hs), 255, 255);
		uint8_t fontColumn = pgm_read_byte(pointer + x); // все шрифты читаются как однобайтовые, 8 точек высотой!
		for (int8_t y = 0; y < LH; y++) {
			if( y + startY + TEXT_BASELINE >= 0 && y + startY + TEXT_BASELINE < HEIGHT ) { // отображать только видимую часть
				// в этой матрице отрисовка снизу вверх, шрифты записаны сверху вниз, ставить надо все точки, черные тоже
				// pixel_color = fontColumn & (1 << (LH - 1 - y)) ? (led_brightness > 1 && fl_5v ? letterColor: CRGB::Red) : CRGB::Black;
				// drawPixelXY(startX + x, startY + y + TEXT_BASELINE, pixel_color);
				pixel_color = led_brightness > 1 && fl_5v ? letterColor: CRGB::Red;
				if( fontColumn & (1 << (LH - 1 - y)) )
					drawPixelXY(startX + x, startY + y + TEXT_BASELINE, pixel_color);
			}
		}
	}
	return pixel_color;
}

// функция отрисовки точки по координатам X Y
void drawPixelXY(int8_t x, int8_t y, CRGB color) {
	if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
	int thisPixel = getPixelNumber(x, y) * SEGMENTS;
	for (byte i = 0; i < SEGMENTS; i++) {
		leds[thisPixel + i] = color;
	}
}

// функция получения цвета пикселя по его номеру
uint32_t getPixColor(int num) {
	int thisPixel = num * SEGMENTS;
	if (thisPixel < 0 || thisPixel > NUM_LEDS - 1) return 0;
	return (((uint32_t)leds[thisPixel].r << 16) | ((long)leds[thisPixel].g << 8 ) | (long)leds[thisPixel].b);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixColorXY(int8_t x, int8_t y) {
	return getPixColor(getPixelNumber(x, y));
}

// Увеличение яркости до максимума с сохранением оттенка
uint32_t maximizeBrightness(uint32_t color, uint8_t limit)  {
	uint32_t red = (color >> 16) & 255;
	uint32_t green = (color >> 8) & 255;
	uint32_t blue = color & 255;
	uint32_t max = red;
	if( green > max) max = green;
	if( blue > max) max = blue;
	uint32_t factor = ((uint32_t)limit * 256) / max;
	red = (red * factor) >> 8;
	green = (green * factor) >> 8;
	blue = (blue * factor) >> 8;
	return (red << 16) | (green << 8) | blue;
}

// **************** НАСТРОЙКА МАТРИЦЫ ****************
#if (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y

#elif (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 1)
#define _WIDTH HEIGHT
#define THIS_X y
#define THIS_Y x

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 3)
#define _WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y x

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 2)
#define _WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 3)
#define _WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y (WIDTH - x - 1)

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 2)
#define _WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y y

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 1)
#define _WIDTH HEIGHT
#define THIS_X y
#define THIS_Y (WIDTH - x - 1)

#else
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y
#pragma message "Wrong matrix parameters! Set to default"

#endif

// получить номер пикселя в ленте по координатам
uint16_t getPixelNumber(int8_t x, int8_t y) {
	if(gs.turn_display & 1) x = WIDTH - x - 1;
	if(gs.turn_display & 2) y = HEIGHT - y - 1;
	if ((THIS_Y % 2 == 0) || MATRIX_TYPE) {               // если чётная строка
		return (THIS_Y * _WIDTH + THIS_X);
	} else {                                              // если нечётная строка
		return (THIS_Y * _WIDTH + _WIDTH - THIS_X - 1);
	}
}

uint8_t led_brightness = 0;
void set_brightness(uint8_t br) {
	if(br != led_brightness) {
		FastLED.setBrightness(br);
		led_brightness = br;
	}
}

void display_setup() {
	FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
	set_brightness(BRIGHTNESS);
	if(DEFAULT_POWER > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, DEFAULT_POWER);
	FastLED.clear();

	// проверка правильности ориентации
	// левый нижний угол - начало координат
	drawPixelXY(0, 0, CRGB::Red);             // левый нижний красный
	drawPixelXY(0, HEIGHT - 1, CRGB::Blue);   // левый верхний синий
	drawPixelXY(WIDTH - 1, 0, CRGB::Green);   // правый нижний зелёный

	FastLED.show();
	// delay(1000);
}

void display_tick(bool clear) {
	if(screenIsFree) return;
	if(itsTinyText) {
		drawSlide();
	} else if(fl_tiny_clock) {
		screenIsFree = true;
		fl_tiny_clock = false;
	} else {
		// очистить буфер для заполнения новыми данными, чтобы не накладывались кадры
		// только в режиме циферблата и бегущей строки
		if( clear ) FastLED.clearData();
		drawString();
	}

	if(!fl_allowLEDS) return;
	#ifndef LED_MOTION
	if(fl_led_motion) drawPixelXY(WIDTH - 1, 0, CRGB::Green);
	#endif
	unsigned long start = 0;
	// вывод часто срывается, по этому выводить кадр пока не получится. Признак неудачи - отклонение > 25%
	do {
		start = micros();
		FastLED.show();
		// if(abs((long)(micros()-start-REFRESH_TIME))>(REFRESH_TIME >> 2))
			// LOG(printf_P, PSTR("время обновления: %d (%d)\n"), micros()-start, REFRESH_TIME);
	} while (abs((long)(micros()-start-REFRESH_TIME))>(REFRESH_TIME >> 2));
}
