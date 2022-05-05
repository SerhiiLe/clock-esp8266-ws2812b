#ifndef define_vars_h
#define define_vars_h

#include <FastLED.h>
#include "leds.h"
extern CRGB leds[];
extern uint8_t led_brightness; // текущая яркость

extern bool wifi_isConnected;
extern bool wifi_isPortal;
extern String wifi_message;
extern bool fl_demo;
extern time_t last_telegram;
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
extern timerMinim telegramTimer;		// Таймер периодичности опроса новых сообщений
extern timerMinim alarmStepTimer;		// Таймер увеличения громкости будильника

// управление плейером
extern int mp3_all;
extern int mp3_current;
extern int8_t cur_Volume;
extern bool mp3_isInit;

/*** определение глобальных перемененных, которые станут настройками ***/
// описания переменных в файле settings_init.h
extern String str_hello;
extern uint8_t max_alarm_time;
extern uint8_t run_allow;
extern uint16_t run_begin;
extern uint16_t run_end;
extern uint8_t wide_font;
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
extern uint16_t br_boost;
extern uint32_t max_power;
extern uint8_t turn_display;
extern uint8_t volume_start;
extern uint8_t volume_finish;
extern uint8_t volume_period;
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
	int8_t text = -1;	// номер текста, который выводится при срабатывании
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

//----------------------------------------------------
#if defined(LOG)
#undef LOG
#endif

#ifdef DEBUG
	//#define LOG                   Serial
	#define LOG(func, ...) Serial.func(__VA_ARGS__)
#else
	#define LOG(func, ...) ;
#endif

#if RELAY_TYPE == 1
	#define RELAY_OFF 0
	#define RELAY_OP(var) var
#else
	#define RELAY_OFF 1
	#define RELAY_OP(var) !var
#endif

#endif