#ifndef timerMinim_h
#define timerMinim_h

#include <Arduino.h>

class timerMinim
{
	public:
		// объявление таймера с указанием интервала
		timerMinim(uint32_t interval=60000) {
			_interval = interval;
			_timer = millis();
		}
		// установка интервала работы таймера
		void setInterval(uint32_t interval) {
			_interval = interval;
		}
		// возвращает true, когда пришло время.
		boolean isReady() {
			if(millis() - _timer >= _interval || is_ready) {
				_timer = millis();
				is_ready = false;
				return true;
			} else {
				return false;
			}
		}
		// ручной сброс таймера на установленный интервал
		void reset() {
			_timer = millis();
		}
		// ручное взведение таймера на сработку
		void setReady() {
			is_ready = true;
		}

	private:
		unsigned long _timer = 0;
		uint32_t _interval = 0;
		bool is_ready = false;
};

#endif