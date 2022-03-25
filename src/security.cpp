// "охрана", информирование о срабатывании датчика движения

#include <Arduino.h>
#include <FastBot.h>
#include "security.h"
#include "defines.h"
#include "settings.h"
#include "ntp.h"

#define MAX_MESSAGES 10
#define FB_DYNAMIC_HTTP

FastBot tb;

// 0 - ожидание
// 1 - ОК
// 2 - Переполнен по ovf
// 3 - Ошибка телеграм
// 4 - Ошибка подключения
// 5 - не задан chat ID
// 6 - множественная отправка, статус неизвестен

void inMsg(FB_msg& msg);
bool fl_secretWanted = false;
time_t last_telegram = 0;
bool fl_accelTelegram = false;

void setup_telegram() {
	// tb.setChatID(tb_chats);
	tb.setToken(tb_token);
}

// инициализация бота
void init_telegram() {
	setup_telegram();
	tb.setLimit(MAX_MESSAGES);
	// tb.setPeriod(1000U * tb_rate);
	tb.attach(inMsg);
}

void tb_tick() {
	tb.tickManual();
}

void tb_send_msg(String s) {
	if(sec_enable) {
		Serial.print(F("Send to telegram: "));
		Serial.println(tb.sendMessage(s,tb_chats));
		// tb.sendMessage(s,tb_chats);
		save_log_file(SEC_TEXT_MOVE);
	}
}

void inMsg(FB_msg& msg) {
	char buf[100];

	// выводим ID чата, имя юзера и текст сообщения
	Serial.printf_P(PSTR("From telegram:%s;%s;%s;%s;%s.\n"),msg.chatID,msg.username,msg.first_name,msg.ID,msg.text);

	if(!fl_accelTelegram && tb_rate > 10) {
		telegramTimer.setInterval(10000);
		fl_accelTelegram = true;
	}
	last_telegram = getTimeU();

	msg.text.trim();

	bool fl_start = false;
	if(fl_secretWanted) {
		fl_secretWanted = false;
		if(msg.text == tb_secret) {
			tb.deleteMessage(0, msg.chatID);
			tb.deleteMessage(1, msg.chatID);
			tb.sendMessage(F("Добро пожаловать!"), msg.chatID);
			if(tb_chats.length()>0) tb_chats += ",";
			tb_chats += msg.chatID;
			save_config_main();
		} else {
			tb.deleteMessage(0, msg.chatID);
			tb.deleteMessage(1, msg.chatID);
			tb.sendMessage(F("Ошибка!"), msg.chatID);
			return;
		}
		fl_start = true;
	}

	msg.text.toLowerCase();

	bool fl_auth = true;
	// проверка авторизован ли этот чат
	if(tb_chats.length() == 0) fl_auth = false;
	if(tb_chats.length() > 0 && tb_chats.indexOf(msg.chatID) < 0) fl_auth = false;

	if(fl_auth) {
		if(msg.text.startsWith(F("last"))) {
			tb.sendMessage(read_log_file(constrain(msg.text.substring(msg.text.lastIndexOf(" ")).toInt(), 1, 40)), msg.chatID);
			return;
		} else
		if(msg.text == F("uptime")) {
			tb.sendMessage(getUptime(buf), msg.chatID);
			return;
		} else
		if(msg.text == F("on")) {
			tb.sendMessage(F("Отсылка сообщений включена."), msg.chatID);
			if(!sec_enable) {
				sec_enable = 1;
				save_log_file(SEC_TEXT_ENABLE);
				save_config_security();
			}
			return;
		} else
		if(msg.text == F("off")) {
			tb.sendMessage(F("Отсылка сообщений отключена."), msg.chatID);
			if(sec_enable) {
				sec_enable = 0;
				save_log_file(SEC_TEXT_DISABLE);
				save_config_security();
			}
			return;
		} else
		if(msg.text == F("status")) {
			sprintf_P(buf,PSTR("Датчик: %s.%%0AПитание: %s.%%0AОсвещение: %d -> %d."),
				sec_enable?F("включён"):F("отключён"),
				fl_5v?F("сеть"):F("аккумулятор"),
				analogRead(PIN_PHOTO_SENSOR), led_brightness);
			tb.sendMessage(buf, msg.chatID);
			return;
		} else
		if(msg.text == F("logout")) {
			int pos1 = tb_chats.indexOf(msg.chatID);
			if(pos1<0) {
				tb.sendMessage(F("err"), msg.chatID);
				return;
			}
			int pos2 = tb_chats.indexOf(",", pos1);
			if(pos1>0) pos1--;
			String nt = tb_chats.substring(0,pos1);
			if(pos2>0) nt = tb_chats.substring(pos2);
			tb_chats = nt;
			save_config_main();
			tb.sendMessage(F("Вышли..."), msg.chatID);
			fl_start = true;
			fl_auth = false;
		}
	} else {
 		if(msg.text == F("login")) {
			tb.sendMessage(F("Пароль доступа?"), msg.chatID);
			fl_secretWanted = true;
			return;
		}
	}
	if(fl_start || msg.text == F("start")) {
		// показать юзер меню (\t - горизонтальное разделение кнопок, \n - вертикальное
		// bot.showMenu("Menu1 \t Menu2 \t Menu3 \n Menu4");
		// отправить инлайн меню (\t - горизонтальное разделение кнопок, \n - вертикальное
  		// (текст сообщения, кнопки)
  		// bot.inlineMenu("Choose wisely", "Answer 1 \t Answer 2 \t Answer 3 \n Answer 4");
		if(fl_auth)	tb.showMenuText(F("/help если надо"), F("On\tOff\tStatus\tShow ID\nLogout\tUptime\tLast 20"), msg.chatID);
		else tb.showMenuText(F("/help если надо"), F("Login\tAbout\tShow ID"), msg.chatID);
		return;
	} else
	if(msg.text == F("stop")) {
		tb.closeMenu(msg.chatID);
		return;
	} else
	if(msg.text == F("show id")) {
		tb.sendMessage(msg.chatID, msg.chatID);
		return;
	} else
	if(msg.text == F("about")) {
		tb.sendMessage(F("Бот для управления датчиком движения в часах. Ничего интересного, можно проходить мимо."), msg.chatID);
		return;
	} else
	if(msg.text == F("help")) {
		Serial.println(tb.sendMessage(F("Все возможности в меню:%0AStart - показать меню,%0AStop - спрятать меню."), msg.chatID));
		return;
	}
	tb.sendMessage(F("Что? Не понятно..."), msg.chatID);
}
