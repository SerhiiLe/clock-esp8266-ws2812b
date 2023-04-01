#ifndef settings_init_h
#define settings_init_h

#include "defines.h"

String str_hello = "Start"; // строка которая выводится в момент запуска часов
uint8_t max_alarm_time = 5; // максимальное время работы будильника
uint8_t run_allow = 0; // режим работы бегущей строки
uint16_t run_begin = 0; // время начала работы бегущей строки
uint16_t run_end = 1439; // время окончания работы бегущей строки
uint8_t wide_font = 0; // использовать обычный широкий шрифт
uint8_t show_move = 1; // включение светодиода датчика движения
uint8_t delay_move = 5; // задержка срабатывания датчика движения (если есть ложные срабатывания)
uint8_t max_move = 30; // максимальное время работы матрицы при питании от аккумулятора
int8_t tz_shift = TIMEZONE; // временная зона, смещение локального времени относительно Гринвича
uint8_t tz_dst = DSTSHIFT; // смещение летнего времени
uint8_t show_date_short = 0; // показывать дату в коротком формате
uint16_t show_date_period = 30; // периодичность вывода даты в секундах
uint8_t show_time_color = 0; // режим выбора цветов циферблата часов
uint32_t show_time_color0 = 0xFFFFFF; // цвет цифр часов (белый)
uint32_t show_time_col[5] = {0xF6D32D,0xF6D32D,0x4444FF,0x57E389,0x57E389}; // отдельно для каждой цифры
uint8_t show_date_color = 0; // режим выбора цветов даты
uint32_t show_date_color0 = 0xFFFFFF; // цвет даты
uint8_t bright_mode = 1; // режим яркости матрицы (авто или ручной)
uint8_t bright0 = 50; // яркость матрицы средняя (1-255)
uint16_t bright_boost = 100; // усиление показателей датчика яркости в процентах (1-250)
uint8_t boost_mode = 0; // режим дополнительного увеличения яркости
uint8_t bright_add = 1; // на сколько дополнительно увеличивать яркость
float latitude = 0.0f; // географическая широта
float longitude = 0.0f; // географическая долгота
uint16_t bright_begin = 0; // время начала дополнительного увеличения яркости
uint16_t bright_end = 0; // время окончания дополнительного увеличения яркости
uint32_t max_power = DEFAULT_POWER; // максимальное потребление матрицы (чтобы блок питания тянул)
uint8_t turn_display = 0; // перевернуть картинку
uint8_t volume_start = 5; // начальная громкость будильника
uint8_t volume_finish = 30; // конечная громкость будильника
uint8_t volume_period = 5; // период в сек увеличения громкости на единицу
uint8_t timeout_mp3 = 36; // таймаут до принудительного сброса модуля mp3, в часах
uint8_t sync_time_period = 8; // периодичность синхронизации ntp, в часах
uint16_t scroll_period = 40; // задержка между обновлениями бегущей строки, определяет скорость движения
String web_login = "admin"; // логин для вэб
String web_password = ""; // пароль для вэб

uint8_t use_move = 1; // использовать датчик движения как датчик сигнализации
uint8_t use_brightness = 1; // использовать датчик освещения как датчик сигнализации
String pin_code = "def555"; // пин-код доступа к отправке сообщений в телеграм другим устройствам
String clock_name = "clock"; // название часов для mDNS
uint16_t sensor_timeout = 20; // время в течении которого сенсор считается действующим, в минутах
String tb_name = ""; // имя бота, адрес. Свободная строка, только для справки
String tb_chats = ""; // чаты из которых разрешено принимать команды
String tb_secret = ""; // пароль для подключения функции управления из чата в телеграм
String tb_token = ""; // API токен бота
uint16_t tb_rate = 300; // интервал запроса новых команд в секундах
uint16_t tb_accelerated = TELEGRAM_ACCELERATED; // ускоренный интервал запроса новых команд в секундах
uint16_t tb_accelerate = TELEGRAM_ACCELERATE; // время в течении которого будет работать ускорение
uint16_t tb_ban = TELEGRAM_BAN; // время на которе прекращается опрос новых сообщений, после сбоя, в секундах

cur_alarm alarms[MAX_ALARMS];
cur_text texts[MAX_RUNNING];

uint8_t sec_enable = 0;
uint8_t sec_curFile = 0;

#endif