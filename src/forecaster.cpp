/*
	Автономный прогноз погоды
*/

#include "forecaster.h"
#include "defines.h"
#include "barometer.h"
#include "rtc.h"
#include "webClient.h"

/*
https://github.com/GyverLibs/Forecaster
*/

#define _FC_TIME_DELTA 10800/_FC_SIZE-30 // те же 3 часа, поделенные на размер буфера, получаем 30 минут

struct allForecasterData {
	time_t last = 0; // время добавления последних показаний
	int32_t Parr[_FC_SIZE]; // массив показаний за последние 3 часа
	float H = 0; // высота над уровнем моря
	bool start = true; // флаг необходимости инициализации буфера показаний давления
	int8_t cast = 0; // значение последнего предсказание погоды
	int delta = 0; // изменение давления
	uint8_t season = 0; // время года
} afd;

// обновление данных для предсказателя погоды, должно вызываться каждые пол часа.
void forecaster_update_data() {
	tm t = getTime();
	forecaster_setMonth(t.tm_mon+1);

	if(fl_barometerIsInit) // если есть аппаратный датчик, то брать с него
		forecaster_addP(getPressure(), getTemperature());
	else if(ws.weather) // если нет аппаратного, но настроен интернет, брать с него
		forecaster_addP(weatherGetPressure(), weatherGetTemperature());
	else return; // или ничего не делать...

	uint8_t csum = rtcGetByte(0);
	LOG(printf_P, PSTR("read csum: %u\n"), csum);

	// сохранить дамп данных на случай перезагрузки
	// uint8_t 
	csum = rtcWriteBlock(1, (uint8_t*)&afd, sizeof(allForecasterData));
	rtcSetByte(0, csum);
	LOG(println, PSTR("forecaster data update"));
	LOG(printf_P, PSTR("write csum: %u\n"), csum);
}

void forecaster_restore_data() {
	allForecasterData buf;
	uint8_t csum_real = rtcReadBlock(1, (uint8_t*)&buf, sizeof(allForecasterData));
	uint8_t csum = rtcGetByte(0);
	LOG(printf_P, PSTR("start read csum: %u, real %u\n"), csum, csum_real);
	uint32_t next = 0;
	if(csum_real == csum) {	// в памяти данные похожие на корректные
		// если последнее обновление меньше, чем 30 минут назад, то вычислить время следующего обновления
		int time_diff = getTimeU() - buf.last;
		if(time_diff < 1800) next = time_diff;
		// если больше 3 часа, то провести обычную процедуру старта
		if(time_diff > 10800) buf.start = true;
		// восстановить данные
		memcpy(&afd, &buf, sizeof(allForecasterData));
		LOG(println, PSTR("forecaster data restored"));
	} else {
		forecaster_setH(ws.altitude);
		LOG(println, PSTR("new forecaster data"));
	}
	// датчик BMP180 выходит на рабочий режим не сразу
	if(next < 120) next=120;
	forecasterTimer.setNext(next * 1000U);
	LOG(printf_P, PSTR("next forecaster update in %i sec\n"), next);
}

// установить высоту над уровнем моря (в метрах)
void forecaster_setH(int h) {
	afd.H = h * 0.0065f;
}

// добавить текущее давление в Па и температуру в С (КАЖДЫЕ 30 МИНУТ)
// здесь же происходит расчёт прогноза
void forecaster_addP(uint32_t P, float t) {
	time_t now = getTimeU();
	if(now-afd.last<_FC_TIME_DELTA) return;
	afd.last = now;
	P = (float)P * pow(1 - afd.H / (t + afd.H + 273.15), -5.257);   // над уровнем моря

	if( afd.start ) {
		afd.start = false;
		for (uint8_t i = 0; i < _FC_SIZE; i++) afd.Parr[i] = P;
	} else {
		for (uint8_t i = 0; i < (_FC_SIZE-1); i++) afd.Parr[i] = afd.Parr[i + 1];
		afd.Parr[_FC_SIZE - 1] = P;
	}
	
	// расчёт изменения по наименьшим квадратам
	long sumX = 0, sumY = 0, sumX2 = 0, sumXY = 0;        
	for (int i = 0; i < _FC_SIZE; i++) {
		sumX += i;
		sumY += afd.Parr[i];
		sumX2 += i * i;
		sumXY += afd.Parr[i] * i;
	}
	float a = _FC_SIZE * sumXY - sumX * sumY;
	a /= _FC_SIZE * sumX2 - sumX * sumX;
	afd.delta = a * (_FC_SIZE - 1);
	
	// расчёт прогноза по Zambretti
	P /= 100;   // -> ГПа
	// if (P < 945) P = 945;
	// if (P > 1030) P = 1030;
	float cast = 0.0f; 
	if (afd.delta > 150) { // Rising 	Between 947 mbar and 1030 mbar 	Rise of 1.6 mbar in 3 hours
		if( P < 947 ) P = 947;
		if( P > 1030 ) P = 1030;
		cast = 161 - 0.155 * P - afd.season;	// rising (160)
	} else if (afd.delta < -150) { // Falling 	Between 985 mbar and 1050 mbar 	Drop of 1.6 mbar in 3 hours
		if( P < 985 ) P = 985;
		if( P > 1050 ) P = 1050;
		cast = 131 - 0.124 * P + afd.season;	// falling (130)
	} else { // Steady 	Between 960 mbar and 1033 mbar 	No drop or rise of 1.6 mbar in 3 hours
		if( P < 960 ) P = 960;
		if( P > 1033 ) P = 1033;
		cast = 139 - 0.133 * P;					// steady (138)
	}
	// if (afd.cast < 0) afd.cast = 0;
	afd.cast = (int8_t)std::round(cast);        
}

// добавить текущее давление в мм.рт.ст и температуру в С (КАЖДЫЕ 30 МИНУТ)
void forecaster_addPmm(float P, float t) {
	forecaster_addP(P * 133.322f, t);
}

// установить месяц (1-12)
// 0 чтобы отключить сезонность
void forecaster_setMonth(uint8_t month) {
	if (month == 0) afd.season = 0;
	else afd.season = (month >= 4 && month <= 9) ? 2 : 1;
	/*
	if( month == 12 ) month = 0;
	month /= 3;                         		// 0 зима, 1 весна, 2 лето, 3 осень
	afd.season = month * 0.5 + 1;           	// 1, 1.5, 2, 2.5
	if( afd.season == 2.5) afd.season = 1.5;    // 1, 1.5, 2, 1.5
	*/
}

// получить прогноз (0 хорошая погода... 10 ливень-шторм)
int8_t forecaster_getCast() {
	return afd.cast;
}

// получить изменение давления в Па за 3 часа
int forecaster_getTrend() {
	return afd.delta;
}
