/*
	Установка времени через NTP
*/

#include <Arduino.h>
#include <time.h>
#include "defines.h"
#include "settings.h"

#define REQUEST_TIMEOUT 10000 // таймаут запроса к NTP серверу.

time_t start_time = 0;
bool fl_needStartTime = true;
bool fl_timeNotSync = true;
bool fl_ntpRequestIsSend = false;
unsigned long request_time = 0;

void DuskTillDawn();

void syncTimeRequest() {
	int tz           = gs.tz_shift;
	int dst          = gs.tz_dst;
	configTime(tz * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
	LOG(println, PSTR("Request for NTP time sync is send"));
	fl_ntpRequestIsSend = true;
	request_time = millis();
}

bool syncTime() {
	time_t now = time(nullptr);
	if( ! fl_ntpRequestIsSend ) {
		syncTimeRequest();
		return false;
	}
	// Сутки от 1го января 1970. Если системное время больше, значит или прошли сутки, или как-то время установилось, например вручную.
	if( now < 86400 ) {
		if((millis() - request_time) > REQUEST_TIMEOUT) {
			// увы, ответ на запрос таки не пришел и время не установилось. Повезёт в следующий раз. 
			LOG(println, PSTR("\n[ERROR] Failed to get NTP time."));
			fl_ntpRequestIsSend = false;
			return false;
		}
		return false;
	}
	// время установлено
	if(fl_needStartTime) {
		// это первое обновление времени после включения, вычисление времени запуска с учётом времени на получение времени и запись в лог
		start_time = now - millis()/1000;
		fl_needStartTime = false;
		if(sec_enable) save_log_file(SEC_TEXT_BOOT);
		last_telegram = now;
	}
	DuskTillDawn();
	fl_timeNotSync = false;
	fl_ntpRequestIsSend = false;
	LOG(println, PSTR("time is synced"));
	return true;
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
	switch (gs.boost_mode)	{
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