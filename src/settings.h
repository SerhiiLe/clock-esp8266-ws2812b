#ifndef settings_h
#define settings_h

void save_config_main();
void load_config_main();
void load_config_alarms();
void save_config_alarms();
void load_config_texts();
void save_config_texts();
void load_config_security();
void save_config_security();
void save_log_file(uint8_t mt);
uint32_t text_to_color(const char *s);
String color_to_text(uint32_t n);
String read_log_file(int16_t cnt);

#define SEC_TEXT_DISABLE 0	// логирование движений отключено
#define SEC_TEXT_ENABLE 1	// логирование движений включено
#define SEC_TEXT_MOVE 2		// зарегистрировано движение
#define SEC_TEXT_BOOT 3		// часы включились (наверное был сбой питания)
#define SEC_TEXT_POWERED 4	// питание включилось
#define SEC_TEXT_POWEROFF 5	// питание отключилось
#define SEC_LOG_MAX 50		// максимальная строка одной записи лога (достаточно должно быть 30, но с запасом)
#define SEC_LOG_MAXFILE 4096	// максимальный размер файла, после которого запись будет во второй файл. (запись по кругу)
#define SEC_LOG_COUNT 3		// число файлов
const char SEC_LOG_FILE[] PROGMEM = "/log%u.txt"; // шаблон имени файла

#endif