/*
	Установка времени через NTP
*/

#include <Arduino.h>
#include "defines.h"
#include "settings.h"
#include "rtc.h"
#include <WiFiUdp.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif


#define REQUEST_TIMEOUT 6000 // таймаут запроса к NTP серверу (ms). 
#define NTP_LOCAL_PORT 1234 // порт с которого делаются запросы
#define NTP_PORT 123 // порт NTP (123 по стандарту)

// сервера времени
const char PROGMEM NTP1[] = "pool.ntp.org";
const char PROGMEM NTP2[] = "time.nist.gov";
const char PROGMEM NTP3[] = "pool.time.in.ua";
const char PROGMEM NTP4[] = "ntp.ps.od.ua";
const char* PROGMEM NTP[] = {NTP1, NTP2, NTP3, NTP4};

WiFiUDP ntp_udp;

time_t start_time = 0;
bool fl_needStartTime = true;
bool fl_timeNotSync = true;
bool fl_ntpRequestIsSend = false;
unsigned long request_time = 0;

void DuskTillDawn();

/*
	встроенная функция даёт странные эффекты на слабом интернете, пришлось получать время "ручками"
	за основу взята реализация https://github.com/GyverLibs/GyverNTP
 */

// формирование и отправка запроса
bool syncTimeRequest() {
	static uint8_t server_num = 0;
	char host[128];
	uint8_t buf[48];
	memset(buf, 0, 48);
	// https://ru.wikipedia.org/wiki/NTP
	buf[0] = 0b11100011;                    // LI 0x3, v4, client
	ntp_udp.begin(NTP_LOCAL_PORT);
	strncpy_P(host, NTP[server_num], sizeof(host)); // выбор сервера ntp из массива
	LOG(printf, PSTR("NTP host: %s index: %u\n"), host, server_num);
	if(!ntp_udp.beginPacket(host, NTP_PORT)) return false;
	ntp_udp.write(buf, 48);
	if(!ntp_udp.endPacket()) return false;
	request_time = millis();
	ntp_udp.flush();
	delay(0);
	fl_ntpRequestIsSend = true;
	server_num = (server_num + 1) % (sizeof(NTP)/sizeof(char*)); // прокрутка номера сервера
	return true;
}

// получение ответа от сервера
time_t ntpReadAnswer() {
	if(ntp_udp.remotePort() != NTP_PORT) return 0;     // не наш порт
	uint8_t buf[48];
	ntp_udp.read(buf, 48);                      // читаем
	if (buf[40] == 0) return 0;         // некорректное время
	unsigned long rtt = millis() - request_time;          // время между запросом и ответом
	uint16_t a_ms = ((buf[44] << 8) | buf[45]) * 1000L >> 16;    // мс ответа сервера
	uint16_t r_ms = ((buf[36] << 8) | buf[37]) * 1000L >> 16;    // мс запроса клиента
	int16_t err = a_ms - r_ms;          // время обработки сервером
	if (err < 0) err += 1000;           // переход через секунду
	rtt = (rtt - err) / 2;              // текущий пинг
	uint32_t _unix = (((uint32_t)buf[40] << 24) | ((uint32_t)buf[41] << 16) | ((uint32_t)buf[42] << 8) | buf[43]);    // 1900
	_unix -= 2208988800ul;                  // перевод в UNIX (1970)
	_unix += rtt / 1000;
	return _unix;
}

// запрос на синхронизацию времени
bool syncTime() {
	static uint8_t ntp_try = 0;
	static bool fl_next_try = false;

	// если запрос ещё не отправлен, то отправить
	if( ! fl_ntpRequestIsSend || fl_next_try ) {
		if(syncTimeRequest()) {
			LOG(println, PSTR("Request for NTP time sync is send"));
		} else {
			LOG(println, PSTR("Error in send request to NTP server"));
		}
		if( !fl_next_try ) ntp_try = 0;
		fl_next_try = false;
		return false;
	}
	// ожидание ответа в течении REQUEST_TIMEOUT
	if((millis() - request_time) < REQUEST_TIMEOUT) {
		if(ntp_udp.parsePacket() == 48) {
			time_t t = ntpReadAnswer();
			LOG(printf_P,PSTR("Got from NTP: %lu (GMT)\n"),t);
			if( t > 0 ) {
				// что-то похожее на время получено, устанавливаем его как системное + сдвиг часового пояса
				time_t tz = (gs.tz_shift + gs.tz_dst)*3600;
				time_t now = t + tz;
				timeval tv = { now, 0 };
				settimeofday(&tv, nullptr);
				fl_timeNotSync = false;
				fl_ntpRequestIsSend = false;
				ntp_udp.stop();
				LOG(printf_P,PSTR("Set time from NTP to: %lu (GMT)\n"),t);
				if(fl_needStartTime) {
					start_time = now - millis()/1000;
					fl_needStartTime = false;
				}
				rtc_saveTIME(t); // сохранить полученное время в модуле часов RTC (GMT)
				DuskTillDawn();
				return true;
			}
		}
	}
	// Запрос не прошел, повезёт в следующий раз
	if((millis() - request_time) > REQUEST_TIMEOUT) {
		ntp_udp.stop();
		fl_ntpRequestIsSend = false;
		LOG(println, PSTR("NTP sync failed"));
		if( ++ntp_try < sizeof(NTP)/sizeof(char*) ) {
			// время ожидания вышло, но ещё не все сервера перебраны, следующая попытка
			fl_next_try = true;
			return syncTime();
		}
		if( time(nullptr) > 86400 ) {
			// время как-то установлено, может быть руками, может через web, может платой RTC
			// всё как и при успешном обновлении, но без обновления RTC
			if(fl_needStartTime) {
				start_time = time(nullptr) - millis()/1000;
				fl_needStartTime = false;
			}
			DuskTillDawn();
			return true;
		}
		// время не установлено, наверное отсутствует плата RTC, мы в 1 января 1970 года, надо долбится до посинения
	}
	return false;
}

// Function that gets current epoch time
time_t getTimeU() {
	return time(nullptr);
}

tm getTime(time_t *t) {
	time_t now = t ? *t: time(nullptr);
	tm timeInfo;
	// gmtime_r(&now, &timeInfo); // @NOTE doesn't work in esp2.3.0
	localtime_r(&now, &timeInfo);
	return timeInfo;
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

/* Sunset/Sunrise calculate from https://web.archive.org/web/20161229042556/http://williams.best.vwh.net/sunrise_sunset_example.htm */
/* вычисление времени рассвета и заката, примерное +/- 5 минут. При вычислении много готовых коэффициентов и переходов DEG -> RAD -> DEG */

uint16_t sunrise = 0;
uint16_t sunset = 0;

float RangeF(float f, float a, float b) {
  if (b <= a) { return a; }       // inconsistent, do what we can
  float range = b - a;
  float x = f - a;                // now range of x should be 0..range
  x = fmodf(x, range);            // actual range is now -range..range
  if (x < 0.0f) { x += range; }   // actual range is now 0..range
  return x + a;                   // returns range a..b
}

void sunCalculate(const tm &now, const bool isRise, uint8_t &hour, uint8_t &minute) {
	// Inputs:
	// day, month, year:      date of sunrise/sunset
	// latitude, longitude:   location for sunrise/sunset
	// zenith:                Sun's zenith for sunrise/sunset
	//   official      = 90 degrees 50'
	//   civil        = 96 degrees
	//   nautical     = 102 degrees
	//   astronomical = 108 degrees
  
	const float Half_PI = PI/2.0f;
	const float DEG = RAD_TO_DEG; // 180/PI;
	const float RAD = DEG_TO_RAD; // PI/180;

	float zenith = 90.8f;
	switch(gs.boost_mode) {
		case 2:
			zenith = 96.0f;
			break;
		case 3:
			zenith = 102.0f;
			break;
		case 4:
			zenith = 108.0f;
		break;
	}
	int8_t localOffset = gs.tz_shift + gs.tz_dst;

	uint16_t year = now.tm_year + 1900;
	uint16_t month = now.tm_mon + 1;
	uint16_t day = now.tm_mday;

	// first calculate the day of the year
	uint16_t N = floor(275 * month / 9) - (floor((month + 9) / 12) * (1 + floor((year - 4 * floor(year / 4) + 2) / 3))) + day - 30;

  	// 2. convert the longitude to hour value and calculate an approximate time
  	float lngHour = gs.longitude / 15;

	// if rising time is desired:
	// t = N + ((6 - lngHour) / 24);
	// if setting time is desired:
	// t = N + ((18 - lngHour) / 24);
	float t = isRise ? N + ((6 - lngHour) / 24): N + ((18 - lngHour) / 24);

	// 3. calculate the Sun's mean anomaly (in rad)
	float M = 0.01720196510765f * t - 0.0574038790981f;

	// 4. calculate the Sun's true longitude
	//        [Note throughout the arguments of the trig functions
	//        (sin, tan) are in degrees. It will likely be necessary to
	//        convert to radians. eg sin(170.626 deg) =sin(170.626*pi/180
	//        radians)=0.16287]
	float L = M + (0.03344050846821f * sinf(M)) + (0.0003490658504f * sinf(M+M)) + 4.932893878082f;
	// NOTE: L potentially needs to be adjusted into the range [0,360] by adding/subtracting 360 ([0,2*pi] in rad)
	L = RangeF(L,0,TWO_PI);

	// 5a. calculate the Sun's right ascension
	float RA = atanf(0.91764f * tanf(L));
  	// NOTE: RA potentially needs to be adjusted into the range [0,360) by adding/subtracting 360
  	RA = RangeF(RA,0,TWO_PI);

  	// 5b. right ascension value needs to be in the same quadrant as L
	float Lquadrant  = floor( L/Half_PI) * Half_PI;
  	float RAquadrant = floor(RA/Half_PI) * Half_PI;
  	RA = RA + (Lquadrant - RAquadrant);

	// 5c. right ascension value needs to be converted into hours
  	RA = DEG * RA / 15;

	// 6. calculate the Sun's declination
	float sinDec = 0.39782 * sinf(L);
	float cosDec = cosf(asinf(sinDec));

	// 7a. calculate the Sun's local hour angle
	float cosH = (cos(zenith * RAD) - (sinDec * sin(gs.latitude * RAD))) / (cosDec * cos(gs.latitude * RAD));
	if (cosH >  1) {
		LOG(println, PSTR("the sun never rises on this location (on the specified date)"));
		// полярная ночь, солнца не будет. Чтобы не ломать логику солнце "всходит" в 0:00 и "заходит" в 0:01
		hour = 0;
		minute = isRise ? 0: 1;
		return;
	}
	if (cosH < -1) {
		LOG(println, PSTR("the sun never sets on this location (on the specified date)"));
		// полярный день, ночи не будет. Чтобы не ломать логику солнце "всходит" в 0:00 и "заходит" в 23:59
		hour = isRise ? 0: 23;
		minute = isRise ? 0: 59;
		return;
	}

	// 7b. finish calculating H and convert into hours
	// if if rising time is desired:
	// H = 360 - acos(cosH)
	// if setting time is desired:
	// H = acos(cosH)
	float H = isRise ? 360 - DEG * acos(cosH): DEG * acos(cosH);
	H = H / 15;

	// 8. calculate local mean time of rising/setting
	float T = H + RA - (0.06571f * t) - 6.622f;

	// 9. adjust back to UTC
	float UT = T - lngHour;
	// NOTE: UT potentially needs to be adjusted into the range [0,24) by adding/subtracting 24
	UT=RangeF(UT,0,24);

	// 10. convert UT value to local time zone of latitude/longitude
	float localT = UT + localOffset;

	hour = (int8_t)localT;
	minute = (int8_t)roundf(60.0f * fmodf(localT, 1.0f));
}

// пересчёт времени рассвета и заката при каждом успешном обновлении времени.
void DuskTillDawn() {
	tm t = getTime(nullptr);
	uint8_t hour = 0;
	uint8_t minute = 0;

	sunCalculate(t, true, hour, minute);
	sunrise = hour * 60 + minute;
	LOG(printf_P, PSTR("tz=%i, dst=%i\nSunrise: %i:%i\n"), gs.tz_shift, gs.tz_dst, hour, minute);

	sunCalculate(t, false, hour, minute);
	sunset = hour * 60 + minute;
	LOG(printf_P, PSTR("Sunset: %i:%i\n"), hour, minute);
}