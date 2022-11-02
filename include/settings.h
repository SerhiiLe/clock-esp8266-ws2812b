#ifndef settings_h
#define settings_h

bool load_config_main();
void save_config_main();
bool load_config_alarms();
void save_config_alarms();
bool load_config_texts();
void save_config_texts();
bool load_config_security();
void save_config_security();
bool load_config_telegram();
void save_config_telegram();
void save_log_file(uint8_t mt);
uint32_t text_to_color(const char *s);
String color_to_text(uint32_t n);
String read_log_file(int16_t cnt);

#define SEC_TEXT_EMPTY 0	// логирование движений отключено
#define SEC_TEXT_DISABLE 1	// логирование движений отключено
#define SEC_TEXT_ENABLE 2	// логирование движений включено
#define SEC_TEXT_MOVE 3		// зарегистрировано движение
#define SEC_TEXT_BRIGHTNESS 4	// освещение резко изменилось
#define SEC_TEXT_BOOT 5		// часы включились (наверное был сбой питания)
#define SEC_TEXT_POWERED 6	// питание включилось
#define SEC_TEXT_POWEROFF 7	// питание отключилось
#define SEC_LOG_MAX 40		// максимальная строка одной записи лога (35 + символы склейки "%0A" + конец строки \0)
#define SEC_LOG_SIZE 4096	// максимальный размер файла, после которого запись будет во второй файл. (запись по кругу)
#define SEC_LOG_COUNT 3		// число файлов
const char SEC_LOG_FILE[] PROGMEM = "/log%u.txt"; // шаблон имени файла

#endif