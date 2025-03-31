#ifndef defines_h
#define defines_h

#define DEBUG // разрешение отладочных сообщений. Закомментировать, если не надо

/*** описание констант, которые описывают конкретное "железо" ***/

// ВНИМАНИЕ! следующая опция переключает раскладку PIO cо старой на новую, с датчиками i2c.
// Будьте внимательны! Проверьте раскладку назначений PIO перед подключением модулей!
#define USE_I2C 1 // использовать I2C для датчики. 1 - схема с i2c, 0 - схема без i2c.

// Назначение PIO для разных типов микроконтроллеров. Сверяйтесь с pinout вашего модуля.
#if ESP32C3 == 1 // ESP32-c3
	#define PIN_LED 6 // LED матрица
	#define PIN_PHOTO_SENSOR A0 // фоторезистор
	#define PIN_BUTTON 1 // кнопка управления
	#define PIN_MOTION 10 // детектор движения
	// #define PIN_5V 3 // детектор наличия питания (5 Вольт). Закомментировать, если не подключен
	#define PIN_RELAY 7 // реле выключатель питания матрицы
	#define LED_MOTION 5 // светодиод индикатор движения. Закомментировать для вывода на матрицу.
	#define SRX 20 // hardware serial RX DFPlayer. Закомментировать, если не подключен
	#define STX 21 // hardware serial TX DFPlayer
	// -> SDA 8 // I2C SDA Для справки, определены в Arduino
	// -> SCL 9 // I2C SCL
	#define BAT_LEVEL 4 // напряжение аккумулятора
#elif ESP32 == 1 // ESP32
	#define PIN_PHOTO_SENSOR 36 // "VP" - фоторезистор
	#define PIN_BUTTON 19 // кнопка управления
	#define PIN_MOTION 18 // детектор движения
	#define PIN_RELAY 26 // реле выключатель питания матрицы
	#if USE_I2C == 0
		// ------8<------ old, compatible with ESP8266 Wemos D1 mini
		#define PIN_LED 22 // LED матрица
		#define LED_MOTION 17 // светодиод индикатор движения. Закомментировать для вывода на матрицу.
		#define PIN_5V 21 // детектор наличия питания (5 Вольт). Закомментировать, если не подключен
		#define SRX 23 // software serial RX DFPlayer. Закомментировать, если не подключен
		#define STX 5 // software serial TX DFPlayer
		// ------8<------
	#else
		// ------8<------ new, for esp32
		#define PIN_LED 23 // LED матрица
		#define LED_MOTION 4 // светодиод индикатор движения. Закомментировать для вывода на матрицу.
		#define PIN_5V 5 // детектор наличия питания (5 Вольт). Закомментировать, если не подключен
		#define SRX 16 // hardware serial RX DFPlayer. Закомментировать, если не подключен
		#define STX 17 // hardware serial TX DFPlayer
		// -> SDA 21 // I2C SDA Для справки, определены в Arduino
		// -> SCL 22 // I2C SCL
		#define BAT_LEVEL 35 // напряжение аккумулятора
		// ------8<------
	#endif
#else // ESP8266
	#define PIN_PHOTO_SENSOR A0 // фоторезистор
	#define PIN_BUTTON 12 // кнопка управления
	#define PIN_MOTION 14 // детектор движения
	#define SRX 13 // software serial RX DFPlayer. Закомментировать, если не подключен
	#define STX 15 // software serial TX DFPlayer
	#if USE_I2C == 0
		// ------8<------ old, без i2c, начальная схема
		#define PIN_LED 5 // LED матрица
		#define PIN_5V 4 // детектор наличия питания (5 Вольт). Закомментировать, если не подключен
		#define LED_MOTION 0 // светодиод индикатор движения. Закомментировать для вывода на матрицу.
		#define PIN_RELAY 16 // реле выключатель питания матрицы
		// ------8<------
	#else
		// ------8<------ new, с i2c, альтернативная схема, не рекомендую для esp8266
		#define PIN_LED 2 // LED матрица
		#define PIN_5V 16 // детектор наличия питания (5 Вольт). Закомментировать, если не подключен
		#define PIN_RELAY 0 // реле выключатель питания матрицы
		// -> SDA 4 // I2C SDA Для справки, определены в Arduino
		// -> SCL 5 // I2C SCL
		// LED_MOTION не поместился, пины закончились, будет отображаться на матрице
		// ------8<------
	#endif
#endif

#define SENSOR_BUTTON 1 // сенсорная кнопка - 1, обычная - 0

#define RELAY_OP_TIME 10 // время срабатывания реле по спецификации. ms
#define RELAY_TYPE 1 // тип реле, срабатывает по: 0 - низкому, 1 - высокому уровню.

// если схема с i2c.
#define USE_NVRAM 0 // использовать отдельный чип на плате RTC, вместо flash esp8266/esp32. 0 - файлы, 1 - чип NVRAM
#define USE_RTC 1	// использовать аппаратный чип RTC (часы). 0 - только интернет, 1 - использовать
#define USE_BMP 1	// использовать датчик давления/температуры (BMP180). 0 - не использовать, 1 - использовать

/*** ограничение потребления матрицей ***/

#define BRIGHTNESS 50		// стандартная максимальная яркость (0-255)
#define DEFAULT_POWER 2000	// по умолчанию 2000 мА или 2А
#define MAX_POWER 15000		// максимальный ток для матрицы в 256 светодиодов (32x8 или 16x16)

/*** описание матрицы ***/

#define WIDTH 32			// ширина матрицы
#define HEIGHT 8			// высота матрицы
#define SEGMENTS 1			// диодов в одном "пикселе" (для создания матрицы из кусков ленты)
#define MATRIX_TYPE 0		// тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 3	// угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 1	// направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
#define NUM_LEDS WIDTH * HEIGHT * SEGMENTS
#define REFRESH_TIME NUM_LEDS * 35 // примерное время обновления матрицы. На esp8266 часто процесс вывода прерывается и матрица не обновляется, нужно повторить вывод
#define CLOCK_SHIFT 2 		// сдвиг первой цифры при выводе часов. XX:YY = 5 символов + 4 пробела = 5*5+4 = 29. 32-29 = 3 или 2 светодиода перед и 1 после
#define TEXT_BASELINE 0		// высота, на которой выводится текст (от низа матрицы)

/*** часовой пояс и летнее время. Можно переопределить через Web настройки ***/

#define TIMEZONE 2 // временная зона по умолчанию
#define DSTSHIFT 0 // сдвиг летнего времени

/*** зарезервированное количество объектов в настройках. Занимают много места. ***/

#define MAX_ALARMS 9	// количество возможных будильников
#define MAX_RUNNING 9	// количество возможных бегущих строк (больше 9 может вызвать проблемы с памятью)
#define MAX_SENSORS 10	// количество слотов для регистрации удалённых сенсоров

/*** ускорение опросов телеграм-бота или временная приостановка запросов ***/

#define TELEGRAM_ACCELERATE 900 // время на которое запросы к телеграм идут чаще, в секундах
#define TELEGRAM_ACCELERATED 10 // периодичность ускоренного опроса телеграм, в секундах 
#define TELEGRAM_BAN 1800 // время на которое прекращаются запросы к телеграм после ошибки, в секундах
#define TELEGRAM_MAX_LENGTH 2000 // максимальный размер сообщений отсылаемых в Телеграм

/*** разное ***/

// #define USE_HTTPS // Использовать https вместо http в запросах к серверу погоды. Крайне не рекомендую, очень медленно.

/*** дальше определение переменных, ничего не менять ***/

#include "define_vars.h"

#endif
