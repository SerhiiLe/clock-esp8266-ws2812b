/*
	Работа с отображением часов (времени, даты)
*/

#include <Arduino.h>
// #include "defines.h"
#include "clock.h"
#include "ntp.h"

extern bool fl_timeNotSync;

const char* clockCurrentText(char *a, bool fl_12) {
	char c = millis() & 512 ?':':' ';
	if( fl_timeNotSync ) {
		sprintf_P(a, PSTR("--%c--"), c);
	} else {
		tm t = getTime();
		uint8_t hour = t.tm_hour;
		if(fl_12) {
			if(hour > 12) hour -= 12;
			if(hour == 0) hour = 12;
		}
		sprintf_P(a, PSTR("%02u%c%02u"), hour, c, t.tm_min);
		if(a[0] == '0') a[0] = ' ';
	}
	return a;
}

// вывод в строку текущего времени для крошечного шрифта
const char* clockTinyText(char *a, bool fl_12) {
	char c = millis() & 512 ?':': 0x7f;
	if( fl_timeNotSync ) {
		sprintf_P(a, PSTR("--%c--%c--"), c, c);
	} else {
		tm t = getTime();
		if(fl_12) {
			uint8_t hour = t.tm_hour;
			char AmPm[] = "am";
			if(hour>12) {
				hour-=12;
				strcpy(AmPm,"pm");
			}
			if(hour == 0) hour = 12;
			sprintf_P(a, PSTR("%02u%c%02u\x7f%s"), hour, c, t.tm_min, AmPm);
		} else 
			sprintf_P(a, PSTR("%02u%c%02u%c%02u"), t.tm_hour, c, t.tm_min, c, t.tm_sec);
		if(a[0] == '0') a[0] = ' ';
	}
	return a;
}

const char* dateCurrentTextShort(char *a) {
	tm t = getTime();
	const char *sW = nullptr;

	switch (t.tm_wday) {
		case 0: sW = PSTR("Вск"); break;
		case 1: sW = PSTR("Пнд"); break;
		case 2: sW = PSTR("Втр"); break;
		case 3: sW = PSTR("Срд"); break;
		case 4: sW = PSTR("Чтв"); break;
		case 5: sW = PSTR("Птн"); break;
		case 6: sW = PSTR("Сбт"); break;
	}
	sprintf_P(a, PSTR("%s %u.%02u.%u"), sW, t.tm_mday, t.tm_mon +1, t.tm_year +1900);
	return a;
}

const char* dateCurrentTextLong(char *a) {
	tm t = getTime();
	const char *sW = nullptr;
	const char *sM = nullptr;

	switch (t.tm_wday) {
		case 0: sW = PSTR("Воскресенье"); break;
		case 1: sW = PSTR("Понедельник");  break;
		case 2: sW = PSTR("Вторник");    break;
		case 3: sW = PSTR("Среда");      break;
		case 4: sW = PSTR("Четверг");    break;
		case 5: sW = PSTR("Пятница");    break;
		case 6: sW = PSTR("Суббота");    break;
	}  
	switch (t.tm_mon) {
		case  0: sM = PSTR("января");   break;
		case  1: sM = PSTR("февраля");  break;
		case  2: sM = PSTR("марта");    break;
		case  3: sM = PSTR("апреля");   break;
		case  4: sM = PSTR("мая");      break;
		case  5: sM = PSTR("июня");     break;
		case  6: sM = PSTR("июля");     break;
		case  7: sM = PSTR("августа");  break;
		case  8: sM = PSTR("сентября"); break;
		case  9: sM = PSTR("октября");  break;
		case 10: sM = PSTR("ноября");   break;
		case 11: sM = PSTR("декабря");  break;
	}
	sprintf_P(a, PSTR("%s %u %s %u года"), sW, t.tm_mday, sM, t.tm_year +1900);
	return a;
}
