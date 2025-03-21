/*
	Работа с настройками.
	Инициализация по умолчанию, чтение из файла, сохранение в файл
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "defines.h"
#include "settings.h"
#include "ntp.h"

Global_Settings gs;
Telegram_Settings ts;

cur_alarm alarms[MAX_ALARMS];
cur_text texts[MAX_RUNNING];

uint8_t sec_enable = 0;
uint8_t sec_curFile = 0;

Quote_Settings qs;
Weather_Settings ws;


byte char_to_byte(char n) {
	if(n>='0' && n<='9') return (byte)n - 48;
	if(n>='A' && n<='F') return (byte)n - 55;
	if(n>='a' && n<='f') return (byte)n - 87;
	return 48;
}

uint32_t text_to_color(const char *s) {
	if( s == nullptr ) return 0xffffff;
	uint32_t c = 0;
	int8_t i = 0;
	byte t = 0;
	char a[7];
	for(i=0; i<(int8_t)strlen(s); i++ ) {
		if( c==6 ) break;
		if( isxdigit(s[i]) ) a[c++] = s[i];
	}
	if(c<3) for(i=c; i>3; i++) a[i] = 'f';
	a[6] = '\0';
	c = 0;
	if(strlen(a)<6) {
		for(i=0; i<3; i++) {
			t = char_to_byte(a[2-i]);
			c |= (t << (i*8)) | (t << (i*8+4)); 
		}
	} else {
		for(i=0; i<6; i++) {
			t = char_to_byte(a[5-i]);
			c |= t << (i*4);
		}
	}
	return c;
}

String color_to_text(uint32_t c) {
	char a[] = "#ffffff";
	byte t = 0;
	for(int8_t i=1; i<=6; i++) {
		t = (byte)((c >> ((6-i)*4)) & 0xF);
		t += t<10? 48: 87;
		a[i] = (char)t;
	}
	return String(a);
}

bool load_config_main() {

	File configFile = LittleFS.open(F("/config.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open main config file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	// Fetch values.
	// const char* sensor = doc["sensor"];
	// long time = doc["time"];
	// double latitude = doc["data"][0];
	// double longitude = doc["data"][1];

	gs.str_hello = doc[F("str_hello")].as<String>();
	gs.max_alarm_time = doc[F("max_alarm_time")];
	gs.run_allow = doc[F("run_allow")];
	gs.run_begin = doc[F("run_begin")];
	gs.run_end = doc[F("run_end")];
	gs.wide_font = doc[F("wide_font")];
	gs.show_move = doc[F("show_move")];
	gs.delay_move = doc[F("delay_move")];
	gs.max_move = doc[F("max_move")];
	gs.tz_shift = doc[F("tz_shift")];
	gs.tz_dst = doc[F("tz_dst")];
	gs.tz_adjust = doc[F("tz_adjust")];
	gs.tiny_clock = doc[F("tiny_clock")];
	gs.dots_style = doc[F("dots_style")];
	gs.t12h = doc[F("t12h")];
	gs.show_date_short = doc[F("date_short")];
	gs.tiny_date = doc[F("tiny_date")];
	gs.show_date_period = doc[F("date_period")]; clockDate.setInterval(1000U * gs.show_date_period);
	gs.show_time_color = doc[F("time_color")];
	gs.show_time_color0 = text_to_color(doc[F("time_color0")]);
	gs.show_time_col[0] = text_to_color(doc[F("time_color1")]);
	gs.show_time_col[1] = gs.show_time_col[0];
	// gs.show_time_col[1] = text_to_color(doc[F("time_color2")]);
	gs.show_time_col[2] = text_to_color(doc[F("time_color3")]);
	gs.show_time_col[5] = gs.show_time_col[2];
	gs.show_time_col[3] = text_to_color(doc[F("time_color4")]);
	gs.show_time_col[4] = gs.show_time_col[3];
	// gs.show_time_col[4] = text_to_color(doc[F("time_color5")]);
	gs.show_time_col[6] = text_to_color(doc[F("time_color6")]);
	gs.show_time_col[7] = gs.show_time_col[6];
	gs.show_date_color = doc[F("date_color")];
	gs.show_date_color0 = text_to_color(doc[F("date_color0")]);
	gs.hue_shift = doc[F("hue_shift")];
	gs.bright_mode = doc[F("bright_mode")];
	gs.bright0 = doc[F("bright0")];
	gs.bright_boost = doc[F("br_boost")];
	if(gs.bright_mode==2) set_brightness(gs.bright0);
	gs.boost_mode = doc[F("boost_mode")];
	gs.bright_add = doc[F("br_add")];
	gs.latitude = doc[F("latitude")];
	gs.longitude = doc[F("longitude")];
	gs.bright_begin = doc[F("br_begin")];
	gs.bright_end = doc[F("br_end")];
	gs.max_power = doc[F("max_power")];
	if(DEFAULT_POWER > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, gs.max_power);
	gs.turn_display = doc[F("turn_display")];
	gs.volume_start = doc[F("volume_start")];
	gs.volume_finish = doc[F("volume_finish")];
	gs.volume_period = doc[F("volume_period")]; alarmStepTimer.setInterval(1000U * gs.volume_period);
	gs.timeout_mp3 = doc[F("timeout_mp3")]; timeoutMp3Timer.setInterval(3600000U * gs.sync_time_period);
	gs.sync_time_period = doc[F("sync_time_period")]; ntpSyncTimer.setInterval(3600000U * gs.sync_time_period);
	gs.scroll_period = doc[F("scroll_period")]; scrollTimer.setInterval(60 - gs.scroll_period);
	gs.slide_show = doc[F("slide_show")];
	gs.minim_show = doc[F("minim_show")];
	gs.web_login = doc[F("web_login")].as<String>();
	gs.web_password = doc[F("web_password")].as<String>();

	LOG(println, PSTR("Main config loaded."));
	return true;
}

void save_config_main() {

	JsonDocument doc; // временный буфер под объект json

	doc[F("str_hello")] = gs.str_hello;
	doc[F("max_alarm_time")] = gs.max_alarm_time;
	doc[F("run_allow")] = gs.run_allow;
	doc[F("run_begin")] = gs.run_begin;
	doc[F("run_end")] = gs.run_end;
	doc[F("wide_font")] = gs.wide_font;
	doc[F("show_move")] = gs.show_move;
	doc[F("delay_move")] = gs.delay_move;
	doc[F("max_move")] = gs.max_move;
	doc[F("tz_shift")] = gs.tz_shift;
	doc[F("tz_dst")] = gs.tz_dst;
	doc[F("tz_adjust")] = gs.tz_adjust;
	doc[F("tiny_clock")] = gs.tiny_clock;
	doc[F("dots_style")] = gs.dots_style;
	doc[F("t12h")] = gs.t12h;
	doc[F("date_short")] = gs.show_date_short;
	doc[F("tiny_date")] = gs.tiny_date;
	doc[F("date_period")] = gs.show_date_period;
	doc[F("time_color")] = gs.show_time_color;
	doc[F("time_color0")] = color_to_text(gs.show_time_color0);
	doc[F("time_color1")] = color_to_text(gs.show_time_col[0]);
	// doc[F("time_color2")] = color_to_text(gs.show_time_col[1]);
	doc[F("time_color3")] = color_to_text(gs.show_time_col[2]);
	doc[F("time_color4")] = color_to_text(gs.show_time_col[3]);
	// doc[F("time_color5")] = color_to_text(gs.show_time_col[4]);
	doc[F("time_color6")] = color_to_text(gs.show_time_col[6]);
	doc[F("date_color")] = gs.show_date_color;
	doc[F("date_color0")] = color_to_text(gs.show_date_color0);
	doc[F("hue_shift")] = gs.hue_shift;
	doc[F("bright_mode")] = gs.bright_mode;
	doc[F("bright0")] = gs.bright0;
	doc[F("br_boost")] = gs.bright_boost;
	doc[F("boost_mode")] = gs.boost_mode;
	doc[F("br_add")] = gs.bright_add;
	doc[F("latitude")] = gs.latitude;
	doc[F("longitude")] = gs.longitude;
	doc[F("br_begin")] = gs.bright_begin;
	doc[F("br_end")] = gs.bright_end;
	doc[F("max_power")] = gs.max_power;
	doc[F("turn_display")] = gs.turn_display;
	doc[F("volume_start")] = gs.volume_start;
	doc[F("volume_finish")] = gs.volume_finish;
	doc[F("volume_period")] = gs.volume_period;
	doc[F("timeout_mp3")] = gs.timeout_mp3;
	doc[F("sync_time_period")] = gs.sync_time_period;
	doc[F("scroll_period")] = gs.scroll_period;
	doc[F("slide_show")] = gs.slide_show;
	doc[F("minim_show")] = gs.minim_show;
	doc[F("web_login")] = gs.web_login;
	doc[F("web_password")] = gs.web_password;

	File configFile = LittleFS.open(F("/config.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(4);

	LOG(println, PSTR("Main config saved."));
}

bool load_config_telegram() {

	File configFile = LittleFS.open(F("/telegram.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open telegram config file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	ts.use_move = doc[F("use_move")];
	ts.use_brightness = doc[F("use_brightness")];
	ts.pin_code = doc[F("pin_code")].as<String>();
	ts.clock_name = doc[F("clock_name")].as<String>();
	ts.sensor_timeout = doc[F("sensor_timeout")];
	ts.tb_name = doc[F("tb_name")].as<String>();
	ts.tb_chats = doc[F("tb_chats")].as<String>();
	ts.tb_token = doc[F("tb_token")].as<String>();
	ts.tb_secret = doc[F("tb_secret")].as<String>();
	ts.tb_rate = doc[F("tb_rate")];
	ts.tb_accelerated = doc[F("tb_accelerated")];
	telegramTimer.setInterval(1000U * ts.tb_accelerated);
	ts.tb_accelerate = doc[F("tb_accelerate")];
	ts.tb_ban = doc[F("tb_ban")];

	LOG(println, PSTR("Telegram config loaded."));
	return true;
}

void save_config_telegram() {

	JsonDocument doc; // временный буфер под объект json

	doc[F("use_move")] = ts.use_move;
	doc[F("use_brightness")] = ts.use_brightness;
	doc[F("pin_code")] = ts.pin_code;
	doc[F("clock_name")] = ts.clock_name;
	doc[F("sensor_timeout")] = ts.sensor_timeout;
	doc[F("tb_name")] = ts.tb_name;
	doc[F("tb_chats")] = ts.tb_chats;
	doc[F("tb_token")] = ts.tb_token;
	doc[F("tb_secret")] = ts.tb_secret;
	doc[F("tb_rate")] = ts.tb_rate;
	doc[F("tb_accelerated")] = ts.tb_accelerated;
	doc[F("tb_accelerate")] = ts.tb_accelerate;
	doc[F("tb_ban")] = ts.tb_ban;

	File configFile = LittleFS.open(F("/telegram.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(2);

	LOG(println, PSTR("Telegram config saved."));
}

bool load_config_alarms() {

	File configFile = LittleFS.open(F("/alarms.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open config for alarms file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();
	
	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	for( int i=0; i<min(MAX_ALARMS,(int)doc.size()); i++) {
		alarms[i].settings = doc[i]["s"];
		alarms[i].hour = doc[i]["h"];
		alarms[i].minute = doc[i]["m"];
		alarms[i].melody = doc[i]["me"];
		alarms[i].text = doc[i]["t"].as<String>();
		alarms[i].color_mode = doc[i]["cm"];
		alarms[i].color = text_to_color(doc[i]["c"]);
	}

	LOG(println, PSTR("Alarms config loaded."));
	return true;
}

void save_config_alarms() {

	JsonDocument doc; // временный буфер под объект json

	for( int i=0; i<MAX_ALARMS; i++) {
		doc[i]["s"] = alarms[i].settings;
		doc[i]["h"] = alarms[i].hour;
		doc[i]["m"] = alarms[i].minute;
		doc[i]["me"] = alarms[i].melody;
		doc[i]["t"] = alarms[i].text;
		doc[i]["cm"] = alarms[i].color_mode;
		doc[i]["c"] = color_to_text(alarms[i].color);
	}

	File configFile = LittleFS.open(F("/alarms.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(2);

	LOG(println, PSTR("Alarms config saved."));
}

bool load_config_texts() {

	File configFile = LittleFS.open(F("/texts.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open config for texts file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	for( int i=0; i<min(MAX_RUNNING,(int)doc.size()); i++) {
		texts[i].text = doc[i]["t"].as<String>();
		texts[i].color_mode = doc[i]["cm"];
		texts[i].color = text_to_color(doc[i]["c"]);
		texts[i].period = doc[i]["p"];
		texts[i].repeat_mode = doc[i]["r"];
		// сразу установка таймера
		textTimer[i].setInterval(texts[i].period*1000U);
	}

	LOG(println, PSTR("Texts config loaded."));
	return true;
}

void save_config_texts() {

	JsonDocument doc; // временный буфер под объект json

	for( int i=0; i<MAX_RUNNING; i++) {
		doc[i]["t"] = texts[i].text;
		doc[i]["cm"] = texts[i].color_mode;
		doc[i]["c"] = color_to_text(texts[i].color);
		doc[i]["p"] = texts[i].period;
		doc[i]["r"] = texts[i].repeat_mode;
	}

	File configFile = LittleFS.open(F("/texts.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing (texts)"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(4);

	LOG(println, PSTR("Texts config saved."));
}

bool load_config_security() {

	File configFile = LittleFS.open(F("/security.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open config for texts file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	sec_enable = doc[F("sec_enable")];
	sec_curFile = doc[F("sec_curFile")];

	LOG(println, PSTR("Security config loaded."));
	return true;
}

void save_config_security() {

	JsonDocument doc; // временный буфер под объект json

	doc[F("sec_enable")] = sec_enable;
	doc[F("sec_curFile")] = sec_curFile;
	doc[F("logs_count")] = SEC_LOG_COUNT;

	File configFile = LittleFS.open(F("/security.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing (texts)"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush(); // подождать, пока данные запишутся. Хотя close должен делать это сам, но без иногда перезагружается.
	configFile.close(); // не забыть закрыть файл
	delay(2);

	LOG(println, PSTR("Security config saved."));
}

// чтение последних cnt строк лога
String read_log_file(int16_t cnt) {

	// всего надо отдать cnt последних строк.
	// Если файл только начал писаться, то надо показать последние записи предыдущего файла
	// сначала считывается предыдущий файл
	int16_t cur = 0;
	int16_t aCnt = 0;
	char aStr[cnt][SEC_LOG_MAX];
	uint8_t prevFile = sec_curFile > 0 ? sec_curFile-1: SEC_LOG_COUNT-1; // вычисление предыдущего лог-файла
	char fileName[32];
	sprintf_P(fileName, SEC_LOG_FILE, prevFile);
	File logFile = LittleFS.open(fileName, "r");
	if(logFile) {
		while(logFile.available()) {
			strncpy(aStr[cur], logFile.readStringUntil('\n').c_str(), SEC_LOG_MAX); // \r\n
			cur = (cur+1) % cnt;
			aCnt++;
		}
	}
	logFile.close();
	// теперь считывается текущий файл
	sprintf_P(fileName, SEC_LOG_FILE, sec_curFile);
	logFile = LittleFS.open(fileName, "r");
	if(logFile) {
		while(logFile.available()) {
			strncpy(aStr[cur], logFile.readStringUntil('\n').c_str(), SEC_LOG_MAX);
			cur = (cur+1) % cnt;
			aCnt++;
		}
	}
	logFile.close();
	// теперь надо склеить массив в одну строку и отдать назад
	String str = "";
	char *ptr;
	for(int16_t i = min(aCnt,cnt); i > 0; i--) {
		cur = cur > 0 ? cur-1: cnt-1;
		ptr = aStr[cur] + strlen(aStr[cur]) - 1;
		strcpy(ptr, "\n"); // замена последнего символа на url-код склейки строк (%0A)
		str += aStr[cur];
	}
	return str;
}

void save_log_file(uint8_t mt) {

	char fileName[32];
	sprintf_P(fileName, SEC_LOG_FILE, sec_curFile);
	File logFile = LittleFS.open(fileName, "a");
	if (!logFile) {
		// не получилось открыть файл на дополнение
		LOG(println, PSTR("Failed to open log file"));
		return;
	}
	// проверка, не превышен ли лимит размера файла, если да, то открыть второй файл.
	size_t size = logFile.size();
	if (size > SEC_LOG_SIZE) {
		LOG(println, PSTR("Log file size is too large, switch file"));
		logFile.close();
		sec_curFile = (sec_curFile+1) % SEC_LOG_COUNT;
		save_config_security();
		sprintf_P(fileName, SEC_LOG_FILE, sec_curFile);
		logFile = LittleFS.open(fileName, "w");
		if (!logFile) {
			// ошибка создания файла
			LOG(println, PSTR("Failed to open new log file"));
			return;
		}
	}
	// составление строки которая будет занесена в файл
	char str[SEC_LOG_MAX];
	tm t = getTime();
	const char *lm = nullptr;
	switch (mt)	{
		case SEC_TEXT_DISABLE: lm = PSTR("Stop."); break;
		case SEC_TEXT_ENABLE: lm = PSTR("Start."); break;
		case SEC_TEXT_MOVE: lm = PSTR("Move!"); break;
		case SEC_TEXT_BRIGHTNESS: lm = PSTR("Brightness!"); break;
		case SEC_TEXT_BOOT: lm = PSTR("Boot clock"); break;
		case SEC_TEXT_POWERED: lm = PSTR("Power is ON"); break;
		case SEC_TEXT_POWEROFF: lm = PSTR("Power is OFF"); break;
	}
	snprintf_P(str, SEC_LOG_MAX, PSTR("%04u-%02u-%02u %02u:%02u:%02u : %s"), t.tm_year +1900, t.tm_mon +1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, lm);

	LOG(println, str);
	logFile.println(str);

	logFile.flush();
	logFile.close();
	delay(2);
}

// чтение конфигурации сервера цитат
bool load_config_quote() {

	File configFile = LittleFS.open(F("/quote.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open quote config file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	qs.enabled = doc[F("enabled")];
	qs.period = doc[F("period")];
	qs.update = doc[F("update")];
	qs.color_mode = doc[F("color_mode")];
	qs.color = text_to_color(doc[F("color")]);
	qs.server = doc[F("server")];
	qs.lang = doc[F("lang")];
	qs.url = doc[F("url")].as<String>();
	qs.params = doc[F("params")].as<String>();
	qs.method = doc[F("method")];
	qs.type = doc[F("type")];
	qs.quote_field = doc[F("quote_field")].as<String>();
	qs.author_field = doc[F("author_field")].as<String>();

	quoteUpdateTimer.setInterval(900000U * (qs.update+1));
	messages[MESSAGE_QUOTE].timer.setInterval(1000U * qs.period);
	return true;
}

// сохранение настроек сервера цитат
void save_config_quote() {

	JsonDocument doc; // временный буфер под объект json

	doc[F("enabled")] = qs.enabled;
	doc[F("period")] = qs.period;
	doc[F("update")] = qs.update;
	doc[F("color_mode")] = qs.color_mode;
	doc[F("color")] = color_to_text(qs.color);
	doc[F("server")] = qs.server;
	doc[F("lang")] = qs.lang;
	doc[F("url")] = qs.url;
	doc[F("params")] = qs.params;
	doc[F("method")] = qs.method;
	doc[F("type")] = qs.type;
	doc[F("quote_field")] = qs.quote_field;
	doc[F("author_field")] = qs.author_field;

	File configFile = LittleFS.open(F("/quote.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(4);
}

// чтение конфигурации сервера погоды
bool load_config_weather() {

	File configFile = LittleFS.open(F("/weather.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open weather config file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	ws.sensors = doc[F("sensors")];
	ws.term_period = doc[F("term_period")];
	ws.term_color_mode = doc[F("term_color_mode")];
	ws.term_color = text_to_color(doc[F("term_color")]);
	ws.tiny_term = doc[F("tiny_term")];
	ws.term_cor = doc[F("term_cor")];
	ws.bar_cor = doc[F("bar_cor")];
	ws.term_pool = doc[F("term_pool")];
	ws.weather = doc[F("weather")];
	ws.sync_weather_period = doc[F("sync_weather_period")];
	ws.show_weather_period = doc[F("show_weather_period")];
	ws.color_mode = doc[F("color_mode")];
	ws.color = text_to_color(doc[F("color")]);
	ws.weather_code = doc[F("weather_code")];
	ws.temperature = doc[F("temperature")];
	ws.a_temperature = doc[F("a_temperature")];
	ws.humidity = doc[F("humidity")];
	ws.cloud = doc[F("cloud")];
	ws.pressure = doc[F("pressure")];
	ws.wind_speed = doc[F("wind_speed")];
	ws.wind_direction = doc[F("wind_direction")];
	ws.wind_direction2 = doc[F("wind_direction2")];
	ws.wind_gusts = doc[F("wind_gusts")];
	ws.pressure_dir = doc[F("pressure_dir")];
	ws.forecast = doc[F("forecast")];

	syncWeatherTimer.setInterval(60000U * ws.sync_weather_period);
	messages[MESSAGE_WEATHER].timer.setInterval(1000U * ws.show_weather_period);
	return true;
}

// сохранение настроек сервера погоды
void save_config_weather() {

	JsonDocument doc; // временный буфер под объект json

	doc[F("sensors")] = ws.sensors;
	doc[F("term_period")] = ws.term_period;
	doc[F("term_color_mode")] = ws.term_color_mode;
	doc[F("term_color")] = color_to_text(ws.term_color);
	doc[F("tiny_term")] = ws.tiny_term;
	doc[F("term_cor")] = ws.term_cor;
	doc[F("bar_cor")] = ws.bar_cor;
	doc[F("term_pool")] = ws.term_pool;
	doc[F("weather")] = ws.weather;
	doc[F("sync_weather_period")] = ws.sync_weather_period;
	doc[F("show_weather_period")] = ws.show_weather_period;
	doc[F("color_mode")] = ws.color_mode;
	doc[F("color")] = color_to_text(ws.color);
	doc[F("weather_code")] = ws.weather_code;
	doc[F("temperature")] = ws.temperature;
	doc[F("a_temperature")] = ws.a_temperature;
	doc[F("humidity")] = ws.humidity;
	doc[F("cloud")] = ws.cloud;
	doc[F("pressure")] = ws.pressure;
	doc[F("wind_speed")] = ws.wind_speed;
	doc[F("wind_direction")] = ws.wind_direction;
	doc[F("wind_direction2")] = ws.wind_direction2;
	doc[F("wind_gusts")] = ws.wind_gusts;
	doc[F("pressure_dir")] = ws.pressure_dir;
	doc[F("forecast")] = ws.forecast;

	File configFile = LittleFS.open(F("/weather.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(4);
}
