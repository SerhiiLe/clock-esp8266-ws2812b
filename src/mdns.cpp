/*
	включение/отключение mdns
	(из-за конфликта с dfPlayer)
*/

#include <Arduino.h>
#include <ESP8266mDNS.h>
#include "defines.h"

bool fl_mdns = false;
unsigned long last_mdns = 0;

void mdns_start(bool start) {
	if(start && !fl_mdns) {
		if(millis() - last_mdns < MDNS_PAUSE) return;
		last_mdns = millis();
		//назначаем символьное имя mDNS нашему серверу опираясь на его динамически полученный IP
		if(MDNS.begin(clock_name, WiFi.localIP())) {
			MDNS.addService("http", "tcp", 80);
			fl_mdns = true;
			LOG(println, PSTR("MDNS responder started"));
		}
	}
	if(!start && fl_mdns) {
		MDNS.close();
		fl_mdns = false;
		LOG(println, PSTR("MDNS responder stoped"));
	}
}

void mdns_process() {
	if(fl_mdns) MDNS.update();
	else mdns_start(true);
}

void mdns_setHostname(const char *str) {
	if(fl_mdns)	MDNS.setHostname(str);
}