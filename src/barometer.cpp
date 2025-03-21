/*
	Опрос барометра/термометра BMP
	это самый дешёвый модуль BMP180 (BMP085 почти тоже, но без линейного стабилизатора 3.3V)

	Имеет неприятное свойство саморазогревания, из-за чего завышает температуру.
*/

#include <Arduino.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <Adafruit_AHTX0.h>
#include "defines.h"
#include "forecaster.h"
#include "webClient.h"

Adafruit_BMP085 bmp0;
Adafruit_BMP280 bmp2;
Adafruit_BME280 bme;
Adafruit_AHTX0 aht;

uint8_t fl_barometerIsInit = 0; // флаг наличия барометра 0 - нет, 1 - BMP180, 2 - BMP280, 4 - BME180, 8 - AHTX0
float Temperature = 0.0f; // температура последнего опроса
int32_t Pressure = 0; // давление последнего опроса
float Humidity = 0.0f; // влажность последнего опроса
unsigned long lastTempTime = 0; // время последнего опроса
uint8_t address_bme280 = 0x76; // адрес датчика BME280

bool barometer_init() {
	if( bmp0.begin(BMP085_STANDARD) ) {
		fl_barometerIsInit = 1;
		LOG(println, PSTR("BMP180 found"));
	} else 
	if( bmp2.begin(address_bme280) ) {
		fl_barometerIsInit = 2;
		uint32_t type = bmp2.sensorID();
		LOG(printf_P, PSTR("BMP280 found, type: 0x%02X\n"), type);
	} else
	if( bme.begin(address_bme280) ) {
		fl_barometerIsInit = 4;
		uint32_t type = bme.sensorID();
		LOG(printf_P, PSTR("BME280 found, type: 0x%02X\n"), type);
		return true;
	}
	if( aht.begin() ) {
		fl_barometerIsInit |= 8;
		LOG(println, PSTR("AHTX0 found"));
	}
	return fl_barometerIsInit ? true: false;
}

int32_t getPressure(bool fl_cor) {
	if(!fl_barometerIsInit) return 0;
	int32_t p = 0;
	if( fl_barometerIsInit & 1 ) {
		p = bmp0.readPressure();
	} else if( fl_barometerIsInit & 2 ) {
		p = bmp2.readPressure();
	} else if( fl_barometerIsInit & 4 ) {
		p = bme.readPressure();
	}
	return p + (fl_cor ? ws.bar_cor * 100: 0);
}

float getTemperature(bool fl_cor) {
	if(!fl_barometerIsInit) return -100.0f;
	float t = 0.0f;
	if( fl_barometerIsInit & 1 ) {
		t = bmp0.readTemperature();
	} else if( fl_barometerIsInit & 2 ) {
		t = bmp2.readTemperature();
	} else if( fl_barometerIsInit & 8 ) {
		sensors_event_t humidity, temp;
		aht.getEvent(&humidity, &temp);
		t = temp.temperature;
	}
	return t + (fl_cor ? ws.term_cor: 0);
}

float getHumidity() {
	if(!fl_barometerIsInit) return 0.0f;
	float h = 0.0f;
	if( fl_barometerIsInit & 4 ) {
		h = bme.readHumidity();
	} else if( fl_barometerIsInit & 8 ) {
		sensors_event_t humidity, temp;
		aht.getEvent(&humidity, &temp);
		h = humidity.relative_humidity;
	}
	return h;
}

const char* currentPressureTemp (char *a, bool fl_tiny) {
	char ft[100] = {0};
	if(ws.forecast) {
		int16_t trend = forecaster_getTrend();
		int8_t cast = forecaster_getCast();
		if(fl_tiny)
			sprintf_P(ft, PSTR("\n%+i %i"), trend, cast);
		else
			sprintf_P(ft, PSTR(" trend:%+i cast:%i"), trend, cast);
	}

	// если есть аппаратные датчики, то вывести их показания
	if(fl_barometerIsInit) {
		if(millis() - lastTempTime > 1000ul * ws.term_pool || lastTempTime == 0) {
			Temperature = getTemperature(false);
			Pressure = getPressure(false)/100;
			Humidity = getHumidity();
			lastTempTime = millis();
		}
		float t = Temperature + ws.term_cor;
		int32_t p = Pressure + ws.bar_cor;
		if( fl_barometerIsInit == 8 ) p = weatherGetPressure()/100;
		float h = Humidity;

		// есть датчик влажности (BME280 или AHTX0), вывести её
		char ht[20] = {0};
		if( fl_barometerIsInit & 12 ) {
			if(fl_tiny)
				sprintf_P(ht, PSTR("\n%5.1f%% h"), h);
			else
				sprintf_P(ht, PSTR(" %1.1f%%h"), h);
		}
		// есть датчик давления (BMP180,BMP280,BME280), вывести его
		char pt[20] = {0};
		if( fl_barometerIsInit & 7 ) {
			if(fl_tiny)
				sprintf_P(pt, PSTR("\n%4i hPa"), p);
			else
				sprintf_P(pt, PSTR(" %i hPa"), p);
		}
		if(fl_tiny)
			sprintf_P(a, PSTR(" %+1.1f\xc2\xb0\x43%s%s%s"), t, ht, pt, ft);
		else
			sprintf_P(a, PSTR("%+1.1f\xc2\xb0\x43%s%s%s"), t, ht, pt, ft);
		return a;

	}
	else if(ws.forecast) {
		// если нет аппаратных датчиков, то вывести показания с сервера
		if(fl_tiny)
			sprintf_P(a, PSTR("%4i hPa%s"), weatherGetPressure(), ft);
		else
			sprintf_P(a, PSTR("%i hPa %s"), weatherGetPressure(), ft);
	}
	sprintf_P(a, PSTR("unknown"));
	return a;
}
