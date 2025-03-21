#ifndef barometer_h
#define barometer_h

bool barometer_init();
const char* currentPressureTemp (char *a, bool fl_tiny = false);
int32_t getPressure(bool fl_cor=true);
float getTemperature(bool fl_cor=true);
float getHumidity();

#endif