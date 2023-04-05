#ifndef mdns_h
#define mdns_h

void mdns_start(bool start);
void mdns_process();
void mdns_setHostname(const char *str);

#endif