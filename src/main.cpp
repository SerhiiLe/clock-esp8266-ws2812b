/**
 * @file main.cpp
 * @author Serhii Lebedenko (slebedenko@gmail.com)
 * @brief 
 * @version 1.1.0
 * @date 2022-03-31
 * 
 * @copyright Copyright (c) 2021,2022
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
#if USE_FTP == 1
#include <ftp.h>
#endif

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

timerMinim autoBrightnessTimer(300);	// Таймер отслеживания показаний датчика света при включенной авторегулировки яркости матрицы
timerMinim clockTimer(512);				// Таймер, чтобы разделитель часов и минут мигал примерно каждую секунду
timerMinim scrollTimer(scroll_period);	// Таймер обновления бегущей строки
timerMinim ntpSyncTimer(60000U * sync_time_period);  // Таймер синхронизации времени с NTP-сервером
timerMinim clockDate(1000U * show_date_period); // периодичность вывода даты в секундах
timerMinim textTimer[MAX_RUNNING];
timerMinim alarmTimer(1000);			// для будильника, срабатывает каждую секунду
timerMinim alarmStepTimer(5000);		// шаг увеличения громкости будильника
timerMinim demoTimer(33);				// таймер для теста/демонстрации экрана
timerMinim telegramTimer(1000U * tb_rate); // период опроса команд из Телеграм

// файловая система подключена
bool fs_isStarted = false;
// время начала работы будильника
time_t alarmStartTime = 0;
// яркость прошлого цикла
uint16_t old_brightness = 2000;
// состояние датчика движения
bool cur_motion = false;
// время последней сработки датчика движения
unsigned long last_move = 0;
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

void digitalToggle(byte pin){
  	digitalWrite(pin, !digitalRead(pin));
}

void setup() {
	Serial.begin(115200);
	Serial.println(PSTR("Starting..."));
	pinMode(LED_MOTION, OUTPUT);
	pinMode(PIN_MOTION, INPUT);
	pinMode(PIN_5V, INPUT);
	pinMode(PIN_RELAY, OUTPUT);
	digitalWrite(PIN_RELAY, 0);
	delay(RELAY_OP_TIME * 2); // задержка на время срабатывания (выключения) рэле. А вообще должно было стоять рэле LOW и тогда после старта оно бы сразу было выключено.
	display_setup();
	randomSeed(analogRead(PIN_PHOTO_SENSOR)+analogRead(PIN_PHOTO_SENSOR));
	screenIsFree = true;
	// initRString(PSTR("..."),1,8);
	initRString(PSTR("boot"),1,5);
	display_tick();
	if( LittleFS.begin()) {
		fs_isStarted = true; // встроенный диск подключился
		Serial.println(PSTR("LittleFS mounted"));
	} else {
		Serial.println(PSTR("ERROR LittleFS mount"));
		initRString(PSTR("Ошибка подключения встроенного диска!!!"));
	}
	load_config_main();
	load_config_alarms();
	load_config_texts();
	load_config_security();
	initRString(str_hello);
	wifi_setup();
	init_telegram();
}

// выключение всех активных в данный момент будильников
void alarmsStop() {
	// для начала надо остановить проигрывание мелодии и сбросить таймер активности
	alarmStartTime = 0;
	delay(10);
	mp3_disableLoop();
	delay(10);
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
		// запуск сервисов, которые должны запуститься после запуска сети. (сеть должна подниматься в фоне)
#if (USE_FTP==1)
		ftp_process(); // может тормозить
#endif
		web_process();
		if(telegramTimer.isReady()) tb_tick();
	}

	if( mp3_isInit ) mp3_check();
	btn.tick();

	if( wifi_isConnected && ( fl_timeNotSync || ntpSyncTimer.isReady() ) ) {
		// установка времени по ntp.
		if( fl_timeNotSync ) {
			// первичная установка времени. Если по каким-то причинам опрос не удался, повторять не чаще, чем раз 5 сек.
			if( alarmStepTimer.isReady() ) Serial.println(syncTime());
		} else // это плановая синхронизация, не критично, если опрос не прошел
			Serial.println(syncTime());
	}

	if(btn.isHolded()) {
		Serial.println("holded");

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
			Serial.println(F("Single"));
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
			Serial.println(F("Double"));
			if(fl_password_reset_req) {
				web_password = "";
				initRString(PSTR("Пароль временно отключен. Зайдите в настройки и задайте новый!"),CRGB::OrangeRed);
			} else
				fl_demo = !fl_demo;
			break;
		case 3:
			Serial.println(F("Triple"));
			if(fl_password_reset_req) {
				if(!wifi_isPortal) wifi_startConfig(true);
			} else
				initRString("IP: "+wifi_currentIP());
			break;
		case 4:
			Serial.println(F("Quadruple"));
			char buf[20];
			sprintf(buf,"%i -> %i -> %i",analogRead(PIN_PHOTO_SENSOR), old_brightness*br_boost/100, led_brightness);
			initRString(buf);
			break;
		case 5:
			Serial.println(F("Fivefold"));
			initRString(PSTR("Сброс пароля! Для подтверждения - 2й клик! Или 3й для сброса WiFi. 1 клик - отмена."),CRGB::OrangeRed);
			fl_password_reset_req = true;
			break;
	}

	// проверка наличия напряжения 5 Вольт
	if(digitalRead(PIN_5V) != fl_5v) {
		fl_5v = ! fl_5v;
		if(sec_enable) save_log_file(fl_5v ? SEC_TEXT_POWERED: SEC_TEXT_POWEROFF);
		fl_allowLEDS = fl_5v;
		// если нет питания - отключить питание матрицы и снизить яркость на минимум
		if(!fl_5v) {
			set_brightness(1);
			old_brightness = 1;
		} else {
			digitalWrite(PIN_RELAY, 0);
			if(bright_mode==2) set_brightness(bright0);
		}
	}

	// проверка статуса датчика движения
	if(digitalRead(PIN_MOTION) != cur_motion) {
		cur_motion = ! cur_motion;
		digitalWrite(LED_MOTION, show_move || alarmStartTime ? cur_motion: 0);
		if(cur_motion) {
			last_move = millis();
			fl_action_move = true;
		}
		if(!fl_5v) {
			// если питания нет, а датчик движения сработал, то запитать матрицу от аккумулятора
			fl_allowLEDS = cur_motion;
			digitalWrite(PIN_RELAY, cur_motion);
			if(fl_allowLEDS) {
				// если на экране должно быть время, то сразу его отрисовать
				// иначе на экране будет непонятно что. Если бежит строка, то
				// время обновления и так маленькое, естественным путём перерисует
				if( screenIsFree ) {
					delay(RELAY_OP_TIME); // задержка на время срабатывания рэле
					screenIsFree = false;
					display_tick();
				}
			}
		}
	}
	// Задержка срабатывания действий при сработке датчика движения, для уменьшения ложных срабатываний
	if(cur_motion && millis()-last_move>delay_move*1000UL && fl_action_move) {
		fl_action_move = false;
		// остановить будильник если сработал датчик движения
		if(alarmStartTime) alarmsStop();
		// отправка уведомления
		tb_send_msg(F("Сработал датчик."));
	}

	if(autoBrightnessTimer.isReady() && fl_5v) {
		uint16_t cur_brightness = analogRead(PIN_PHOTO_SENSOR);
		if(abs(cur_brightness-old_brightness)>1) {
			// усиление показаний датчика
			uint16_t val = br_boost!=100 ? cur_brightness*br_boost/100: cur_brightness;
			switch(bright_mode) {
			case 0: // полный автомат от 1 до 255
				set_brightness(constrain((val >> 2) + 1, 1, 255));
				break;
			case 1: // автоматический с ограничителем
				set_brightness(constrain((( val * bright0 ) >> 10) + 1, 1,255));
				break;
			}
		}
		old_brightness = cur_brightness;
	}

	if(alarmTimer.isReady()) {
		fl_save = false;
		t = getTime();
		// проверка времени работы бегущей строки
		i = t.tm_hour*60+t.tm_min;
		fl_run_allow = run_allow == 0 || (run_allow == 1 && i >= run_begin && i <= run_end);
		// проверка времени ускорения работы telegram
		if(fl_accelTelegram)
			if(getTimeU() - last_telegram > TELEGRAM_TIMEOUT) {
				telegramTimer.setInterval(1000U * tb_rate);
				fl_accelTelegram = false;
			}
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
								mp3_volume(5); // начинать с маленькой громкости
								delay(10);
								mp3_play(alarms[i].melody); // запустить мелодию
								delay(10);
								mp3_enableLoop(); // зациклить мелодию
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
		if(cur_Volume<30) mp3_volume(++cur_Volume);
		if(alarmStartTime + max_alarm_time * 60 < getTimeU()) alarmsStop(); // будильник своё отработал, наверное не разбудил
	}

	// если экран освободился, то выбор, что сейчас надо выводить.
	// проверка разрешения выводить бегущую строку
	if(fl_run_allow) {
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
		initRString(clockCurrentText(timeString), show_time_color > 0 ? show_time_color: show_time_color0, 2);

	if(fl_demo) {
		if(demoTimer.isReady()) demo_tick();
	} else {
		if(scrollTimer.isReady()) display_tick();
	}
}
