#ifndef ntp_h
#define ntp_h

bool syncTime();
time_t getTimeU();
tm getTime(time_t *t = nullptr);
const char* getUptime(char *str);

#endif
