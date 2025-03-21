/*
	Работа с модулем часов.
	Модули бывают разные, по этому блок выделен отдельно, чтобы было удобно менять под разные платы
*/
#include <Arduino.h>
#include <RTClib.h>
#include <sys/time.h>
#include "defines.h"
#include "rtc.h"

RTC_DS1307 rtc1;
RTC_DS3231 rtc2;

// часы работают
uint8_t rtc_enable = 0;
// тип чипа RTC
uint8_t rtc_chip = 0;

uint8_t rtc_init() {
	if( rtc_chip == 1 ) {
		if( rtc1.begin() ) {
			rtc_enable = 1;
			if( ! rtc1.isrunning()) {
				// плата rtc после смены батарейки или если была остановлена не работает.
				// этот кусок запускает отсчёт времени датой компиляции прошивки. 
				LOG(println, PSTR("RTC is NOT running, let's set the time!"));
				// When time needs to be set on a new device, or after a power loss, the
				// following line sets the RTC to the date & time this sketch was compiled
				rtc1.adjust(DateTime(F(__DATE__), F(__TIME__)));
				// This line sets the RTC with an explicit date & time, for example to set
				// January 21, 2014 at 3am you would call:
				// rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
				return 2;
			}
		}
	} else if( rtc2.begin() ) {
		rtc_enable = 2;
		if (rtc2.lostPower()) {
			// как и в случае с DS1307, если питание было потеряно, то часы остановились
			// и их нужно перезапустить
			LOG(println, PSTR("RTC lost power, let's set the time!"));
			// When time needs to be set on a new device, or after a power loss, the
			// following line sets the RTC to the date & time this sketch was compiled
			rtc2.adjust(DateTime(F(__DATE__), F(__TIME__)));
			// This line sets the RTC with an explicit date & time, for example to set
			// January 21, 2014 at 3am you would call:
			// rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
			return 2;
		}
	}
	if( rtc_enable == 0) return 0;

	// часы запущены, установка времени системы по RTC
	rtc_setSYS();
	return 1;
}

// Установка времени системы по значениям из RTC
void rtc_setSYS() {
	if( ! rtc_enable ) return;
	DateTime now = rtc_enable == 1 ? rtc1.now(): rtc2.now();
/*
	// код для получения unixtime из отдельных частей даты
	struct tm tm;
	tm.tm_hour = now.hour();
	tm.tm_min = now.minute();
	tm.tm_year = now.year() - 1900;
	tm.tm_mon = now.month() - 1;
	tm.tm_mday = now.day();
	tm.tm_sec = now.second();
	tm.tm_isdst = tz_dst;
	time_t t2 = mktime(&tm);
	LOG(printf_P,"RTC time (old): %llu\n",t2);
*/
	time_t t = now.unixtime();
	LOG(printf_P, PSTR("RTC time: %lu\n"), t);
	t += (gs.tz_shift * 3600) + (gs.tz_dst * 3600); 
	// set the system time
	timeval tv = { t, 0 };
	settimeofday(&tv, nullptr);
}

// запись времени системы в RTC
void rtc_saveTIME(time_t t) {
	if( ! rtc_enable ) return;
	DateTime now = rtc_enable == 1 ? rtc1.now(): rtc2.now();
	// записать новое время только если оно не совпадает с текущим в RTC
	if(t != now.unixtime()) {
		if( rtc_enable == 1 ) rtc1.adjust(DateTime(t));
		else rtc2.adjust(DateTime(t));
		LOG(printf_P, PSTR("adjust RTC from %lu to %lu\n"), now.unixtime(), t);
	}
}

time_t getRTCTimeU() {
	if( ! rtc_enable ) return 0;
	DateTime now = rtc_enable == 1 ? rtc1.now(): rtc2.now();
	return now.unixtime();
}

/*
	Serial.print(" since midnight 1/1/1970 = ");
	Serial.print(now.unixtime());
	Serial.print("s = ");
	Serial.print(now.unixtime() / 86400L);
	Serial.println("d");
*/
/*
https://github.com/adafruit/RTClib/blob/master/examples/ds1307nvram/ds1307nvram.ino
*/

/*
	маленькое статическое ОЗУ только в чипе DS1307
	https://www.analog.com/media/en/technical-documentation/data-sheets/ds1307.pdf
	64 байта, первые 8 (0-7) это время, дальше 56 ячеек, которые можно использовать для любых целей
	Стандартный драйвер RTClib.h сдвигает адрес на 8, таким образом первая свободная ячейка становится 0.
*/

uint8_t fletcher8(uint8_t *data, uint16_t len) {
    uint16_t sum1 = 0xf, sum2 = 0xf;
    while( len-- ) {
        sum1 += *data++;
        sum2 += sum1;
    };
    sum1 = (sum1 & 0x0f) + (sum1 >> 4);
    sum1 = (sum1 & 0x0f) + (sum1 >> 4);
    sum2 = (sum2 & 0x0f) + (sum2 >> 4);
    sum2 = (sum2 & 0x0f) + (sum2 >> 4);
    return sum2<<4 | sum1;
}

// прочесть один байт из SRAM DS1307. 
uint8_t rtcGetByte(uint8_t address) {
	if( rtc_enable != 1 ) return 0xff;
	return rtc1.readnvram(address);
}

// прочесть блок и подсчитать простейшую контрольную сумму
uint8_t rtcReadBlock(uint8_t address, uint8_t *buf, uint8_t size) {
	if( rtc_enable != 1 ) return 0;
	// при чтении драйвер сам разбирает на порции
	rtc1.readnvram(buf, size, address);
	return fletcher8(buf, size);
}

// записать один байт
void rtcSetByte(uint8_t address, uint8_t data) {
	if( rtc_enable != 1 ) return;
	rtc1.writenvram(address, data);
}

// записать блок и подсчитать простейшую контрольную сумму
uint8_t rtcWriteBlock(uint8_t address, uint8_t *buf, uint8_t size) {
	if( rtc_enable != 1 ) return 255;
	// драйвер не может писать пакеты больше чем 32 байта, по этому надо делить на порции
	uint8_t siz = size;
	uint8_t addr = address;
	uint8_t sent = 0;
	while( siz > 0 ) {
		size_t len = std::min((uint8_t)30, siz);
		rtc1.writenvram(addr + sent , buf + sent, len);
		siz -= len;
		sent += len;
	}
	return fletcher8(buf, size);
}