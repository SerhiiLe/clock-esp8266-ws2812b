#ifndef timerMinim_h
#define timerMinim_h

#include <Arduino.h>

/*
	Таймер для переодически повторяющихся операций.

	Отсчёт каждого цикла начинается с момента опроса готовности таймера, если он к этому времени сработал.
	Это приводит к тому, что периоды не равномерны, время каждого цикла не может быть меньше заданного,
	но может быть намного больше заданного. Таким образом таймер не отсчитывает равные промежутки,
	а скорее гарантирует пропуск заданного времени для приписанной к таймеру операции.

	В фоне ничего не считается, ресурсов не потребляет, все расчёты только в момент вызова isReady()

	Для разовых ожиданий проще использовать millis() напрямую.
*/

class timerMinim
{
	public:
		// объявление таймера с указанием интервала
		timerMinim(uint32_t interval=60000) {
			setInterval(interval);
			reset();
		}
		// установка интервала работы таймера
		void setInterval(uint32_t interval) {
			// хотя-бы одна миллисекунда, для приличия
			_interval = interval == 0 ? 1: interval;
		}
		// возвращает true, когда пришло время.
		boolean isReady() {
			unsigned long time = millis();
			if(_overflow) { // попытка защититься от переполнения
				if(time < _time) // ждём переполнения, которое наступает каждые 49 дней
					_overflow = false;
				else
					return false;
			}
			if(time >= _next) {
				reset();
				return true;
			} else {
				return false;
			}
		}
		// ручной сброс таймера на установленный интервал
		void reset() {
			_time = millis();
			_next = _time + _interval;
			_overflow = _time > _next; 
		}
		// выставить время следующего срабатывания
		void setNext(uint32_t next = 0) {
			_time = millis();
			_next = _time + next;
			_overflow = _time > _next; 
		}

	private:
		uint32_t _interval = 0;
		unsigned long _time = 0;
		unsigned long _next = 0;
		bool _overflow = false;
};

#endif