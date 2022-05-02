/*
	Работа с настройкаи.
	Инициализация по умолчанию, чтение из файла, сохранение в файл
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "settings_init.h"
#include "settings.h"
#include "ntp.h"

byte char_to_byte(char n) {
	if(n>='0' && n<='9') return (byte)n - 48;
	if(n>='A' && n<='F') return (byte)n - 55;
	if(n>='a' && n<='f') return (byte)n - 87;
	return 48;
}

uint32_t text_to_color(const char *s) {
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

void load_config_main() {
	StaticJsonDocument<1536> doc; // временный буфер под объект json

	File configFile = LittleFS.open(F("/config.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open main config file"));
		//  Создаем файл запив в него данные по умолчанию
		save_config_main();
		return;
	}

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return;
	}

	// Fetch values.
	// const char* sensor = doc["sensor"];
	// long time = doc["time"];
	// double latitude = doc["data"][0];
	// double longitude = doc["data"][1];

	str_hello = doc[F("str_hello")].as<String>();
	max_alarm_time = doc[F("max_alarm_time")];
	run_allow = doc[F("run_allow")];
	run_begin = doc[F("run_begin")];
	run_end = doc[F("run_end")];
	wide_font = doc[F("wide_font")];
	show_move = doc[F("show_move")];
	delay_move = doc[F("delay_move")];
	tz_shift = doc[F("tz_shift")];
	tz_dst = doc[F("tz_dst")];
	show_date_short = doc[F("date_short")];
	show_date_period = doc[F("date_period")]; clockDate.setInterval(1000U * show_date_period);
	show_time_color = doc[F("time_color")];
	show_time_color0 = text_to_color(doc[F("time_color0")]);
	show_time_col[0] = text_to_color(doc[F("time_color1")]);
	show_time_col[1] = text_to_color(doc[F("time_color2")]);
	show_time_col[2] = text_to_color(doc[F("time_color3")]);
	show_time_col[3] = text_to_color(doc[F("time_color4")]);
	show_time_col[4] = text_to_color(doc[F("time_color5")]);
	show_date_color = doc[F("date_color")];
	show_date_color0 = text_to_color(doc[F("date_color0")]);
	bright_mode = doc[F("bright_mode")];
	bright0 = doc[F("bright0")];
	br_boost = doc[F("br_boost")];
	if(bright_mode==2) set_brightness(bright0);
	max_power = doc[F("max_power")];
	if(DEFAULT_POWER > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, max_power);
	turn_display = doc[F("turn_display")];
	volume_start = doc[F("volume_start")];
	volume_finish = doc[F("volume_finish")];
	volume_period = doc[F("volume_period")]; alarmStepTimer.setInterval(1000U * volume_period);
	sync_time_period = doc[F("sync_time_period")]; ntpSyncTimer.setInterval(60000U * sync_time_period);
	scroll_period = doc[F("scroll_period")]; scrollTimer.setInterval(scroll_period);
	tb_name = doc[F("tb_name")].as<String>();
	tb_chats = doc[F("tb_chats")].as<String>();
	tb_token = doc[F("tb_token")].as<String>();
	tb_secret = doc[F("tb_secret")].as<String>();
	tb_rate = doc[F("tb_rate")]; // telegramTimer.setInterval(1000U * tb_rate);
	web_login = doc[F("web_login")].as<String>();
	web_password = doc[F("web_password")].as<String>();

	LOG(printf_P, PSTR("размер объекта config: %i\n"), doc.memoryUsage());
}

void save_config_main() {
	StaticJsonDocument<1536> doc; // временный буфер под объект json

	doc[F("str_hello")] = str_hello;
	doc[F("max_alarm_time")] = max_alarm_time;
	doc[F("run_allow")] = run_allow;
	doc[F("run_begin")] = run_begin;
	doc[F("run_end")] = run_end;
	doc[F("wide_font")] = wide_font;
	doc[F("show_move")] = show_move;
	doc[F("delay_move")] = delay_move;
	doc[F("tz_shift")] = tz_shift;
	doc[F("tz_dst")] = tz_dst;
	doc[F("date_short")] = show_date_short;
	doc[F("date_period")] = show_date_period;
	doc[F("time_color")] = show_time_color;
	doc[F("time_color0")] = color_to_text(show_time_color0);
	doc[F("time_color1")] = color_to_text(show_time_col[0]);
	doc[F("time_color2")] = color_to_text(show_time_col[1]);
	doc[F("time_color3")] = color_to_text(show_time_col[2]);
	doc[F("time_color4")] = color_to_text(show_time_col[3]);
	doc[F("time_color5")] = color_to_text(show_time_col[4]);
	doc[F("date_color")] = show_date_color;
	doc[F("date_color0")] = color_to_text(show_date_color0);
	doc[F("bright_mode")] = bright_mode;
	doc[F("bright0")] = bright0;
	doc[F("br_boost")] = br_boost;
	doc[F("max_power")] = max_power;
	doc[F("turn_display")] = turn_display;
	doc[F("volume_start")] = volume_start;
	doc[F("volume_finish")] = volume_finish;
	doc[F("volume_period")] = volume_period;
	doc[F("sync_time_period")] = sync_time_period;
	doc[F("scroll_period")] = scroll_period;
	doc[F("tb_name")] = tb_name;
	doc[F("tb_chats")] = tb_chats;
	doc[F("tb_token")] = tb_token;
	doc[F("tb_secret")] = tb_secret;
	doc[F("tb_rate")] = tb_rate;
	doc[F("web_login")] = web_login;
	doc[F("web_password")] = web_password;

	File configFile = LittleFS.open(F("/config.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(2);

	LOG(printf_P, PSTR("размер объекта config: %i\n"), doc.memoryUsage());
}

void load_config_alarms() {
	StaticJsonDocument<1024> doc; // временный буфер под объект json

	File configFile = LittleFS.open(F("/alarms.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open config for alarms file"));
		//  Создаем файл запив в него данные по умолчанию
		save_config_alarms();
		return;
	}

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();
	
	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return;
	}

	for( int i=0; i<min(MAX_ALARMS,(int)doc.size()); i++) {
		alarms[i].settings = doc[i]["s"];
		alarms[i].hour = doc[i]["h"];
		alarms[i].minute = doc[i]["m"];
		alarms[i].melody = doc[i]["me"];
	}

	LOG(printf_P, PSTR("размер объекта alarms: %i\n"), doc.memoryUsage());
}

void save_config_alarms() {
	StaticJsonDocument<1024> doc; // временный буфер под объект json

	for( int i=0; i<MAX_ALARMS; i++) {
		doc[i]["s"] = alarms[i].settings;
		doc[i]["h"] = alarms[i].hour;
		doc[i]["m"] = alarms[i].minute;
		doc[i]["me"] = alarms[i].melody;
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

	LOG(printf_P, PSTR("размер объекта alarms: %i\n"), doc.memoryUsage());
}

void load_config_texts() {
	StaticJsonDocument<1536> doc; // временный буфер под объект json

	File configFile = LittleFS.open(F("/texts.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open config for texts file"));
		//  Создаем файл запив в него данные по умолчанию
		save_config_texts();
		return;
	}

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();
	
	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return;
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

	LOG(printf_P, PSTR("размер объекта texts: %i\n"), doc.memoryUsage());
}

void save_config_texts() {
	StaticJsonDocument<1536> doc; // временный буфер под объект json

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
	delay(2);

	LOG(printf_P, PSTR("размер объекта texts: %i\n"), doc.memoryUsage());
}

void load_config_security() {
	StaticJsonDocument<256> doc; // временный буфер под объект json

	File configFile = LittleFS.open(F("/security.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open config for texts file"));
		//  Создаем файл запив в него данные по умолчанию
		save_config_security();
		return;
	}

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();
	
	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return;
	}

	sec_enable = doc[F("sec_enable")];
	sec_curFile = doc[F("sec_curFile")];

	LOG(printf_P, PSTR("размер объекта security: %i\n"), doc.memoryUsage());
}

void save_config_security() {
	StaticJsonDocument<256> doc; // временный буфер под объект json

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

	LOG(printf_P, PSTR("размер объекта security: %i\n"), doc.memoryUsage());
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
	// int16_t nCur = cur;
	for(int16_t i = min(aCnt,cnt); i > 0; i--) {
		cur = cur > 0 ? cur-1: cnt-1;
		ptr = aStr[cur] + strlen(aStr[cur]) - 1;
		strcpy(ptr, "%0A"); // замена последнего символа на url-код склейки строк
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
	if (size > SEC_LOG_MAXFILE) {
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