// вывод времени / даты
#ifndef clock_h
#define clock_h

const char* clockCurrentText(char *a, bool fl_12=false);
const char* dateCurrentTextShort(char *a, bool tiny=false);
const char* dateCurrentTextLong(char *a);
const char* clockTinyText(char *a, bool fl_12=false);
const char* dateCurrentTextTinyFull(char *a);

#endif