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
#include "defines.h"
#include "dfplayer.h"
#ifdef SRX
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

EspSoftwareSerial::UART mp3Serial;
DFRobotDFPlayerMini dfPlayer;
#endif

int mp3_all = 0;
int mp3_current = 1;
int8_t cur_Volume = 15;
bool mp3_isInit = false;
bool mp3_isReady = false;

#ifdef SRX
// плата установлена, описание функций

void checkInit() {
	if( ! mp3_isInit || timeoutMp3Timer.isReady() ) mp3_init();
}

boolean mp3_isPlay() {
	checkInit();
	return dfPlayer.readState() & 1;
}

void mp3_update() {
	if(mp3_isPlay() || mp3_current>mp3_all )
		mp3_current = dfPlayer.readCurrentFileNumber();
}

// DFPlayer медленный и любит при каждом чихе отваливаться, по этому много проверок и задержек. Некрасиво, но работает достаточно устойчиво.

void mp3_init() {
	LOG(println, PSTR("init mp3 player"));
	if( mp3_isInit ) {
		mp3Serial.flush();
		dfPlayer.reset();
	} else {
		mp3Serial.begin(9600, SWSERIAL_8N1, SRX, STX, false);
		dfPlayer.setTimeOut(1000);
		mp3_isReady = dfPlayer.begin(mp3Serial);
		if(mp3_isReady) {
			mp3_isInit = true;
			mp3_reread();
		}
	}
	if(mp3_isReady) {
		dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
		delay(10);
		mp3_volume(1,false);
	}
	if(dfPlayer.readCurrentFileNumber()>mp3_all) {
		dfPlayer.start();
		delay(10);
		mp3_update();
		dfPlayer.stop();
		delay(10);
	}
}

void mp3_volume(uint8_t t, boolean p) {
	checkInit();
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
	timeoutMp3Timer.reset();
}

void mp3_play(int t) {
	checkInit();
	if( mp3_all == 0 ) return;
	if( t < 1 || t > mp3_all ) return;
	LOG(printf_P, PSTR("want track: %i\n"),t);
	if( ! mp3_isPlay() ) dfPlayer.start();
	delay(100);
	if(dfPlayer.readCurrentFileNumber() != t) {
		int cur = 0, old = 0, cnt = 0;
		while(true) {
			cur = dfPlayer.readCurrentFileNumber();
			LOG(printf_P, PSTR("track: %i\n"),cur);
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
	timeoutMp3Timer.reset();
}

void mp3_reread() {
	checkInit();
	mp3_all = dfPlayer.readFileCounts();
	// mp3_isReady = mp3_all == 0 ? false: true;
}

void mp3_start() {
	checkInit();
	dfPlayer.start();
}

void mp3_pause() {
	checkInit();
	dfPlayer.pause();
}

void mp3_stop() {
	checkInit();
	dfPlayer.stop();
}

void mp3_enableLoop() {
	checkInit();
	dfPlayer.enableLoop();
}

void mp3_disableLoop() {
	checkInit();
	dfPlayer.disableLoop();
}

void mp3_enableLoopAll() {
	checkInit();
	dfPlayer.enableLoopAll();
}

void mp3_disableLoopAll() {
	checkInit();
	dfPlayer.disableLoopAll();
}

void mp3_randomAll() {
	checkInit();
	dfPlayer.randomAll();
}

void mp3_messages(uint8_t type, int value) {
switch (type) {
	case TimeOut:
		LOG(println, PSTR("Time Out!"));
		mp3_isInit = false;
		break;
	case WrongStack:
		LOG(println, PSTR("Stack Wrong!"));
		mp3_isInit = false;
		break;
	case DFPlayerCardInserted:
		LOG(println, PSTR("Card Inserted!"));
		break;
	case DFPlayerCardRemoved:
		LOG(println, PSTR("Card Removed!"));
		mp3_all = 0;
		break;
	case DFPlayerCardOnline:
		LOG(println, PSTR("Card Online!"));
		mp3_reread();
		break;
	case DFPlayerUSBInserted:
		LOG(println, PSTR("USB Inserted!"));
		break;
	case DFPlayerUSBRemoved:
		LOG(println, PSTR("USB Removed!"));
		break;
	case DFPlayerPlayFinished:
		LOG(printf_P, PSTR("Number: %i. Play Finished!\n"),value);
		dfPlayer.stop();
		break;
	case DFPlayerFeedBack:
		LOG(printf_P, PSTR("Feedback: %i. Play Finished!\n"),value);
		mp3_current = value;
		// dfPlayer.stop();
		break;
	case DFPlayerError:
		LOG(print, PSTR("DFPlayerError:"));
		switch (value) {
			case Busy:
				LOG(println, PSTR("Card not found"));
				break;
			case Sleeping:
				LOG(println, PSTR("Sleeping"));
				break;
			case SerialWrongStack:
				LOG(println, PSTR("Get Wrong Stack"));
				break;
			case CheckSumNotMatch:
				LOG(println, PSTR("Check Sum Not Match"));
				break;
			case FileIndexOut:
				LOG(println, PSTR("File Index Out of Bound"));
				break;
			case FileMismatch:
				LOG(println, PSTR("Cannot Find File"));
				break;
			case Advertise:
				LOG(println, PSTR("In Advertise"));
				break;
			default:
				LOG(printf_P, PSTR("Unknown error: %i\n"),value);
				break;
		}
		break;
	default:
		LOG(printf_P, PSTR("Unknown: %i, val: %i\n"),type,value);
		break;
	}
}

void mp3_check() {
	if (dfPlayer.available()) {
		mp3_messages(dfPlayer.readType(), dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
	}
}

#else
// заглушки, если плата DFPlayer не установлена
boolean mp3_isPlay() {return true;}
void mp3_volume(uint8_t t, boolean p) {}
void mp3_init() {mp3_isInit = true; mp3_isReady = true;}
void mp3_check() {}
void mp3_play(int t) {}
void mp3_reread() {}
void mp3_update() {}
void mp3_start() {}
void mp3_pause() {}
void mp3_stop() {}
void mp3_enableLoop() {}
void mp3_disableLoop() {}
void mp3_enableLoopAll() {}
void mp3_disableLoopAll() {}
void mp3_randomAll() {}
#endif