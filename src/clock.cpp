/*
	Работа с отображением часов (времени, даты)
*/

#include <Arduino.h>
// #include "defines.h"
#include "clock.h"
#include "ntp.h"

extern bool fl_timeNotSync;

const char* clockCurrentText(char *a) {
	if( fl_timeNotSync ) {
		sprintf_P(a, PSTR("--%c--"), millis() & 512 ?':':' ');
	} else {
		tm t = getTime();
		sprintf_P(a, PSTR("%02u%c%02u"), t.tm_hour, millis() & 512 ?':':' ', t.tm_min);
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
