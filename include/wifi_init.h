#ifndef wifi_init_h
#define wifi_init_h

void wifi_setup();
void wifi_process();
String wifi_currentIP();
int8_t wifi_rssi();
void wifi_startConfig(bool a);

#endif