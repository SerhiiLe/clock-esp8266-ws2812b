/*
	Работа с платой DFPlayer mini.
	Получить стабильную работу согласно документации не получилось.
	Не работают:
		запросы на число файлов в каталоге,
		прямое изменение громкости
		прямое указание номера файла
	По этому все файлы выбираются прямым сквозным порядковым номером,
	функцией "следующий"-"предыдущий" файл, перебором.
	Громкость выбирается так-же, "вверх"-"вниз"
*/

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "defines.h"
#include "dfplayer.h"

SoftwareSerial mp3Serial;
DFRobotDFPlayerMini dfPlayer;

int mp3_all = 0;
int mp3_current = 1;
int8_t cur_Volume = 15;
bool mp3_isInit = false;
bool mp3_isReady = false;

boolean mp3_isplay() {
  return dfPlayer.readState() & 1;
}

void mp3_update() {
	if(mp3_isplay())
		mp3_current = dfPlayer.readCurrentFileNumber();
}

void mp3_init() {
	Serial.println(F("init mp3 player"));
	if( mp3_isInit ) {
		mp3Serial.flush();
		dfPlayer.reset();
	} else {
		mp3Serial.begin(9600, SWSERIAL_8N1, SRX, STX, false, 95, 11);
		mp3_isReady = dfPlayer.begin(mp3Serial);
		dfPlayer.setTimeOut(1000);
		if(mp3_isReady) {
			mp3_isInit = true;
			mp3_reread();
		}
	}
	if(mp3_isReady) {
		// delay(100);
		dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
		delay(10);
		mp3_volume(1,false);
	}
}

void mp3_volume(uint8_t t, boolean p) {
  int cur = 0, old = 0;
  while(true) {
	cur = dfPlayer.readVolume();
	if( cur<0 ) {
		mp3_init();
		delay(10);
		continue;
	}
	if( cur==t ) {
		if (p) cur_Volume = t;
		break;
	}
	if( cur==old ) {
		delay(10);
		continue;
	}
	old=cur;
	if( cur<t ) {
		dfPlayer.volumeUp();
		delay(10);
	}
	if( cur>t ) {
		dfPlayer.volumeDown();
		delay(10);
	}      
  }
}

void mp3_play(int t) {
	if( ! mp3_isInit ) mp3_init();
	if( mp3_all == 0 ) return;
	if( t < 1 || t > mp3_all ) return;
	Serial.print(F("want track: "));
	Serial.println(t);
	if( ! mp3_isplay() ) dfPlayer.start();
	delay(100);
	if(dfPlayer.readCurrentFileNumber() != t) {
		int cur = 0, old = 0, cnt = 0;
		while(true) {
			cur = dfPlayer.readCurrentFileNumber();
			Serial.print(F("track: "));
			Serial.println(cur);
			if( cur<0 ) {
				mp3_init();
				delay(10);
				continue;
			}
			if( cur==t ) break;
			if( cur==old ) {
				if( cnt++ > 20 ) {
					mp3_init();
					dfPlayer.start();
					delay(90);
					cnt = 0;
					old = 0;
				}
				delay(10);
				continue;
			} else cnt = 0;
			old=cur;
			if( cur<t ) {
				if( t-cur < (mp3_all >> 1) )
					dfPlayer.next();
				else
					dfPlayer.previous();
				delay(10);
			}
			if (cur>t) {
				if( cur-t < (mp3_all >> 1) )
					dfPlayer.previous();
				else
					dfPlayer.next();
				delay(10);
			}      
		}
	}
	mp3_volume(cur_Volume);
	mp3_current = t;
}

void mp3_reread() {
	mp3_all = dfPlayer.readFileCounts();
	// mp3_isReady = mp3_all == 0 ? false: true;
}

void mp3_start() {
	dfPlayer.start();
}

void mp3_pause() {
	dfPlayer.pause();
}

void mp3_stop() {
	dfPlayer.stop();
}

void mp3_enableLoop() {
	dfPlayer.enableLoop();
}

void mp3_disableLoop() {
	dfPlayer.disableLoop();
}

void mp3_enableLoopAll() {
	dfPlayer.enableLoopAll();
}

void mp3_disableLoopAll() {
	dfPlayer.disableLoopAll();
}

void mp3_randomAll() {
	dfPlayer.randomAll();
}

void mp3_messages(uint8_t type, int value) {
switch (type) {
	case TimeOut:
		Serial.println(F("Time Out!"));
		break;
	case WrongStack:
		Serial.println(F("Stack Wrong!"));
		break;
	case DFPlayerCardInserted:
		Serial.println(F("Card Inserted!"));
		break;
	case DFPlayerCardRemoved:
		Serial.println(F("Card Removed!"));
		mp3_all = 0;
		break;
	case DFPlayerCardOnline:
		Serial.println(F("Card Online!"));
		mp3_reread();
		break;
	case DFPlayerUSBInserted:
		Serial.println(F("USB Inserted!"));
		break;
	case DFPlayerUSBRemoved:
		Serial.println(F("USB Removed!"));
		break;
	case DFPlayerPlayFinished:
		Serial.print(F("Number:"));
		Serial.print(value);
		Serial.println(F(" Play Finished!"));
		dfPlayer.stop();
		break;
	case DFPlayerFeedBack:
		Serial.print(F("Feedback:"));
		Serial.print(value);
		Serial.println(F(" Play Finished!"));
		mp3_current = value;
		// dfPlayer.stop();
		break;
	case DFPlayerError:
		Serial.print(F("DFPlayerError:"));
		switch (value) {
			case Busy:
				Serial.println(F("Card not found"));
				break;
			case Sleeping:
				Serial.println(F("Sleeping"));
				break;
			case SerialWrongStack:
				Serial.println(F("Get Wrong Stack"));
				break;
			case CheckSumNotMatch:
				Serial.println(F("Check Sum Not Match"));
				break;
			case FileIndexOut:
				Serial.println(F("File Index Out of Bound"));
				break;
			case FileMismatch:
				Serial.println(F("Cannot Find File"));
				break;
			case Advertise:
				Serial.println(F("In Advertise"));
				break;
			default:
				Serial.print(F("Unknown error: "));
				Serial.println(value);
				break;
		}
		break;
	default:
		Serial.print(F("Unknown: "));
		Serial.print(type);
		Serial.print(F(" val: "));
		Serial.println(value);
		break;
	}
}

void mp3_check() {
	if (dfPlayer.available()) {
		mp3_messages(dfPlayer.readType(), dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
	}
}