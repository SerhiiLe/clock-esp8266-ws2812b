#ifndef defines_h
#define defines_h

#define DEBUG 0 // разрешение отладочных сообщений. Не используется

/*** описание констант, которые описывают конкретное "железо" ***/

#define PIN_LED 5 // LED матрица
#define PIN_PHOTO_SENSOR A0 // фоторезистор
#define PIN_BUTTON 12 // кнопка управления
#define PIN_MOTION 14 // детектор движения (кнопка)
#define SENSOR_BUTTON 1 // сенсорная кнопка - 1, обычная - 0
#define PIN_5V 4 // детектор наличия питания (5 Вольт)
#define PIN_RELAY 16 // реле выключатель питания матрицы
#define RELAY_OP_TIME 10 // время срабатывания реле по даташиту. ms
#define LED_MOTION 0 // светодиод индикатор движения
#define SRX 13 // software serial RX DFPlayer
#define STX 15 //  software serial TX DFPlayer

#define BRIGHTNESS 50		// стандартная максимальная яркость (0-255)
#define DEFAULT_POWER 2000	// по умолчанию 2000 мА или 2А
#define MAX_POWER 15000		// максимальный ток для матрицы в 256 светодиодов (32x8 или 16x16)

#define WIDTH 32             // ширина матрицы
#define HEIGHT 8             // высота матрицы
#define SEGMENTS 1           // диодов в одном "пикселе" (для создания матрицы из кусков ленты)
#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 3    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 1     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
#define NUM_LEDS WIDTH * HEIGHT * SEGMENTS
#define REFRESH_TIME NUM_LEDS * 35 // примерное время обновления матрицы. На esp8266 часто процесс вывода прерывается и матрица не обновляется, нужно повторить вывод

/*** часовой пояс и летнее время. Можно переопределить через Web настройки ***/

#define TIMEZONE 2 // временная зона по умолчанию
#define DSTSHIFT 0 // сдвиг летнего времени

/*** зарезервированное количество объектов в настройках. Занимают много места. ***/

#define MAX_ALARMS 9 // количество возможных будильников
#define MAX_RUNNING 6 // количество возможных бегущих строк

#include <FastLED.h>
#include "leds.h"
extern CRGB leds[];
extern uint8_t led_brightness; // текущая яркость

extern bool wifi_isConnected;
extern bool wifi_isPortal;
extern String wifi_message;
extern bool fl_demo;
extern time_t last_telegram;
extern bool fl_accelTelegram; 
#define TELEGRAM_TIMEOUT 1200 // время на которое запросы к телеграм идут чаще, в секундах
extern bool ftp_isAllow;
extern bool fl_5v;
extern bool fl_allowLEDS;
extern bool fl_timeNotSync;

// таймеры должны быть доступны в разных местах
#include "timerMinim.h"
extern timerMinim scrollTimer;          // таймер скроллинга
extern timerMinim autoBrightnessTimer;  // Таймер отслеживания показаний датчика света при включенной авторегулировки яркости матрицы
extern timerMinim saveSettingsTimer;    // Таймер отложенного сохранения настроек
extern timerMinim ntpSyncTimer;         // Таймер синхронизации времени с NTP-сервером
extern timerMinim scrollTimer;          // Таймер задержки между обновлениями бегущей строки, определяет скорость движения
extern timerMinim clockDate;            // Таймер периодичности вывода даты в виде бегущей строки (длительность примерно 15 секунд)
extern timerMinim textTimer[];          // Таймеры бегущих строк
extern timerMinim telegramTimer;

// управление плейером
extern int mp3_all;
extern int mp3_current;
extern int8_t cur_Volume;
extern bool mp3_isInit;

/*** определение глобальных перемененных, которые станут настройками. И определять их надо будет 5 раз... С-ка... ***/
// описания переменных в файле settings_init.h
extern String str_hello;
extern uint8_t max_alarm_time;
extern uint8_t run_allow;
extern uint16_t run_begin;
extern uint16_t run_end;
extern uint8_t show_move;
extern uint8_t delay_move;
extern int8_t tz_shift;
extern int8_t tz_dst;
extern uint8_t show_date_short;
extern uint16_t show_date_period;
extern uint8_t show_time_color;
extern uint32_t show_time_color0;
extern uint32_t show_time_col[];
extern uint8_t show_date_color;
extern uint32_t show_date_color0;
extern uint8_t bright_mode;
extern uint8_t bright0;
extern uint8_t br_boost;
extern uint32_t max_power;
extern uint8_t turn_display;
extern uint16_t sync_time_period;
extern uint16_t scroll_period;
extern String tb_name;
extern String tb_chats;
extern String tb_secret;
extern String tb_token;
extern uint16_t tb_rate;
extern String web_login;
extern String web_password;

struct cur_alarm {
	uint16_t settings = 0;	// настройки (побитовое поле)
	uint8_t hour = 0;	// часы
	uint8_t minute = 0;	// минуты
	uint16_t melody = 0;	// номер мелодии
};
extern cur_alarm alarms[];

struct cur_text {
	String text = "";	// текст который надо отобразить
	uint8_t color_mode = 0; // режим цвета, как везде (0 )
	uint32_t color = 0xFFFFFF; // по умолчанию - белый
	uint16_t period = 60; // период повтора в секундах
	uint16_t repeat_mode = 0; // режим повтора (0 пока активно, 1 до конца дня, 2 день недели, 3 день месяца)
};
extern cur_text texts[];

extern uint8_t sec_enable;
extern uint8_t sec_curFile;

#endif