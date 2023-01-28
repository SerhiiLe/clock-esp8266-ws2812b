/**
 * @file main.cpp
 * @author Serhii Lebedenko (slebedenko@gmail.com)
 * @brief 
 * @version 1.5.0
 * @date 2023-01-28
 * 
 * @copyright Copyright (c) 2021,2022,2023
 * 
 */

/*
	Главный файл проекта, в нём инициализация и логика основного цикла.
	"мультизадачность" организована как "кооперативная", через таймеры на millis
*/

#include <Arduino.h>
#include "defines.h"
#include <GyverButton.h>
#include <LittleFS.h>
#include <ftp.h>

#include "settings.h"
#include "clock.h"
#include "runningText.h"
#include "wifi_init.h"
#include "ntp.h"
#include "web.h"
#include "dfplayer.h"
#include "demo.h"
#include "security.h"

#if SENSOR_BUTTON == 1
GButton btn(PIN_BUTTON, LOW_PULL, NORM_OPEN); // комбинация для сенсорной кнопки
#else
GButton btn(PIN_BUTTON); // комбинация для обычной кнопки
#endif

timerMinim autoBrightnessTimer(500);	// Таймер отслеживания показаний датчика света при включенном авторегулировании яркости матрицы
timerMinim clockTimer(512);				// Таймер, чтобы разделитель часов и минут мигал примерно каждую секунду
timerMinim scrollTimer(scroll_period);	// Таймер обновления бегущей строки
timerMinim ntpSyncTimer(3600000U * sync_time_period);  // Таймер синхронизации времени с NTP-сервером 3600000U
timerMinim clockDate(1000U * show_date_period); // периодичность вывода даты в секундах
timerMinim textTimer[MAX_RUNNING];
timerMinim alarmTimer(1000);			// для будильника, срабатывает каждую секунду
timerMinim alarmStepTimer(5000);		// шаг увеличения громкости будильника
timerMinim demoTimer(33);				// таймер для теста/демонстрации экрана
timerMinim telegramTimer(1000U * tb_accelerated);	// период опроса команд из Телеграм
timerMinim timeoutMp3Timer(3600000U * timeout_mp3); // таймер принудительного сброса mp3

// файловая система подключена
bool fs_isStarted = false;
// время начала работы будильника
time_t alarmStartTime = 0;
// яркость прошлого цикла
int16_t old_brightness = 5000;
// состояние датчика движения
bool cur_motion = false;
// время последней сработки датчика движения
unsigned long last_move = 0;
// время последнего отключения экрана
unsigned long last_screen_no5V = 0;
// флаг отработки действия датчика движения
bool fl_action_move = true;
// разрешение выводить бегущую строку (уведомления, день недели и число)
bool fl_run_allow = true;
// буфер под вывод даты / времени (в юникоде 1 буква = 2 байта)
char timeString[100];
// работает тест/демо режим
bool fl_demo = false;
// флаг требования сброса пароля
bool fl_password_reset_req = false;
// флаг наличия питания
bool fl_5v = true;
// Разрешение отображения на матрице
bool fl_allowLEDS = true;
// Текущая мелодия будильника, которая должна играть
uint8_t active_alarm = 0;
// разрешение увеличения яркости
bool fl_bright_boost = false;
// старое значение fl_bright_boost
bool old_bright_boost = true;

void setup() {
	Serial.begin(115200);
	Serial.println(PSTR("Starting..."));
	pinMode(LED_MOTION, OUTPUT);
	pinMode(PIN_MOTION, INPUT);
#ifdef PIN_5V
	pinMode(PIN_5V, INPUT);
#endif
	pinMode(PIN_RELAY, OUTPUT);
	digitalWrite(PIN_RELAY, RELAY_OFF);
	delay(RELAY_OP_TIME); // задержка на время срабатывания (выключения) рэле. А вообще должно было стоять рэле LOW и тогда после старта оно бы сразу было выключено.
	display_setup();
	randomSeed(analogRead(PIN_PHOTO_SENSOR)+analogRead(PIN_PHOTO_SENSOR));
	screenIsFree = true;
	// initRString(PSTR("..."),1,8);
	initRString(PSTR("boot"),1,7); //5
	display_tick();
	if( LittleFS.begin()) {
		fs_isStarted = true; // встроенный диск подключился
		LOG(println, PSTR("LittleFS mounted"));
	} else {
		LOG(println, PSTR("ERROR LittleFS mount"));
		initRString(PSTR("Ошибка подключения встроенного диска!!!"));
	}
	if(!load_config_main()) {
		LOG(println, PSTR("Create new config file"));
		//  Создаем файл запив в него данные по умолчанию, при любой ошибке чтения
		save_config_main();
	}
	if(!load_config_alarms()) {
		LOG(println, PSTR("Create new alarms file"));
		save_config_alarms(); // Создаем файл
	}
	if(!load_config_texts()) {
		LOG(println, PSTR("Create new texts file"));
		save_config_texts(); // Создаем файл
	}
	if(!load_config_security()) {
		LOG(println, PSTR("Create new security file"));
		save_config_security();	// Создаем файл
	}
	if(!load_config_telegram()) {
		LOG(println, PSTR("Create new telegram file"));
		save_config_telegram();	// Создаем файл
	}
	initRString(str_hello);
	wifi_setup();
	init_telegram();
}

// выключение всех активных в данный момент будильников
void alarmsStop() {
	// для начала надо остановить проигрывание мелодии и сбросить таймер активности
	alarmStartTime = 0;
	delay(10);
	// mp3_disableLoop();
	// delay(10);
	mp3_stop();
	// устанавливается флаг, что все активные сейчас будильники отработали
	for(uint8_t i=0; i<MAX_ALARMS; i++)
		if(alarms[i].settings & 1024)
			alarms[i].settings |= 2048;
}

void loop() {
	int16_t i = 0;
	bool fl_doit = false;
	bool fl_save = false;
	tm t;

	wifi_process();
	if( wifi_isConnected ) {
		// установка времени по ntp.
		if( fl_timeNotSync )
			// первичная установка времени. Если по каким-то причинам опрос не удался, повторять не чаще, чем раз в секунду.
			if( alarmStepTimer.isReady() ) syncTime();
		if(ntpSyncTimer.isReady()) // это плановая синхронизация, не критично, если опрос не прошел
			syncTime();
		// запуск сервисов, которые должны запуститься после запуска сети. (сеть должна подниматься в фоне)
		ftp_process();
		web_process();
		if(telegramTimer.isReady()) tb_tick();
	}

	if( mp3_isInit ) mp3_check();
	btn.tick();

	if(btn.isHolded()) {
		LOG(println, PSTR("holded"));

		if(wifi_isPortal) {
			initRString(wifi_message,CRGB::White);
		} else if(!wifi_isConnected) {
			initRString(PSTR("WiFi не найден, для настройки - 1 клик"),CRGB::White);
		} else {
			initRString(PSTR("Справка: 1 клик-дата, 2-Демо, 3-IP, 4-Яркость, 5-Сброс пароля или WiFi."));
		}
	}
	if(btn.hasClicks()) // проверка на наличие нажатий
	switch (btn.getClicks()) {
		case 1:
			LOG(println, PSTR("Single"));
			if(wifi_isPortal) wifi_startConfig(false);
			else
			if(!wifi_isConnected)
				wifi_startConfig(true);
			else {
				if(fl_demo) fl_demo = false;
				else {
					initRString(dateCurrentTextLong(timeString), show_date_color > 0 ? show_date_color: show_date_color0);
					clockDate.reset();
				}
			}
			if(alarmStartTime) alarmsStop(); // остановить будильник если не сработал датчик движения, но нажали на кнопку
			fl_password_reset_req = false;
			break;
		case 2:
			LOG(println, PSTR("Double"));
			if(fl_5v) { // при низком напряжении сенсорная кнопка может давать ложные срабатывания, по этому отключить все функции
				if(fl_password_reset_req) {
					web_password = "";
					initRString(PSTR("Пароль временно отключен. Зайдите в настройки и задайте новый!"),CRGB::OrangeRed);
				} else
					fl_demo = !fl_demo;
			}
			break;
		case 3:
			LOG(println, PSTR("Triple"));
			if(fl_5v && fl_password_reset_req) {
				if(!wifi_isPortal) wifi_startConfig(true);
			} else
				initRString("IP: "+wifi_currentIP());
			break;
		case 4:
			LOG(println, PSTR("Quadruple"));
			if(fl_5v) {
				char buf[20];
				sprintf_P(buf,PSTR("%i -> %i -> %i"),analogRead(PIN_PHOTO_SENSOR), old_brightness*bright_boost/100, led_brightness);
				initRString(buf);
			}
			break;
		case 5:
			LOG(println, PSTR("Fivefold"));
			if(fl_5v) {
				initRString(PSTR("Сброс пароля! Для подтверждения - 2 клика! Или 3 для сброса WiFi. 1 клик - отмена."),CRGB::OrangeRed);
				fl_password_reset_req = true;
			}
			break;
	}

	// проверка наличия напряжения 5 Вольт
#ifdef PIN_5V
	if(digitalRead(PIN_5V) != fl_5v) {
		fl_5v = ! fl_5v;
		if(sec_enable) save_log_file(fl_5v ? SEC_TEXT_POWERED: SEC_TEXT_POWEROFF);
		fl_allowLEDS = fl_5v;
		// если нет питания - отключить питание матрицы и снизить яркость на минимум
		if(!fl_5v) {
			set_brightness(1);
			old_brightness = 1;
		} else {
			digitalWrite(PIN_RELAY, RELAY_OFF);
			if(bright_mode==2) set_brightness(bright0);
		}
	}
#endif

	// проверка статуса датчика движения
	if(digitalRead(PIN_MOTION) != cur_motion) {
		cur_motion = ! cur_motion;
		digitalWrite(LED_MOTION, show_move || alarmStartTime ? cur_motion: 0);
		last_move = millis(); // как включение, так и выключение датчика сбрасывает таймер
		fl_action_move = cur_motion;
		if(!fl_5v) {
			// если питания нет, а датчик движения сработал, то питать матрицу от аккумулятора
			if(cur_motion && millis()-last_screen_no5V>(max_move)*500L) {
				fl_allowLEDS = cur_motion;
				digitalWrite(PIN_RELAY, RELAY_OP(cur_motion));
				if(fl_allowLEDS && screenIsFree) {
					// если на экране должно быть время, то сразу его отрисовать
					// иначе на экране будет непонятно что. Если бежит строка, то
					// время обновления и так маленькое, естественным путём перерисует
					delay(RELAY_OP_TIME); // задержка на время срабатывания рэле
					screenIsFree = false;
					display_tick();
				}
			}
		}
	}
	// выключение матрицы с задержкой ИЛИ по таймауту, если нет питания 5V
	if((
		!fl_5v && !cur_motion && fl_allowLEDS && millis()-last_move>(delay_move+2)*1000UL
		) || (
		!fl_5v && fl_allowLEDS && millis()-last_move>(max_move)*1000UL
	)) {
		fl_allowLEDS = false;
		last_screen_no5V = millis();
		digitalWrite(PIN_RELAY, RELAY_OFF);
	}
	// Задержка срабатывания действий при сработке датчика движения, для уменьшения ложных срабатываний
	if(fl_action_move && millis()-last_move>delay_move*1000UL) {
		fl_action_move = false;
		// остановить будильник если сработал датчик движения
		if(alarmStartTime) alarmsStop();
		// отправка уведомления
		if(sec_enable && use_move) {
			tb_send_msg(F("Возможно движение"));
			save_log_file(SEC_TEXT_MOVE);
		}
	}

	if(autoBrightnessTimer.isReady() && fl_5v) {
		int16_t cur_brightness = analogRead(PIN_PHOTO_SENSOR);
		int16_t min_brightness = cur_brightness > old_brightness ? old_brightness: cur_brightness; 
		// загрубление датчика освещённости. Чем ярче, тем больше разброс показаний
		if(abs(cur_brightness-old_brightness)>(min_brightness>0?(min_brightness>>4)+1:0) || fl_bright_boost != old_bright_boost) {
			// "охранная" функция, если освещённость изменилась резко, то отослать сообщение
			// фоторезистор имеет большую инертность, может приходить два сообщения
			if(sec_enable && use_brightness && abs(cur_brightness-old_brightness)>(min_brightness>0?(min_brightness>>3)+2:2)) {
				char buf[80];
				sprintf_P(buf,PSTR("Изменилось освещение: %i -> %i"), old_brightness, cur_brightness);
				tb_send_msg(buf);
				save_log_file(SEC_TEXT_BRIGHTNESS);
			}
			// усиление показаний датчика
			uint16_t val = bright_boost!=100 ? cur_brightness*bright_boost/100: cur_brightness;
			// дополнительная яркость по времени
			uint8_t add_val = fl_bright_boost ? bright_add: 0;
			switch(bright_mode) {
				case 0: // полный автомат от 1 до 255
					set_brightness(constrain((val >> 2) + 1 + add_val, 1, 255));
					break;
				case 1: // автоматический с ограничителем
					set_brightness(constrain((( val * bright0 ) >> 10) + 1 + add_val, 1,255));
					break;
				default: // ручной
					set_brightness(constrain((uint16_t)bright0 + (uint16_t)add_val, 1, 255));
			}
			old_brightness = cur_brightness;
			old_bright_boost = fl_bright_boost;
		}
	}

	if(alarmTimer.isReady()) {
		fl_save = false;
		t = getTime();
		// проверка времени работы бегущей строки
		i = t.tm_hour*60+t.tm_min;
		fl_run_allow = run_allow == 0 || (run_allow == 1 && i >= run_begin && i <= run_end);
		fl_bright_boost = boost_mode != 0 && 
			((boost_mode > 0 && boost_mode < 5 && i >= sunrise && i <= sunset) ||
			(boost_mode == 5 && i >= bright_begin && i <= bright_end));
		// перебор всех будильников, чтобы найти активный
		for(i=0; i<MAX_ALARMS; i++)
			if(alarms[i].settings & 512) {
				// активный будильник найден, проверка времени срабатывания
				if(alarms[i].hour == t.tm_hour && alarms[i].minute == t.tm_min) {
					// защита от повторного запуска
					if(!(alarms[i].settings & 1024)) {
						// определение других критериев срабатывания
						fl_doit = false;
						switch ((alarms[i].settings >> 7) & 3) {
							case 0: // разово
							case 1: // каждый день
								fl_doit = true;
								break;
							case 2: // по дням
								if((alarms[i].settings >> t.tm_wday) & 1)
									fl_doit = true;
						}
						if(fl_doit) { // будильник сработал
							if(alarmStartTime == 0) {
								active_alarm = i;
								mp3_volume(volume_start); // начинать с маленькой громкости
								mp3_reread(); // перечитать количество треков, почему-то без этого может не запуститься
								mp3_enableLoop(); // зациклить мелодию
								delay(10);
								mp3_play(alarms[i].melody); // запустить мелодию
								alarmStepTimer.reset();
								alarmStartTime = getTimeU(); // чтобы избежать конфликтов между будильниками на одно время и отсчитывать максимальное время работы
							}
							alarms[i].settings = alarms[i].settings | 1024; // установить флаг активности
						}
					}
				} else if(alarms[i].settings & 2048) { // будильник уже разбудил
					alarms[i].settings &= ~(3072U); // сбросить флаги активности
					if(((alarms[i].settings >> 7) & 3) == 0) { // это разовый будильник, надо отключить и сохранить настройки
						alarms[i].settings &= ~(512U);
						fl_save = true;
					}
				}
			}
		if(fl_save) save_config_alarms();
	}
	// плавное увеличение громкости и ограничение на время работы будильника
	if(alarmStartTime && alarmStepTimer.isReady()) {
		i = alarms[active_alarm].text;
		if(screenIsFree && i >= 0) {
			// вывод текста только на время работы будильника
			initRString(texts[i].text, texts[i].color_mode > 0 ? texts[i].color_mode: texts[i].color);
		}
		if(!mp3_isPlay()) {
			// мелодия не запустилась, повторить весь цикл сначала. Редко, но случается :(
			mp3_reread();
			mp3_enableLoop();
			delay(10);
			mp3_play(alarms[active_alarm].melody);
			alarmStartTime = getTimeU();
		} else
			// мелодия играет, увеличить громкость на единицу
			if(cur_Volume<volume_finish) mp3_volume(++cur_Volume);
		if(alarmStartTime + max_alarm_time * 60 < getTimeU()) alarmsStop(); // будильник своё отработал, наверное не разбудил
	}

	// если экран освободился, то выбор, что сейчас надо выводить.
	// проверка разрешения выводить бегущую строку
	if(fl_run_allow && alarmStartTime == 0) {
		fl_save = false;
		// в приоритете бегущая строка
		for(i=0; i<MAX_RUNNING; i++)
			if(screenIsFree) // дополнительная проверка нужна потому, что статус может поменяться в каждом цикле
				if(textTimer[i].isReady() && (texts[i].repeat_mode & 512)) {
					fl_doit = false;
					t = getTime();
					switch ((texts[i].repeat_mode >> 7) & 3) {
						case 0: // всегда, пока включён
							fl_doit = true;
							break;
						case 1: // по дате, в конкретный день месяца
							if(((texts[i].repeat_mode >> 10) & 31) == t.tm_mday)
								fl_doit = true;
							break;
						case 2: // по дням недели
							if((texts[i].repeat_mode >> t.tm_wday) & 1)
								fl_doit = true;
							break;
						default: // до конца дня
							if(((texts[i].repeat_mode >> 10) & 31) == t.tm_mday)
								fl_doit = true;
							else {
								texts[i].repeat_mode &= 511;
								fl_save = true;
							}
					}
					if(fl_doit) initRString(texts[i].text, texts[i].color_mode > 0 ? texts[i].color_mode: texts[i].color);
				}
		if(fl_save) save_config_texts();
		// затем дата
		if(screenIsFree && clockDate.isReady())
			initRString(show_date_short ? dateCurrentTextShort(timeString): dateCurrentTextLong(timeString),
				show_date_color > 0 ? show_date_color: show_date_color0);
	}
	// если всё уже показано, то вывести время
	if(!fl_demo && screenIsFree && clockTimer.isReady())
		initRString(clockCurrentText(timeString), show_time_color > 0 ? show_time_color: show_time_color0, CLOCK_SHIFT);

	if(fl_demo) {
		if(demoTimer.isReady()) demo_tick();
	} else {
		if(scrollTimer.isReady()) display_tick();
	}
}
