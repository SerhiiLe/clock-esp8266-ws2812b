#ifndef timerMinim_h
#define timerMinim_h

#include <Arduino.h>

class timerMinim
{
	public:
		timerMinim(uint32_t interval=60000);	// объявление таймера с указанием интервала
		void setInterval(uint32_t interval);	// установка интервала работы таймера
		boolean isReady();					   	// возвращает true, когда пришло время. Сбрасывается в false сам (AUTO) или вручную (MANUAL)
		void reset();						   	// ручной сброс таймера на установленный интервал

	private:
		unsigned long _timer = 0;
		uint32_t _interval = 0;
};

#endif