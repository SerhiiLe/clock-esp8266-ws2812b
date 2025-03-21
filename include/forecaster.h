/*
    К библиотеке добавлены функции сохранения и восстановления данных.
    Это нужно, чтобы не ждать 3 часа для получения первых показателей после каждой перезагрузки.
*/
/*
    Библиотека для определения прогноза погоды по давлению для Arduino
    Документация:
    GitHub: https://github.com/GyverLibs/Forecaster
    Возможности:
    - Определение краткосрочного прогноза погоды по алгоритму Замбретти
    - Принимает давление, температуру, высоту над ур. моря и месяц года
    - Определение тренда давления при помощи линеаризации
    
    AlexGyver, alex@alexgyver.ru
    https://alexgyver.ru/
    MIT License
    
    Основано на
    https://integritext.net/DrKFS/zambretti.htm
    https://github.com/sassoftware/iot-zambretti-weather-forcasting/blob/master/README.md
*/

#ifndef _Forecaster_h
#define _Forecaster_h
#include <Arduino.h>
#include "ntp.h"

#define _FC_SIZE 6  // размер буфера. Усреднение за 3 часа, при размере 6 - каждые 30 минут

void forecaster_restore_data();
void forecaster_update_data();
void forecaster_setH(int h);
void forecaster_addP(uint32_t P, float t);
void forecaster_addPmm(float P, float t);
void forecaster_setMonth(uint8_t month);
int8_t forecaster_getCast();
int forecaster_getTrend();

#endif