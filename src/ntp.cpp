/*
	Установка времени через NTP
*/

#include <Arduino.h>
#include <time.h>
#include "defines.h"
#include "settings.h"

time_t start_time = 0;
bool fl_needStartTime = true;
bool fl_timeNotSync = true;

void syncTime() {
	int tz           = tz_shift;
	int dst          = tz_dst;
	time_t now       = time(nullptr);
	unsigned timeout = 1000; // try for timeout
	unsigned start   = millis();
	configTime(tz * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
	LOG(print, PSTR("Waiting for NTP time sync: "));
	while (now < 86400 ) { // Сутки от 1го января 1970. Ждать пока время не установится. При повторной синхронизации не ждать.
		delay(20); // запрос происходит в асинхронном режиме и ждать первого обновления не обязательно, но для упрощения логики желательно
		LOG(print, ".");
		now = time(nullptr);
		if((millis() - start) > timeout) {
			LOG(println, PSTR("\n[ERROR] Failed to get NTP time."));
			return;
		}
	}
	if(fl_needStartTime) {
		start_time = now - millis()/1000;
		fl_needStartTime = false;
		if(sec_enable) save_log_file(SEC_TEXT_BOOT);
		last_telegram = now;
	}
	fl_timeNotSync = false;
	LOG(println, now);
}

// Function that gets current epoch time
time_t getTimeU() {
	return time(nullptr);
}

tm getTime(time_t *t) {
	time_t now = t ? *t: time(nullptr);
	tm timeinfo;
	// gmtime_r(&now, &timeinfo); // @NOTE doesn't work in esp2.3.0
	localtime_r(&now, &timeinfo);
	return timeinfo;
}

const char* getUptime(char *str) {
	time_t now = time(nullptr);
	time_t u = now - start_time;
	uint8_t s = u % 60;
	u /= 60;
	uint8_t m = u % 60;
	u /= 60;
	uint8_t h = u % 24;
	u /= 24;
	tm t = getTime(&start_time);
	sprintf_P(str,PSTR("up %dd %dh %02dm %02ds, from %04u-%02u-%02u %02u:%02u:%02u"), (int)u, h, m, s, t.tm_year +1900, t.tm_mon +1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	return str;
}