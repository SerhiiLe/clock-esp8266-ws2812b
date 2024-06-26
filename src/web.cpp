/*
	встроенный web сервер для настройки часов
	(для начальной настройки ip и wifi используется wifi_init)
*/

#include <Arduino.h>
#include "defines.h"
#ifdef ESP32
#include <WebServer.h>
#include "mHTTPUpdateServer.h"
#include <ESPmDNS.h>
#include <rom/rtc.h>
#else // ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#endif
#include <LittleFS.h>
#include <time.h>
#include "web.h"
#include "settings.h"
#include "runningText.h"
#include "ntp.h"
#include "dfplayer.h"
#include "security.h"
#include "clock.h"
#include "wifi_init.h"
#include "webClient.h"

#ifdef ESP32
WebServer HTTP(80);
HTTPUpdateServer httpUpdater;
#endif
#ifdef ESP8266
ESP8266WebServer HTTP(80);
ESP8266HTTPUpdateServer httpUpdater;
#endif
bool web_isStarted = false;

void save_settings();
void save_telegram();
void save_alarm();
void off_alarm();
void save_text();
void off_text();
void save_quote();
void show_quote();
void save_weather();
void show_weather();
void show();
void sysinfo();
void play();
void maintence();
void set_clock();
void onoff();
void send();
void logout();
void sensors();
void registration();

bool fileSend(String path);
bool need_save = false;
bool fl_mdns = false;

bool fl_playStarted = false;

// отключение веб сервера для активации режима настройки wifi
void web_disable() {
	HTTP.stop();
	web_isStarted = false;
	LOG(println, PSTR("HTTP server stoped"));

	#ifdef ESP32
	MDNS.disableWorkstation();
	#else // ESP8266
	MDNS.close();
	#endif
	fl_mdns = false;
	LOG(println, PSTR("MDNS responder stoped"));
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
	if( web_isStarted ) {
		HTTP.handleClient();
		#ifdef ESP8266
		if(fl_mdns) MDNS.update();
		#endif
	} else {
		HTTP.begin();
		// Обработка HTTP-запросов
		HTTP.on(F("/save_settings"), save_settings);
		HTTP.on(F("/save_telegram"), save_telegram);
		HTTP.on(F("/save_alarm"), save_alarm);
		HTTP.on(F("/off_alarm"), off_alarm);
		HTTP.on(F("/save_text"), save_text);
		HTTP.on(F("/off_text"), off_text);
		HTTP.on(F("/save_quote"), save_quote);
		HTTP.on(F("/show_quote"), show_quote);
		HTTP.on(F("/save_weather"), save_weather);
		HTTP.on(F("/show_weather"), show_weather);
		HTTP.on(F("/show"), show);
		HTTP.on(F("/sysinfo"), sysinfo);
		HTTP.on(F("/play"), play);
		HTTP.on(F("/clear"), maintence);
		HTTP.on(F("/clock"), set_clock);
		HTTP.on(F("/onoff"), onoff);
		HTTP.on(F("/send"), send);
		HTTP.on(F("/logout"), logout);
		HTTP.on(F("/sensors"), sensors);
		HTTP.on(F("/registration"), registration);
		HTTP.on(F("/who"), [](){
			text_send(ts.clock_name);
		});
		HTTP.onNotFound([](){
			if(!fileSend(HTTP.uri()))
				not_found();
			});
		web_isStarted = true;
  		httpUpdater.setup(&HTTP, gs.web_login, gs.web_password);
		LOG(println, PSTR("HTTP server started"));

		#ifdef ESP32
		if(MDNS.begin(ts.clock_name.c_str())) {
		#else // ESP8266
		if(MDNS.begin(ts.clock_name, WiFi.localIP())) {
		#endif
			MDNS.addService("http", "tcp", 80);
			fl_mdns = true;
			LOG(println, PSTR("MDNS responder started"));
		}
	}
}

// страничка выхода, будет предлагать ввести пароль, пока он не перестанет совпадать с реальным
void logout() {
	if(gs.web_login.length() > 0 && gs.web_password.length() > 0)
		if(HTTP.authenticate(gs.web_login.c_str(), gs.web_password.c_str()))
			HTTP.requestAuthentication(DIGEST_AUTH);
	if(!fileSend(F("/logged-out.html")))
		not_found();
}

// список файлов, для которых авторизация не нужна, остальные под паролем
bool auth_need(String s) {
	if(s == F("/index.html")) return false;
	if(s == F("/about.html")) return false;
	if(s == F("/send.html")) return false;
	if(s == F("/logged-out.html")) return false;
	if(s.endsWith(F(".js"))) return false;
	if(s.endsWith(F(".css"))) return false;
	if(s.endsWith(F(".ico"))) return false;
	if(s.endsWith(F(".png"))) return false;
	return true;
}

// авторизация. много комментариев из документации, чтобы по новой не искать
bool is_no_auth() {
	// allows you to set the realm of authentication Default:"Login Required"
	// const char* www_realm = "Custom Auth Realm";
	// the Content of the HTML response in case of Unauthorized Access Default:empty
	// String authFailResponse = "Authentication Failed";
	if(gs.web_login.length() > 0 && gs.web_password.length() > 0 )
		if(!HTTP.authenticate(gs.web_login.c_str(), gs.web_password.c_str())) {
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
	if(!fs_isStarted) {
		// файловая система не загружена, переход на страничку обновления
		HTTP.client().printf_P(PSTR("HTTP/1.1 200\r\nContent-Type: %s\r\nContent-Length: 80\r\nConnection: close\r\n\r\n<html><body><h1><a href='/update'>File system not exist!</a></h1></body></html>"),ct);
		return true;
	}
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

// печать байта в виде шестнадцатеричного числа
void print_byte(char *buf, byte c, size_t& pos) {
	if((c & 0xf) > 9)
		buf[pos+1] = (c & 0xf) - 10 + 'A';
	else
		buf[pos+1] = (c & 0xf) + '0';
	c = (c>>4) & 0xf;
	if(c > 9)
		buf[pos]=c - 10 + 'A';
	else
		buf[pos] = c+'0';
	pos += 2;
}

// кодирование строки для json
const char* jsonEncode(char* buf, const char *str, size_t max_length) {
	// size_t len = strlen(str);
	size_t p = 0, i = 0;
	byte c;

	while( str[i] != '\0' && p < max_length-8) {
		// Выделение символа UTF-8 и перевод его в UTF-16 для вывода в JSON
		// 0xxxxxxx - 7 бит 1 байт, 110xxxxx - 10 бит 2 байта, 1110xxxx - 16 бит 3 байта, 11110xxx - 21 бит 4 байта
		c = (byte)str[i++];
		if( c > 127  ) {
			buf[p++] = '\\';
			buf[p++] = 'u';
			// utf8 -> utf16
			if( c >> 5 == 6 ) {
				uint16_t cc = ((uint16_t)(str[i-1] & 0x1F) << 6);
				cc |= (uint16_t)(str[i++] & 0x3F);
				print_byte(buf, (byte)(cc>>8), p);
				print_byte(buf, (byte)(cc&0xff), p);
			} else if( c >> 4 == 14 ) {
				uint16_t cc = ((uint16_t)(str[i-1] & 0x0F) << 12);
				cc |= ((uint16_t)(str[i++] & 0x3F) << 6);
				cc |= (uint16_t)(str[i++] & 0x3F);
				print_byte(buf, (byte)(cc>>8), p);
				print_byte(buf, (byte)(cc&0xff), p);
			} else if( c >> 3 == 30 ) {
				uint32_t CP = ((uint32_t)(str[i-1] & 0x07) << 18);
				CP |= ((uint32_t)(str[i++] & 0x3F) << 12);
				CP |= ((uint32_t)(str[i++] & 0x3F) << 6);
				CP |= (uint32_t)(str[i++] & 0x3F);
				CP -= 0x10000;
				uint16_t cc = 0xD800 + (uint16_t)((CP >> 10) & 0x3FF);
				print_byte(buf, (byte)(cc>>8), p);
				print_byte(buf, (byte)(cc&0xff), p);
				cc = 0xDC00 + (uint16_t)(CP & 0x3FF);
				print_byte(buf, (byte)(cc>>8), p);
				print_byte(buf, (byte)(cc&0xff), p);
			}
		} else {
			buf[p++] = c;
		}
	}

	buf[p++] = '\0';
	return buf;
}

/****** шаблоны простых операций для выделения переменных из web ******/

// определение выбран checkbox или нет
bool set_simple_checkbox(const __FlashStringHelper * name, uint8_t &var) {
	if( HTTP.hasArg(name) ) {
		if( var == 0 ) {
			var = 1;
			need_save = true;
			return true;
		}
	} else {
		if( var > 0 ) {
			var = 0;
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение простых целых чисел
template <typename T>
bool set_simple_int(const __FlashStringHelper * name, T &var, long from, long to) {
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != (long)var ) {
			var = constrain(HTTP.arg(name).toInt(), from, to);
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение дробных чисел
bool set_simple_float(const __FlashStringHelper * name, float &var, float from, float to, float prec=8.0f) {
	if( HTTP.hasArg(name) ) {
		if( round(HTTP.arg(name).toFloat()*pow(10.0f,prec)) != round(var*pow(10.0f,prec)) ) {
			var = constrain(HTTP.arg(name).toFloat(), from, to);
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение простых строк
bool set_simple_string(const __FlashStringHelper * name, String &var) {
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != var ) {
			var = HTTP.arg(name);
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение времени
bool set_simple_time(const __FlashStringHelper * name, uint16_t &var) {
	if( HTTP.hasArg(name) ) {
		if( decode_time(HTTP.arg(name)) != var ) {
			var = decode_time(HTTP.arg(name));
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение цвета
bool set_simple_color(const __FlashStringHelper * name, uint32_t &var) {
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != var ) {
			var = text_to_color(HTTP.arg(name).c_str());
			need_save = true;
			return true;
		}
	}
	return false;
}

/****** обработка разных запросов ******/

// сохранение настроек
void save_settings() {
	if(is_no_auth()) return;
	need_save = false;

	set_simple_string(F("str_hello"), gs.str_hello);
	set_simple_int(F("max_alarm_time"), gs.max_alarm_time, 1, 30);
	set_simple_int(F("run_allow"), gs.run_allow, 0, 2);
	set_simple_time(F("run_begin"), gs.run_begin);
	set_simple_time(F("run_end"), gs.run_end);
	set_simple_checkbox(F("wide_font"), gs.wide_font);
	set_simple_checkbox(F("show_move"), gs.show_move);
	set_simple_int(F("delay_move"), gs.delay_move, 0, 10);
	set_simple_int(F("max_move"), gs.max_move, gs.delay_move, 255);
	bool sync_time = false;
	if( set_simple_int(F("tz_shift"), gs.tz_shift, -12, 12) )
		sync_time = true;
	if( set_simple_checkbox(F("tz_dst"), gs.tz_dst) )
		sync_time = true;
	set_simple_checkbox(F("tz_adjust"), gs.tz_adjust);
	set_simple_int(F("tiny_clock"), gs.tiny_clock, 0, 8);
	set_simple_int(F("dots_style"), gs.dots_style, 0, 11);
	set_simple_checkbox(F("t12h"), gs.t12h);
	set_simple_checkbox(F("date_short"), gs.show_date_short);
	if( set_simple_int(F("date_period"), gs.show_date_period, 20, 1439) )
		clockDate.setInterval(1000U * gs.show_date_period);
	set_simple_int(F("time_color"), gs.show_time_color, 0, 5);
	set_simple_color(F("time_color0"), gs.show_time_color0);
	// цвет часов
	set_simple_color(F("time_color1"), gs.show_time_col[0]);
	gs.show_time_col[1] = gs.show_time_col[0];
	// set_simple_color(F("time_color2"), gs.show_time_col[1]);
	// цвет разделителей
	set_simple_color(F("time_color3"), gs.show_time_col[2]);
	gs.show_time_col[5] = gs.show_time_col[2];
	// цвет минут
	set_simple_color(F("time_color4"), gs.show_time_col[3]);
	gs.show_time_col[4] = gs.show_time_col[3];
	// set_simple_color(F("time_color5"), gs.show_time_col[4]);
	// цвет секунд
	set_simple_color(F("time_color6"), gs.show_time_col[6]);
	gs.show_time_col[7] = gs.show_time_col[6];
	set_simple_int(F("date_color"), gs.show_date_color, 0, 4);
	set_simple_color(F("date_color0"), gs.show_date_color0);
	set_simple_int(F("hue_shift"), gs.hue_shift, 0, 4);
	bool need_bright = false;
	if( set_simple_int(F("bright_mode"), gs.bright_mode, 0, 2) )
		need_bright = true;
	if( set_simple_int(F("bright0"), gs.bright0, 1, 255) )
		need_bright = true;
	set_simple_int(F("br_boost"), gs.bright_boost, 1, 1000);
	if( set_simple_int(F("boost_mode"), gs.boost_mode, 0, 5) )
		sync_time = true;
	if( set_simple_int(F("br_add"), gs.bright_add, 1, 255) )
		need_bright = true;
	if( set_simple_float(F("latitude"), gs.latitude, -180.0f, 180.0f) )
		sync_time = true;
	if( set_simple_float(F("longitude"), gs.longitude, -180.0f, 180.0f) )
		sync_time = true;
	set_simple_time(F("br_begin"), gs.bright_begin);
	set_simple_time(F("br_end"), gs.bright_end);
	if( set_simple_int(F("max_power"), gs.max_power, 200, MAX_POWER) )
		if(DEFAULT_POWER > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, gs.max_power);
	set_simple_int(F("turn_display"), gs.turn_display, 0, 3);
	set_simple_int(F("volume_start"), gs.volume_start, 1, 30);
	set_simple_int(F("volume_finish"), gs.volume_finish, 1, 30);
	gs.volume_finish = constrain(gs.volume_finish, gs.volume_start, 30);
	if( set_simple_int(F("volume_period"), gs.volume_period, 1, 30) )
		alarmStepTimer.setInterval(1000U * gs.volume_period);
	if( set_simple_int(F("timeout_mp3"), gs.timeout_mp3, 1, 255) )
		timeoutMp3Timer.setInterval(3600000U * gs.timeout_mp3);
	if( set_simple_int(F("sync_time_period"), gs.sync_time_period, 1, 255) )
		ntpSyncTimer.setInterval(3600000U * gs.sync_time_period);
	if( set_simple_int(F("scroll_period"), gs.scroll_period, 0, 40) )
		scrollTimer.setInterval(60 - gs.scroll_period);
	bool need_web_restart = false;
	if( set_simple_string(F("web_login"), gs.web_login) )
		need_web_restart = true;
	if( set_simple_string(F("web_password"), gs.web_password) )
		need_web_restart = true;

	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_main();
	initRString(PSTR("Настройки сохранены"));
	if( sync_time ) syncTime();
	if( need_bright ) old_bright_boost = !old_bright_boost;
	if(need_web_restart) httpUpdater.setup(&HTTP, gs.web_login, gs.web_password);
}

// сохранение настроек telegram (охраны)
void save_telegram() {
	if(is_no_auth()) return;
	need_save = false;
	bool fl_setTelegram = false;

	set_simple_checkbox(F("use_move"), ts.use_move);
	set_simple_checkbox(F("use_brightness"), ts.use_brightness);
	set_simple_string(F("pin_code"), ts.pin_code);
	if( set_simple_string(F("clock_name"), ts.clock_name) )
		#ifdef ESP32
		if(fl_mdns)	MDNS.setInstanceName(ts.clock_name.c_str());
		#else // ESP8266
		if(fl_mdns)	MDNS.setHostname(ts.clock_name.c_str());
		#endif
	set_simple_int(F("sensor_timeout"), ts.sensor_timeout, 1, 16000);
	set_simple_string(F("tb_name"), ts.tb_name);
	if( set_simple_string(F("tb_chats"), ts.tb_chats) )
		fl_setTelegram = true;
	if( set_simple_string(F("tb_token"), ts.tb_token) )
		fl_setTelegram = true;
	set_simple_string(F("tb_secret"), ts.tb_secret);
	if( set_simple_int(F("tb_rate"), ts.tb_rate, 0, 3600) )
		telegramTimer.setInterval(1000U * ts.tb_rate);
	set_simple_int(F("tb_accelerated"), ts.tb_accelerated, 1, 600);
	set_simple_int(F("tb_accelerate"), ts.tb_accelerate, 1, 3600);
	set_simple_int(F("tb_ban"), ts.tb_ban, 900, 3600);

	HTTP.sendHeader(F("Location"),"/security.html");
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_telegram();
	initRString(PSTR("Настройки сохранены"));
	if(fl_setTelegram) setup_telegram();
}

// перезагрузка часов, сброс ком-порта, отключение сети и диска, чтобы ничего не мешало перезагрузке
void reboot_clock() {
	Serial.flush();
	#ifdef ESP32
	WiFi.getSleep();
	#else // ESP8266
	WiFi.forceSleepBegin(); //disable AP & station by calling "WiFi.mode(WIFI_OFF)" & put modem to sleep
	#endif
	LittleFS.end();
	delay(1000);
	ESP.restart();
}

void reset_settings(int t) {
	switch (t) {
		case 0: { // главная конфигурация
			LOG(println, PSTR("reset settings"));
			if(LittleFS.exists(F("/config.json"))) LittleFS.remove(F("/config.json"));
			} break;
		case 1: { // настройки будильников
			LOG(println, PSTR("reset alarms"));
			if(LittleFS.exists(F("/alarms.json"))) LittleFS.remove(F("/alarms.json"));
			} break;
		case 2: { // настройки бегущих строк
			LOG(println, PSTR("reset texts"));
			if(LittleFS.exists(F("/texts.json"))) LittleFS.remove(F("/texts.json"));
			} break;
		case 3: { // настройки цитат
			LOG(println, PSTR("reset quotes"));
			if(LittleFS.exists(F("/quote.json"))) LittleFS.remove(F("/quote.json"));
			} break;
		case 4: { // настройки сервера погоды
			LOG(println, PSTR("reset weather"));
			if(LittleFS.exists(F("/weather.json"))) LittleFS.remove(F("/weather.json"));
			} break;
		case 5: { // настройки Telegram/security
			LOG(println, PSTR("reset se"));
			if(LittleFS.exists(F("/security.json"))) LittleFS.remove(F("/security.json"));
			} break;
		case 6: { // журнал событий
			LOG(println, PSTR("erase logs"));
			char fileName[32];
			for(int8_t i=0; i<SEC_LOG_COUNT; i++) {
				sprintf_P(fileName, SEC_LOG_FILE, i);
				if( LittleFS.exists(fileName) ) LittleFS.remove(fileName);
			}
			} break;
		default: // такого раздела нет, просто перезагрузится
			break;
	}
}

void maintence() {
	if(is_no_auth()) return;
	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303); 
	String name = "t";
	if( HTTP.hasArg(name) ) {
		int t = constrain(HTTP.arg(name).toInt(), 0, 255);
		initRString(PSTR("Сброс"));
		if(t == 96) {
			// сброс всех настроек (кроме wifi)
			for(uint8_t i=0; i<=6; i++)
				reset_settings(i);
		} else if(t > 0) {
			// сброс конкретного раздела настроек
			reset_settings(t-1);
		}
		reboot_clock();
	}
}

// сохранение настроек будильника
void save_alarm() {
	if(is_no_auth()) return;
	need_save = false;
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
		set_simple_int(F("melody"), alarms[target].melody, 1, mp3_all);
		set_simple_string(F("text"), alarms[target].text);
		set_simple_int(F("color_mode"), alarms[target].color_mode, 0, 4);
		set_simple_color(F("color"), alarms[target].color);
	}
	HTTP.sendHeader(F("Location"),F("/alarms.html"));
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_alarms();
	if(fl_playStarted) {
		mp3_stop();
		fl_playStarted = false;
	}
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
			text_send(F("1"));
			initRString(PSTR("Будильник отключён"));
		}
	} else
		text_send(F("0"));
}

// сохранение настроек бегущей строки. Строки настраиваются по одной.
void save_text() {
	if(is_no_auth()) return;
	need_save = false;
	uint8_t target = 0;
	uint16_t settings = 512;
	String name = F("target");
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		set_simple_string(F("text"), texts[target].text);
		if( set_simple_int(F("period"), texts[target].period, 30, 3600) )
			textTimer[target].setInterval(texts[target].period*1000U);
		set_simple_int(F("color_mode"), texts[target].color_mode, 0, 5);
		set_simple_color(F("color"), texts[target].color);
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
	initRString(PSTR("Текст установлен"));
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
			text_send(F("1"));
			initRString(PSTR("Текст отключён"));
		}
	} else
		text_send(F("0"));
}

// костыль для настройки режима повтора. Работает 50/50
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
			fl_playStarted = true;
			break;
		case 2: // следующий трек
			t = mp3_current + 1;
			if(t>mp3_all) t=1;
			mp3_play(t);
			fl_playStarted = true;
			break;
		case 3: // играть
			repeat_mode(r);
			delay(10);
			mp3_play(c);
			fl_playStarted = true;
			break;
		case 4: // пауза
			mp3_pause();
			break;
		case 5: // режим
			repeat_mode(r);
			break;
		case 6: // остановить
			mp3_stop();
			fl_playStarted = false;
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
			// if(!mp3_isInit) mp3_init();
			// else mp3_reread();
			// if(mp3_isInit) mp3_update();
			mp3_reread();
			mp3_update();
			break;
	}
	char buff[20];
	sprintf_P(buff, PSTR("%i:%i:%i:%i"), mp3_current, mp3_all, cur_Volume, mp3_isPlay());
	text_send(buff);
}

// "proxy" отправка сообщений в телеграм через web запрос, для сторонних устройств
void send() {
	// if(is_no_auth()) return;
	String name = F("pin");
	if( HTTP.hasArg(name) ) {
		if( ts.pin_code == HTTP.arg(name) ) {
			name = F("msg");
			if( HTTP.hasArg(name) ) {
				tb_send_msg(HTTP.arg(name));
				text_send(F("1"));
				return;
			}
		}
	}
	text_send(F("0"));
}

// Установка времени. Для крайних случаев, когда интернет отсутствует
void set_clock() {
	if(is_no_auth()) return;
	uint8_t type=0;
	String name = "t";
	if(HTTP.hasArg(name)) {
		struct tm tm;
		type = HTTP.arg(name).toInt();
		if(type==0 || type==1) {
			name = F("time");
			if(HTTP.hasArg(name)) {
				size_t pos = HTTP.arg(name).indexOf(":");
				tm.tm_hour = constrain(HTTP.arg(name).toInt(), 0, 23);
				tm.tm_min = constrain(HTTP.arg(name).substring(pos+1).toInt(), 0, 59);
				name = F("date");
				if(HTTP.hasArg(name)) {
					size_t pos = HTTP.arg(name).indexOf("-");
					tm.tm_year = constrain(HTTP.arg(name).toInt()-1900, 0, 65000);
					tm.tm_mon = constrain(HTTP.arg(name).substring(pos+1).toInt()-1, 0, 11);
					size_t pos2 = HTTP.arg(name).substring(pos+1).indexOf("-");
					tm.tm_mday = constrain(HTTP.arg(name).substring(pos+pos2+2).toInt(), 1, 31);
					name = F("sec");
					if(HTTP.hasArg(name)) {
						tm.tm_sec = constrain(HTTP.arg(name).toInt()+1, 0, 60);
						tm.tm_isdst = gs.tz_dst;
						time_t t = mktime(&tm);
						LOG(printf_P,"web time: %llu\n",t);
						// set the system time
						timeval tv = { t, 0 };
						settimeofday(&tv, nullptr);
					}
				}
			}
		} else {
			syncTime();
		}
		HTTP.sendHeader(F("Location"),F("/maintenance.html"));
		HTTP.send(303);
		delay(1);
	} else {
		tm t = getTime();
		HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
		HTTP.client().printf_P(PSTR("\"time\":\"%u\","), t.tm_hour*60+t.tm_min);
		HTTP.client().printf_P(PSTR("\"date\":\"%u-%02u-%02u\"}"), t.tm_year +1900, t.tm_mon +1, t.tm_mday);
		#ifdef ESP8266
		HTTP.client().stop();
		#endif
	}
}

// Включение/выключение различных режимов
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
			if(a) {
				save_log_file(cond?SEC_TEXT_ENABLE:SEC_TEXT_DISABLE);
				save_config_security();
			}
		}
	}
	text_send(cond?F("1"):F("0"));
}

#ifdef ESP32
const char* print_full_platform_info(char* buf) {
	int p = 0;
	const char *cpu;
	switch (chip_info.model) {
		case 1: // ESP32
			cpu = "ESP32";
			break;
		case 2: // ESP32-S2
			cpu = "ESP32-S2";
			break;
		case 9: // ESP32-S3
			cpu = "ESP32-S4";
			break;
		case 5: // ESP32-C3
			cpu = "ESP32-C3";
			break;
		case 6: // ESP32-H2
			cpu = "ESP32-H2";
			break;
		default:
			cpu = "unknown";
	}
	p = sprintf(buf, "Chip:%s_r%u/", cpu, chip_info.revision);
	p += sprintf(buf+p, "Cores:%u/%s", chip_info.cores, ESP.getSdkVersion());
	return buf;
}

// декодирование информации о причине перезагрузки ядра
const char* print_reset_reason(char *buf) {
	int p = 0;
	uint8_t old_reason = 127;
	const char *res;
	for(int i=0; i<chip_info.cores; i++) {
		uint8_t reason = rtc_get_reset_reason(i);
		if( old_reason != reason ) {
			old_reason = reason;
			if( p ) p += sprintf(buf+p, ", ");
			switch ( reason ) {
				case 1 : res = "PowerON"; break;        	       /**<1, Vbat power on reset*/
				case 3 : res = "SW_RESET"; break;               /**<3, Software reset digital core*/
				case 4 : res = "OWDT_RESET"; break;             /**<4, Legacy watch dog reset digital core*/
				case 5 : res = "DeepSleep"; break;              /**<5, Deep Sleep reset digital core*/
				case 6 : res = "SDIO_RESET"; break;             /**<6, Reset by SLC module, reset digital core*/
				case 7 : res = "TG0WDT_SYS_RESET"; break;       /**<7, Timer Group0 Watch dog reset digital core*/
				case 8 : res = "TG1WDT_SYS_RESET"; break;       /**<8, Timer Group1 Watch dog reset digital core*/
				case 9 : res = "RTCWDT_SYS_RESET"; break;       /**<9, RTC Watch dog Reset digital core*/
				case 10 : res = "INTRUSION_RESET"; break;       /**<10, Intrusion tested to reset CPU*/
				case 11 : res = "TGWDT_CPU_RESET"; break;       /**<11, Time Group reset CPU*/
				case 12 : res = "SW_CPU_RESET"; break;          /**<12, Software reset CPU*/
				case 13 : res = "RTCWDT_CPU_RESET"; break;      /**<13, RTC Watch dog Reset CPU*/
				case 14 : res = "EXT_CPU_RESET"; break;         /**<14, for APP CPU, reseted by PRO CPU*/
				case 15 : res = "RTCWDT_BROWN_OUT"; break;	    /**<15, Reset when the vdd voltage is not stable*/
				case 16 : res = "RTCWDT_RTC_RESET"; break;      /**<16, RTC Watch dog reset digital core and rtc module*/
				default : res = "NO_MEAN";
			}
			p += sprintf(buf+p, "%s", res);
		}
	}
	return buf;
}
#endif

// Информация о состоянии железки
void sysinfo() {
	if(is_no_auth()) return;
	char buf[100];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
	HTTP.client().printf_P(PSTR("\"Uptime\":\"%s\","), getUptime(buf));
	HTTP.client().printf_P(PSTR("\"Time\":\"%s\","), clockCurrentText(buf));
	HTTP.client().printf_P(PSTR("\"Date\":\"%s\","), dateCurrentTextLong(buf));
	HTTP.client().printf_P(PSTR("\"Sunrise\":\"%u:%02u\","), sunrise / 60, sunrise % 60);
	HTTP.client().printf_P(PSTR("\"Sunset\":\"%u:%02u\","), sunset / 60, sunset % 60);
	HTTP.client().printf_P(PSTR("\"Illumination\":%i,"), analogRead(PIN_PHOTO_SENSOR));
	HTTP.client().printf_P(PSTR("\"LedBrightness\":%i,"), led_brightness);
	HTTP.client().printf_P(PSTR("\"fl_5v\":%i,"), fl_5v);
	HTTP.client().printf_P(PSTR("\"Rssi\":%i,"), wifi_rssi());
	HTTP.client().printf_P(PSTR("\"FreeHeap\":%i,"), ESP.getFreeHeap());
	#ifdef ESP32
	HTTP.client().printf("\"MaxFreeBlockSize\":%i,", ESP.getMaxAllocHeap());
	HTTP.client().printf("\"HeapFragmentation\":%i,", 100-ESP.getMaxAllocHeap()*100/ESP.getFreeHeap());
	HTTP.client().printf("\"ResetReason\":\"%s\",", print_reset_reason(buf));
	HTTP.client().printf("\"FullVersion\":\"%s\",", print_full_platform_info(buf));
	#else // ESP8266
	HTTP.client().printf_P(PSTR("\"MaxFreeBlockSize\":%i,"), ESP.getMaxFreeBlockSize());
	HTTP.client().printf_P(PSTR("\"HeapFragmentation\":%i,"), ESP.getHeapFragmentation());
	HTTP.client().printf_P(PSTR("\"ResetReason\":\"%s\","), ESP.getResetReason().c_str());
	HTTP.client().printf_P(PSTR("\"FullVersion\":\"%s\","), ESP.getFullVersion().c_str());
	#endif
	HTTP.client().printf_P(PSTR("\"CpuFreqMHz\":%i,"), ESP.getCpuFreqMHz());
	HTTP.client().printf_P(PSTR("\"BuildTime\":\"%s %s\"}"), F(__DATE__), F(__TIME__));
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}

cur_sensor sensor[MAX_SENSORS];

// Информация о зарегистрированных сенсорах
void sensors() {
	if(is_no_auth()) return;
	bool fl = false;
	char buf[100];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n["));
	for(uint8_t i=0; i<MAX_SENSORS; i++) {
		if(sensor[i].registered >= getTimeU() - ts.sensor_timeout*60 + 60) {
			if(fl) HTTP.client().print(",");
			HTTP.client().printf_P(PSTR("{\"num\":%i,"), i);
			HTTP.client().printf_P(PSTR("\"hostname\":\"%s\","), jsonEncode(buf, sensor[i].hostname.c_str(), sizeof(buf)));
			HTTP.client().printf_P(PSTR("\"ip\":\"%s\","), sensor[i].ip.toString().c_str());
			HTTP.client().printf_P(PSTR("\"timeout\":%i}"), ts.sensor_timeout*60 + sensor[i].registered - getTimeU());
			fl = true;
		}
	}
	HTTP.client().print("]");
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}

// Регистрация сенсора
void registration() {
	// Авторизация примитивная, через shared key, или pin
	// должно быть всего два параметра: pin (shared key) и name - имя устройства
	String name = F("pin");
	if( HTTP.hasArg(name) ) {
		if( ts.pin_code == HTTP.arg(name) ) {
			// pin подошел
			name = F("name");
			if( HTTP.hasArg(name) ) {
				IPAddress sensor_ip = HTTP.client().remoteIP();
				bool fl_found = false;
				// проверка, зарегистрирован ли уже этот ip
				for(uint8_t i=0; i<MAX_SENSORS; i++) {
					if(sensor[i].ip == sensor_ip) {
						// надо ли обновить hostname
						if(sensor[i].hostname != HTTP.arg(name))
							sensor[i].hostname = HTTP.arg(name);
						// обновить время регистрации
						sensor[i].registered = getTimeU();
						fl_found = true;
						break;
					}
				}
				if(!fl_found) {
					// новый сенсор, поиск свободной ячейки
					for(uint8_t i=0; i<MAX_SENSORS; i++) {
						if(sensor[i].registered < getTimeU() - ts.sensor_timeout*60 - 60) {
							// найдена свободная ячейка
							sensor[i].hostname = HTTP.arg(name);
							sensor[i].ip = sensor_ip;
							sensor[i].registered = getTimeU();
							fl_found = true;
							break;
						}
					}
				}
				if(fl_found) {
					LOG(printf_P,PSTR("registered \"%s\" ip %s\n"), HTTP.arg(name).c_str(), sensor_ip.toString().c_str());
					text_send(F("1"));
					return;
				}
			}
		}
	}
	// любая ошибка, в том числе закончились свободные ячейки
	text_send(F("0"));
}

// сохранение настроек цитат
void save_quote() {
	if(is_no_auth()) return;
	need_save = false;
	bool fl_change_color = false;

	if(set_simple_checkbox(F("enabled"), qs.enabled)) {
		// если цитаты отключили, то сбросить текущую цитату 
		if( qs.enabled == 0 ) messages[MESSAGE_QUOTE].count = 0;
	}
	if(set_simple_int(F("period"), qs.period, 120, 3600))
		messages[MESSAGE_QUOTE].timer.setInterval(1000U * qs.period);
	if(set_simple_int(F("update"), qs.update, 0, 3))
		quoteUpdateTimer.setInterval(900000U * (qs.update+1));
	if(set_simple_int(F("color_mode"), qs.color_mode, 0, 4))
		fl_change_color = true;
	if(set_simple_color(F("color"), qs.color))
		fl_change_color = true;
	set_simple_int(F("server"), qs.server, 0, 2);
	set_simple_int(F("lang"), qs.lang, 0, 3);
	set_simple_string(F("url"), qs.url);
	set_simple_string(F("params"), qs.params);
	set_simple_int(F("method"), qs.method, 0, 1);
	set_simple_int(F("type"), qs.type, 0, 2);
	set_simple_string(F("quote_field"), qs.quote_field);
	set_simple_string(F("author_field"), qs.author_field);

	if(fl_change_color) messages[MESSAGE_QUOTE].color = qs.color_mode > 0 ? qs.color_mode: qs.color;

	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303);
	delay(1);
	if( need_save ) {
		save_config_quote();
		quote.fl_init = false;
	}
	initRString(PSTR("Настройки сохранены"));
}

void show_quote() {
	if(is_no_auth()) return;
	text_send(messages[MESSAGE_QUOTE].text);
}

const char help[] PROGMEM = R"""(
(h)elp - this help
(m)sg="text" or (t)ext="text" - show this "text" on the matrix
(c)nt=5 - show 5 times (2 by default, max 100)
(i)nt=60 - show with interval 60 seconds (30 by default, max 600)
(d)ue=1 - color mode (1-rainbow or 2-different) or color in hex (FFFFFF or FFF)
)""";

void show() {
	bool fl_web = false;
	bool fl_msg = false;
	bool fl_cnt = false;
	bool fl_int = false;
	bool fl_due = false;
	bool cond = false;
	int num_args = HTTP.args();
	if(num_args==0) {
		HTTP.send_P(200, PSTR("text/plain"), help);
		LOG(println, F("show: send help"));
		return;
	}
	for(int i=0; i<num_args; i++) {
		String arg_name = HTTP.argName(i);
		if(arg_name.startsWith("h")) {
			HTTP.send_P(200, PSTR("text/plain"), help);
			LOG(println, F("show: send help"));
		}
		if(arg_name.startsWith("m") || arg_name.startsWith("t")) {
			messages[MESSAGE_WEB].text = HTTP.arg(i);
			if(messages[MESSAGE_WEB].text.length() > 1) fl_msg = true;
		}
		if(arg_name.startsWith("c")) {
			messages[MESSAGE_WEB].count = constrain(HTTP.arg(i).toInt(), 1, 100);
			fl_cnt = true;
		}
		if(arg_name.startsWith("i")) {
			messages[MESSAGE_WEB].timer.setInterval(constrain(HTTP.arg(i).toInt()*1000, 15000, 600000));
			fl_int = true;
		}
		if(arg_name.startsWith("d")) {
			int color = HTTP.arg(i).toInt();
			if( color > 0 && color < 6 ) {
				messages[MESSAGE_WEB].color = color;
			} else {
				messages[MESSAGE_WEB].color = text_to_color(HTTP.arg(i).c_str());
			}
			fl_due = true;
		}
		if(arg_name.startsWith("w")) {
			fl_web = true;
		}
	}
	if( fl_msg ) {
		if( ! fl_cnt ) messages[MESSAGE_WEB].count = 2;
		if( ! fl_int ) messages[MESSAGE_WEB].timer.setInterval(30000);
		if( ! fl_due ) messages[MESSAGE_WEB].color = 1;
		cond = true;
	} else
		messages[MESSAGE_WEB].count = 0;
	if( fl_web ) {
		HTTP.sendHeader(F("Location"),"/maintenance.html");
		HTTP.send(303);
		delay(1);
	} else
		text_send(cond?F("1"):F("0"));
}

void save_weather() {
	if(is_no_auth()) return;
	need_save = false;
	bool need_weather = false;
	bool fl_change_color = false;

	need_weather = set_simple_checkbox(F("weather"), ws.weather);
	if(set_simple_int(F("sync_weather_period"), ws.sync_weather_period, 15, 1439))
		syncWeatherTimer.setInterval(60000U * ws.sync_weather_period);
	if(set_simple_int(F("show_weather_period"), ws.show_weather_period, 30, 3600))
		messages[MESSAGE_WEATHER].timer.setInterval(1000U * ws.show_weather_period);
	if(set_simple_int(F("color_mode"), ws.color_mode, 0, 4))
		fl_change_color = true;
	if(set_simple_color(F("color"), ws.color))
		fl_change_color = true;
	set_simple_checkbox(F("weather_code"), ws.weather_code);
	set_simple_checkbox(F("temperature"), ws.temperature);
	set_simple_checkbox(F("a_temperature"), ws.a_temperature);
	set_simple_checkbox(F("humidity"), ws.humidity);
	set_simple_checkbox(F("cloud"), ws.cloud);
	set_simple_checkbox(F("pressure"), ws.pressure);
	set_simple_checkbox(F("wind_speed"), ws.wind_speed);
	set_simple_checkbox(F("wind_direction"), ws.wind_direction);
	set_simple_checkbox(F("wind_direction2"), ws.wind_direction2);
	set_simple_checkbox(F("wind_gusts"), ws.wind_gusts);
	set_simple_checkbox(F("pressure_dir"), ws.pressure_dir);
	set_simple_checkbox(F("forecast"), ws.forecast);

	if(fl_change_color) messages[MESSAGE_WEATHER].color = ws.color_mode > 0 ? ws.color_mode: ws.color;

	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303);
	delay(1);
	if( need_save ) {
		save_config_weather();
		if( ws.weather ) {
			if( need_weather ) syncWeatherTimer.setReady();
			char txt[512];
			messages[MESSAGE_WEATHER].text = String(generate_weather_string(txt));
		} else {
			messages[MESSAGE_WEATHER].count = 0;
		}
	}
	initRString(PSTR("Настройки сохранены"));
}

void show_weather() {
	if(is_no_auth()) return;
	char txt[512];
	text_send(String(generate_weather_string(txt)));
}

