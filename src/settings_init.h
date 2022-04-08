#ifndef settings_init_h
#define settings_init_h

#include "defines.h"

String str_hello = "Start"; // строка которая выводится в момент запуска часов
uint8_t max_alarm_time = 5; // максимальное время работы будильника
uint8_t run_allow = 0; // режим работы бегущей строки
uint16_t run_begin = 0; // время начала работы бегущей строки
uint16_t run_end = 1440; // время окончания работы бегущей строки
uint8_t wide_font = 0; // использовать обычный широкий шрифт
uint8_t show_move = 1; // включение светодиода датчика движения
uint8_t delay_move = 4; // задержка срабатывания датчика движения (если есть ложные срабатывания)
int8_t tz_shift = TIMEZONE; // временная зона, смещение локального времени относительно Гринвича
int8_t tz_dst = DSTSHIFT; // смещение летнего времени
uint8_t show_date_short = 0; // показывать дату в коротком формате
uint16_t show_date_period = 30; // периодичность вывода даты в секундах
uint8_t show_time_color = 0; // режим выбора цветов циферблата часов
uint32_t show_time_color0 = 0xFFFFFF; // цвет цифр часов (белый)
uint32_t show_time_col[5] = {0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF}; // отдельно для каждой цифры
uint8_t show_date_color = 0; // режим выбора цветов даты
uint32_t show_date_color0 = 0xFFFFFF; // цвет даты
uint8_t bright_mode = 1; // режим яркости матрицы (авто или ручной)
uint8_t bright0 = 50; // яркость матрицы средняя (1-255)
uint8_t br_boost = 100; // усиление показателей датчика яркости в процентах (1-250)
uint32_t max_power = DEFAULT_POWER; // максимальное потребление матрицы (чтобы блок питания тянул)
uint8_t turn_display = 0; // перевернуть картинку
uint8_t volume_start = 5; // начальная громкость будильника
uint8_t volume_finish = 30; // конечная громкость будильника
uint8_t volume_period = 5; // период в сек увеличения громкости на единицу
uint16_t sync_time_period = 60; // периодичность синхронизации ntp в минутах
uint16_t scroll_period = 50; // задержка между обновлениями бегущей строки, определяет скорость движения
String tb_name = ""; // имя бота, адрес. Свободная строка, только для справки
String tb_chats = ""; // чаты из которых разрешено принимать команды
String tb_secret = ""; // пароль для подключения функции управления из чата в телеграм
String tb_token = ""; // API токен бота
uint16_t tb_rate = 60; // интервал запроса новых команд в секундах
String web_login = "admin"; // логин для вэб
String web_password = ""; // пароль для вэб

cur_alarm alarms[MAX_ALARMS];
cur_text texts[MAX_RUNNING];

uint8_t sec_enable = 0;
uint8_t sec_curFile = 0;

#endif