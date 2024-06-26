// вывод времени / даты
#ifndef clock_h
#define clock_h

const char* clockCurrentText(char *a, bool fl_12=false); // выводит в строку текущее время
const char* clockTinyText(char *a, bool fl_12=false); // выводит в строку текущее время с секундами
const char* dateCurrentTextShort(char *a); // вывод в строку текущей даты
const char* dateCurrentTextLong(char *a); // вывод в строку даты с названием месяца

#endif