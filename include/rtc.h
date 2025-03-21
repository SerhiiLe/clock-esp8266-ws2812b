#ifndef rtc_h
#define rtc_h

uint8_t rtc_init();
void rtc_setSYS();
void rtc_saveTIME(time_t t);
time_t getRTCTimeU();
uint8_t rtcGetByte(uint8_t address);
uint8_t rtcReadBlock(uint8_t address, uint8_t *buf, uint8_t size);
void rtcSetByte(uint8_t address, uint8_t data);
uint8_t rtcWriteBlock(uint8_t address, uint8_t *buf, uint8_t size);
uint8_t fletcher8(uint8_t *data, uint16_t len = 0);

#endif