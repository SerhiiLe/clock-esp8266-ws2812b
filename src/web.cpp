/*
	встроенный web сервер для настройки часов
	(для начальной настройки ip и wifi используется wifi_init)
*/

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "defines.h"
#include "web.h"
#include "settings.h"
#include "runningText.h"
#include "ntp.h"
#include "dfplayer.h"
#include "security.h"

ESP8266WebServer HTTP(80);
ESP8266HTTPUpdateServer httpUpdater;
bool web_isStarted = false;

void save_settings();
void save_alarm();
void off_alarm();
void save_text();
void off_text();
void play();
void maintence();
void onoff();
void logout();

bool fileSend(String path);

// отключение веб сервера для активации режима настройки wifi
void web_disable() {
	HTTP.stop();
	web_isStarted = false;
}

// отправка простого текста
void text_send(String s, uint16_t r = 200) {
	HTTP.send(r, F("text/plain"), s);
}
// отправка сообщение "не найдено"
void not_found() {
	text_send(F("Not Found"), 404);
}

// диспетчер вызовов веб сервера
void web_process() {
	if( web_isStarted )
		HTTP.handleClient();
	else {
		HTTP.begin();
		// Обработка HTTP-запросов
		HTTP.on(F("/save_settings"), save_settings);
		HTTP.on(F("/save_alarm"), save_alarm);
		HTTP.on(F("/off_alarm"), off_alarm);
		HTTP.on(F("/save_text"), save_text);
		HTTP.on(F("/off_text"), off_text);
		HTTP.on(F("/play"), play);
		HTTP.on(F("/clear"), maintence);
		HTTP.on(F("/onoff"), onoff);
		HTTP.on(F("/logout"), logout);
		HTTP.onNotFound([](){
			if(!fileSend(HTTP.uri()))
				not_found();
			});
		web_isStarted = true;
  		httpUpdater.setup(&HTTP, web_login, web_password);
	}
}

// страничка выхода, будет предлагать ввести пароль, пока он не перестанет совпадать с реальным
void logout() {
	if(web_login.length() > 0 && web_password.length() > 0)
		if(HTTP.authenticate(web_login.c_str(), web_password.c_str()))
			HTTP.requestAuthentication(DIGEST_AUTH);
	if(!fileSend(F("/logged-out.html")))
		not_found();
}

// список файлов, для которых авторизация не нужна, остальные под паролем
bool auth_need(String s) {
	if(s == F("/index.html")) return false;
	if(s == F("/about.html")) return false;
	if(s == F("/logged-out.html")) return false;
	if(s.endsWith(F(".js"))) return false;
	if(s.endsWith(F(".css"))) return false;
	if(s.endsWith(F(".ico"))) return false;
	if(s.endsWith(F(".png"))) return false;
	return true;
}

// авторизация. много коментариев из документации, чтобы по новой не искать
bool is_no_auth() {
	// allows you to set the realm of authentication Default:"Login Required"
	// const char* www_realm = "Custom Auth Realm";
	// the Content of the HTML response in case of Unautherized Access Default:empty
	// String authFailResponse = "Authentication Failed";
	if(web_login.length() > 0 && web_password.length() > 0 )
		if(!HTTP.authenticate(web_login.c_str(), web_password.c_str())) {
		    //Basic Auth Method with Custom realm and Failure Response
			//return server.requestAuthentication(BASIC_AUTH, www_realm, authFailResponse);
			//Digest Auth Method with realm="Login Required" and empty Failure Response
			//return server.requestAuthentication(DIGEST_AUTH);
			//Digest Auth Method with Custom realm and empty Failure Response
			//return server.requestAuthentication(DIGEST_AUTH, www_realm);
			//Digest Auth Method with Custom realm and Failure Response
			HTTP.requestAuthentication(DIGEST_AUTH);
			return true;
		}
	return false;
}

// отправка файла
bool fileSend(String path) {
	// если путь пустой - исправить на индексную страничку
	if( path.endsWith("/") ) path += F("index.html");
	// проверка необходимости авторизации
	if(auth_need(path))
		if(is_no_auth()) return false;
	// определение типа файла
	const char *ct = nullptr;
	if(path.endsWith(F(".html"))) ct = PSTR("text/html");
	else if(path.endsWith(F(".css"))) ct = PSTR("text/css");
	else if(path.endsWith(F(".js"))) ct = PSTR("application/javascript");
	else if(path.endsWith(F(".json"))) ct = PSTR("application/json");
	else if(path.endsWith(F(".png"))) ct = PSTR("image/png");
	else if(path.endsWith(F(".jpg"))) ct = PSTR("image/jpeg");
	else if(path.endsWith(F(".gif"))) ct = PSTR("image/gif");
	else if(path.endsWith(F(".ico"))) ct = PSTR("image/x-icon");
	else ct = PSTR("text/plain");
	// открытие файла на чтение
	if(LittleFS.exists(path)) {
		File file = LittleFS.open(path, "r");
		// файл существует и открыт, выделение буфера передачи и отсылка заголовка
		char buf[1476];
		size_t sent = 0;
		int siz = file.size();
		HTTP.client().printf_P(PSTR("HTTP/1.1 200\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n"),ct,siz);
		// отсылка файла порциями, по размеру буфера или остаток
		while(siz > 0) {
			size_t len = std::min((int)(sizeof(buf) - 1), siz);
			file.read((uint8_t *)buf, len);
			HTTP.client().write((const char*)buf, len);
			siz -= len;
			sent+=len;
		}
		file.close();  
	} else return false; // файла нет, ошибка
	return true;
}

// декодирование времени, заданного в поле input->time
uint16_t decode_time(String s) {
	// выделение часов и минут из строки вида 00:00
	size_t pos = s.indexOf(":");
	uint8_t h = constrain(s.toInt(), 0, 23);
	uint8_t m = constrain(s.substring(pos+1).toInt(), 0, 59);
	return h*60 + m;
}

// сохранение настроек
void save_settings() {
	if(is_no_auth()) return;
	bool need_save = false;
	String name = F("str_hello");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != str_hello ) {
			str_hello = HTTP.arg(name);
			need_save = true;
		}
	}
	name = F("max_alarm_time");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != max_alarm_time ) {
			max_alarm_time = constrain(HTTP.arg(name).toInt(), 1, 30);
			need_save = true;
		}
	}
	name = F("run_allow");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != run_allow ) {
			run_allow = constrain(HTTP.arg(name).toInt(), 0, 2);
			need_save = true;
		}
	}
	name = F("run_begin");
	if( HTTP.hasArg(name) ) {
		if( decode_time(HTTP.arg(name)) != run_begin ) {
			run_begin = decode_time(HTTP.arg(name));
			need_save = true;
		}
	}
	name = F("run_end");
	if( HTTP.hasArg(name) ) {
		if( decode_time(HTTP.arg(name)) != run_end ) {
			run_end = decode_time(HTTP.arg(name));
			need_save = true;
		}
	}
	name = F("wide_font");
	if( HTTP.hasArg(name) ) {
		if( wide_font == 0 ) {
			wide_font = 1;
			need_save = true;
		}
	} else {
		if( wide_font > 0 ) {
			wide_font = 0;
			need_save = true;
		}
	}
	name = F("show_move");
	if( HTTP.hasArg(name) ) {
		if( show_move == 0 ) {
			show_move = 1;
			need_save = true;
		}
	} else {
		if( show_move > 0 ) {
			show_move = 0;
			need_save = true;
		}
	}
	name = F("delay_move");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != delay_move ) {
			delay_move = constrain(HTTP.arg(name).toInt(), 0, 10);
			need_save = true;
		}
	}
	bool sync_time = false;
	name = F("tz_shift");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != tz_shift ) {
			tz_shift = constrain(HTTP.arg(name).toInt(), -12, 12);
			need_save = true;
			sync_time = true;
		}
	}
	name = F("tz_dst");
	if( HTTP.hasArg(name) ) {
		if( tz_dst == 0 ) {
			tz_dst = 1;
			need_save = true;
			sync_time = true;
		}
	} else {
		if( tz_dst > 0 ) {
			tz_dst = 0;
			need_save = true;
			sync_time = true;
		}
	}
	name = F("date_short");
	if( HTTP.hasArg(name) ) {
		if( show_date_short == 0 ) {
			show_date_short = 1;
			need_save = true;
		}
	} else {
		if( show_date_short > 0 ) {
			show_date_short = 0;
			need_save = true;
		}
	}
	name = F("date_period");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != show_date_period ) {
			show_date_period = constrain(HTTP.arg(name).toInt(), 20, 1440);
			clockDate.setInterval(1000U * show_date_period);
			need_save = true;
		}
	}
	name = F("time_color");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != show_time_color ) {
			show_time_color = constrain(HTTP.arg(name).toInt(), 0, 3);
			need_save = true;
		}
	}
	name = F("time_color0");
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != show_time_color0 ) {
			show_time_color0 = text_to_color(HTTP.arg(name).c_str());
			need_save = true;
		}
	}
	name = F("time_color1");
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != show_time_col[0] ) {
			show_time_col[0] = text_to_color(HTTP.arg(name).c_str());
			need_save = true;
		}
	}
	name = F("time_color2");
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != show_time_col[1] ) {
			show_time_col[1] = text_to_color(HTTP.arg(name).c_str());
			need_save = true;
		}
	}
	name = F("time_color3");
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != show_time_col[2] ) {
			show_time_col[2] = text_to_color(HTTP.arg(name).c_str());
			need_save = true;
		}
	}
	name = F("time_color4");
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != show_time_col[3] ) {
			show_time_col[3] = text_to_color(HTTP.arg(name).c_str());
			need_save = true;
		}
	}
	name = F("time_color5");
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != show_time_col[4] ) {
			show_time_col[4] = text_to_color(HTTP.arg(name).c_str());
			need_save = true;
		}
	}
	name = F("date_color");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != show_date_color ) {
			show_date_color = constrain(HTTP.arg(name).toInt(), 0, 2);
			need_save = true;
		}
	}
	name = F("date_color0");
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != show_date_color0 ) {
			show_date_color0 = text_to_color(HTTP.arg(name).c_str());
			need_save = true;
		}
	}
	name = F("bright_mode");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != bright_mode ) {
			bright_mode = constrain(HTTP.arg(name).toInt(), 0, 2);
			need_save = true;
		}
	}
	name = F("bright0");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != bright0 ) {
			bright0 = constrain(HTTP.arg(name).toInt(), 1, 255);
			need_save = true;
		}
		if(bright_mode==2) set_brightness(bright0);
	}
	name = F("br_boost");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != br_boost ) {
			br_boost = constrain(HTTP.arg(name).toInt(), 1, 250);
			need_save = true;
		}
	}
	name = F("max_power");
	if( HTTP.hasArg(name) ) {
		if( (uint32_t)HTTP.arg(name).toInt() != max_power ) {
			max_power = constrain(HTTP.arg(name).toInt(), 200, MAX_POWER);
			if(DEFAULT_POWER > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, max_power);
			need_save = true;
		}
	}
	name = F("turn_display");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != turn_display ) {
			turn_display = constrain(HTTP.arg(name).toInt(), 0, 3);
			need_save = true;
		}
	}
	name = F("volume_start");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != volume_start ) {
			volume_start = constrain(HTTP.arg(name).toInt(), 1, 30);
			need_save = true;
		}
	}
	name = F("volume_finish");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != volume_finish ) {
			volume_finish = constrain(HTTP.arg(name).toInt(), 1, 30);
			need_save = true;
		}
	}
	volume_finish = constrain(volume_finish, volume_start, 30);
	name = F("volume_period");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != volume_period ) {
			volume_period = constrain(HTTP.arg(name).toInt(), 1, 30);
			alarmStepTimer.setInterval(1000U * volume_period);
			need_save = true;
		}
	}
	name = F("sync_time_period");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != sync_time_period ) {
			sync_time_period = constrain(HTTP.arg(name).toInt(), 20, 1440);
			ntpSyncTimer.setInterval(60000U * sync_time_period);
			need_save = true;
		}
	}
	name = F("scroll_period");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != scroll_period ) {
			scroll_period = constrain(HTTP.arg(name).toInt(), 20, 1440);
			scrollTimer.setInterval(scroll_period);
			need_save = true;
		}
	}
	bool fl_setTelegram = false;
	name = F("tb_name");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != tb_name ) {
			tb_name = HTTP.arg(name);
			need_save = true;
		}
	}
	name = F("tb_chats");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != tb_chats ) {
			tb_chats = HTTP.arg(name);
			need_save = true;
			fl_setTelegram = true;
		}
	}
	name = F("tb_token");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != tb_token ) {
			tb_token = HTTP.arg(name);
			need_save = true;
			fl_setTelegram = true;
		}
	}
	name = F("tb_secret");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != tb_secret ) {
			tb_secret = HTTP.arg(name);
			need_save = true;
		}
	}
	name = F("tb_rate");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != tb_rate ) {
			tb_rate = constrain(HTTP.arg(name).toInt(), 0, 1440);
			// telegramTimer.setInterval(1000U * tb_rate);
			need_save = true;
		}
	}
	bool need_web_restart = false;
	name = F("web_login");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != web_login ) {
			web_login = HTTP.arg(name);
			need_save = true;
			need_web_restart = true;
		}
	}
	name = F("web_password");
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != web_password ) {
			web_password = HTTP.arg(name);
			need_save = true;
			need_web_restart = true;
		}
	}
	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_main();
	initRString(PSTR("Настройки сохранены"));
	if( sync_time ) syncTime();
	if(fl_setTelegram) setup_telegram();
	if(need_web_restart) httpUpdater.setup(&HTTP, web_login, web_password);
}

// перезагрузка часов, сброс ком-порта, отключение сети и диска, чтобы ничего не мешало перезагрузке
void reboot_clock() {
	Serial.flush();
	WiFi.forceSleepBegin(); //disable AP & station by calling "WiFi.mode(WIFI_OFF)" & put modem to sleep
	LittleFS.end();
	delay(1000);
	ESP.restart();
}

void maintence() {
	if(is_no_auth()) return;
	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303); 
	initRString(PSTR("Сброс"));
	if( HTTP.hasArg("t") ) {
		if( HTTP.arg("t") == "t" && LittleFS.exists(F("/texts.json")) ) {
			Serial.println(F("reset texts"));
			LittleFS.remove(F("/texts.json"));
			reboot_clock();
		}
		if( HTTP.arg("t") == "a" && LittleFS.exists(F("/alarms.json")) ) {
			Serial.println(F("reset alarms"));
			LittleFS.remove(F("/alarms.json"));
			reboot_clock();
		}
		if( HTTP.arg("t") == "c" && LittleFS.exists(F("/config.json")) ) {
			Serial.println(F("reset settings"));
			LittleFS.remove(F("/config.json"));
			reboot_clock();
		}
		if( HTTP.arg("t") == "l" ) {
			Serial.println(F("erase logs"));
			char fileName[32];
			for(int8_t i=0; i<SEC_LOG_COUNT; i++) {
				sprintf_P(fileName, SEC_LOG_FILE, i);
				if( LittleFS.exists(fileName) ) LittleFS.remove(fileName);
			}
			reboot_clock();
		}
		if( HTTP.arg("t") == "r" ) {
			reboot_clock();
		}
	}
}

// сохранение настроек будильника
void save_alarm() {
	if(is_no_auth()) return;
	bool need_save = false;
	uint8_t target = 0;
	uint16_t settings = 512;
	String name = F("target");
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		name = F("time");
		if( HTTP.hasArg(name) ) {
			// выделение часов и минут из строки вида 00:00
			size_t pos = HTTP.arg(name).indexOf(":");
			uint8_t h = constrain(HTTP.arg(name).toInt(), 0, 23);
			uint8_t m = constrain(HTTP.arg(name).substring(pos+1).toInt(), 0, 59);
			if( h != alarms[target].hour || m != alarms[target].minute ) {
				alarms[target].hour = h;
				alarms[target].minute = m;
				need_save = true;
			}
		}
		name = F("rmode");
		if( HTTP.hasArg(name) ) settings |= constrain(HTTP.arg(name).toInt(), 0, 3) << 7;
		if( HTTP.hasArg(F("mo")) ) settings |= 2;
		if( HTTP.hasArg(F("tu")) ) settings |= 4;
		if( HTTP.hasArg(F("we")) ) settings |= 8;
		if( HTTP.hasArg(F("th")) ) settings |= 16;
		if( HTTP.hasArg(F("fr")) ) settings |= 32;
		if( HTTP.hasArg(F("sa")) ) settings |= 64;
		if( HTTP.hasArg(F("su")) ) settings |= 1;
		if( settings != alarms[target].settings ) {
			alarms[target].settings = settings;
			need_save = true;
		}
		name = F("melody");
		if( HTTP.hasArg(name) ) {
			if( HTTP.arg(name).toInt() != alarms[target].melody ) {
				alarms[target].melody = constrain(HTTP.arg(name).toInt(), 1, mp3_all);
				need_save = true;
			}
		}
	}
	HTTP.sendHeader(F("Location"),F("/alarms.html"));
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_alarms();
	mp3_stop();
	initRString(PSTR("Будильник установлен"));
}

// отключение будильника
void off_alarm() {
	if(is_no_auth()) return;
	uint8_t target = 0;
	String name = "t";
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		if( alarms[target].settings & 512 ) {
			alarms[target].settings &= ~(512U);
			save_config_alarms();
			text_send("1");
			initRString(PSTR("Будильник отключен"));
		}
	} else
		text_send("0");
}

// сохранение настроек бегущей строки. Строки настраиваются по одной.
void save_text() {
	if(is_no_auth()) return;
	bool need_save = false;
	uint8_t target = 0;
	uint16_t settings = 512;
	String name = F("target");
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		name = F("text");
		if( HTTP.hasArg(name) ) {
			if( HTTP.arg(name) != texts[target].text ) {
				texts[target].text = HTTP.arg(name);
				need_save = true;
			}
		}
		name = F("period");
		if( HTTP.hasArg(name) ) {
			if( HTTP.arg(name).toInt() != texts[target].period ) {
				texts[target].period = constrain(HTTP.arg(name).toInt(),30,3600);
				need_save = true;
				textTimer[target].setInterval(texts[target].period*1000U);
			}
		}
		name = F("color_mode");
		if( HTTP.hasArg(name) ) {
			if( HTTP.arg(name).toInt() != texts[target].color_mode ) {
				texts[target].color_mode = constrain(HTTP.arg(name).toInt(), 0, 3);
				need_save = true;
			}
		}
		name = F("color");
		if( HTTP.hasArg(name) ) {
			if( text_to_color(HTTP.arg(name).c_str()) != texts[target].color ) {
				texts[target].color = text_to_color(HTTP.arg(name).c_str());
				need_save = true;
			}
		}
		name = F("rmode");
		if( HTTP.hasArg(name) ) settings |= constrain(HTTP.arg(name).toInt(), 0, 3) << 7;
		if( HTTP.hasArg("mo") ) settings |= 2;
		if( HTTP.hasArg("tu") ) settings |= 4;
		if( HTTP.hasArg("we") ) settings |= 8;
		if( HTTP.hasArg("th") ) settings |= 16;
		if( HTTP.hasArg("fr") ) settings |= 32;
		if( HTTP.hasArg("sa") ) settings |= 64;
		if( HTTP.hasArg("su") ) settings |= 1;
		if((settings >> 7 & 3) == 3) { // если режим "до конца дня", то записать текущий день
			tm t = getTime();
			settings |= t.tm_mday << 10;
		} else { // иначе то, что передано формой
			name = "sel_day";
			if( HTTP.hasArg(name) ) settings |= constrain(HTTP.arg(name).toInt(), 1, 31) << 10;
		}
		if( settings != texts[target].repeat_mode ) {
			texts[target].repeat_mode = settings;
			need_save = true;
		}
	}
	HTTP.sendHeader(F("Location"),F("/running.html"));
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_texts();
	initRString(PSTR("Текст включен"));
}

// отключение бегущей строки
void off_text() {
	if(is_no_auth()) return;
	uint8_t target = 0;
	String name = "t";
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		if( texts[target].repeat_mode & 512 ) {
			texts[target].repeat_mode &= ~(512U);
			save_config_texts();
			text_send("1");
			initRString(PSTR("Текст отключен"));
		}
	} else
		text_send("0");
}

// костыль для настройки режима посвтора. Работает 50/50
void repeat_mode(uint8_t r) {
	static uint8_t old_r = 0;
	if(r==0) {
		if(old_r==1) mp3_disableLoop();
		if(old_r==2) mp3_disableLoopAll();
		// mp3_stop();
	}
	if(r==1) {
		if(old_r==2) mp3_disableLoopAll();
		mp3_enableLoop();
	}
	if(r==2) {
		if(old_r==1) mp3_disableLoop();
		mp3_enableLoopAll();
	}
	if(r==3) mp3_randomAll();
	old_r = r;
}

// обслуживает страничку плейера.
void play() {
	if(is_no_auth()) return;
	uint8_t p = 0;
	uint8_t r = 0;
	uint8_t v = 15;
	uint16_t c = 1;
	int t = 0;
	String name = "p";
	if( HTTP.hasArg(name) ) p = HTTP.arg(name).toInt();
	name = "c";
	if( HTTP.hasArg(name) ) c = constrain(HTTP.arg(name).toInt(), 1, mp3_all);
	name = "r";
	if( HTTP.hasArg(name) ) r = constrain(HTTP.arg(name).toInt(), 0, 3);
	name = "v";
	if( HTTP.hasArg(name) ) v = constrain(HTTP.arg(name).toInt(), 1, 30);
	switch (p)	{
		case 1: // предыдущий трек
			t = mp3_current - 1;
			if(t<1) t=mp3_all;
			mp3_play(t);
			break;
		case 2: // следующий трек
			t = mp3_current + 1;
			if(t>mp3_all) t=1;
			mp3_play(t);
			break;
		case 3: // играть
			repeat_mode(r);
			delay(10);
			mp3_play(c);
			break;
		case 4: // пауза
			mp3_pause();
			break;
		case 5: // режим
			repeat_mode(r);
			break;
		case 6: // остановить
			mp3_stop();
			break;
		case 7: // тише
			t = cur_Volume - 1;
			if(t<0) t=0;
			mp3_volume(t);
			break;
		case 8: // громче
			t = cur_Volume + 1;
			if(t>30) t=30;
			mp3_volume(t);
			break;
		case 9: // громкость
			mp3_volume(v);
			break;
		default:
			if(!mp3_isInit) mp3_init();
			else mp3_reread();
			if(mp3_isInit) mp3_update();
			break;
	}
	char buff[20];
	sprintf(buff,"%i:%i:%i",mp3_current,mp3_all,cur_Volume);
	text_send(buff);
}

void onoff() {
	if(is_no_auth()) return;
	int8_t a=0;
	bool cond=false;
	String name = "a";
	if(HTTP.hasArg(name)) a = HTTP.arg(name).toInt();
	name = "t";
	if(HTTP.hasArg(name)) {
		if(HTTP.arg(name) == F("demo")) {
			// включает/выключает демо режим
			if(a) fl_demo = !fl_demo;
			cond = fl_demo;
		} else 
		if(HTTP.arg(name) == F("ftp")) {
			// включает/выключает ftp сервер, чтобы не кушал ресурсов просто так
			if(a) ftp_isAllow = !ftp_isAllow;
			cond = ftp_isAllow;
		} else
		if(HTTP.arg(name) == F("security")) {
			// включает/выключает режим "охраны"
			if(a) sec_enable = !(bool)sec_enable;
			cond = sec_enable;
			save_log_file(cond?SEC_TEXT_ENABLE:SEC_TEXT_DISABLE);
			save_config_security();
		}
	}
	text_send(cond?"1":"0");
}
