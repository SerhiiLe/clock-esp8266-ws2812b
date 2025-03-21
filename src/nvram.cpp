/*
	Модуль работы с NVRAM расположенном на внешнем чипе
	(судя по спецификации ресурс отдельного чипа в 200 раз больше ресурса FLASH ESP8266, 1000000 vs 5000)
	https://ww1.microchip.com/downloads/en/devicedoc/doc0336.pdf
*/

#include <Arduino.h>
#include <Wire.h>
#include <AT24CX.h>
#include "nvram.h"
#include "defines.h"

/*
Краткое содержание спецификации:
Объём памяти указывается в битах, например AT24C32 имеет 32 кбит память или 32/8 = 4 кбайт.
Чтение и запись идёт побайтно, а не побитно.
Адресация внутри чипа страничная, то есть адрес внутри чипа состоит из двух байт - адрес страницы + адрес на странице.
Страницы могут быть по 32 байта (AT24C32 и AT24C64), 64 (AT24C128 и AT24C256), или 128 (AT24C512).
*/
/*
Запись и чтение не может быть больше числа байт на одной странице, но библиотека делает последовательные запросы.
Библиотека пишет побайтно или такие данные:
int - 2 байта (uint16_t), long - 4 (uint32_t), float - 4, double - 8, chars - указывается в третьем параметре и аналогична побайтовым read/write
*/
/*
Организация памяти:
Данные собраны в блоки, аналогичные JSON, но без названий для экономии места.
Из этого следует, что любое изменение формата (добавление или удаление полей) приводит к полной не читаемости данных.
В начале каждого блока первые 2 байта - длина блока, вторые 2 байта - контрольная сумма блока (fletcher16).
Таким образом адрес следующего блока = размер текущего блока + 4.
Если длина блока не соответствует ожидаемой, то этот блок считаются испорченными.
Если контрольная сумма не соответствует записанной, то блок считается испорченным.
Для корректной работы нужно последовательно инициализировать все блоки от 0 до последнего, чтобы в случае сбоя инициализировать их.
*/

#define CHIP_ADDRESS 0x50	// адрес чипа на шине i2c
#define CHIP_CAPACITY 4096	// ёмкость чипа в байтах
#define CHIP_TYPE AT24C32	// тип чипа (чип серии AT24CX, номер чипа, объём 32kBit (4kByte), 32 байта одна страница)

CHIP_TYPE *nvram;

bool nvram_enable = false;
// номер чипа EEPROM на шине i2c (может быть до 8 чипов на одной шине, выбирается перемычками)
uint8_t eeprom_chip = 0;

bool nvram_init() {
	if( ! eeprom_chip ) return false;
	nvram = new CHIP_TYPE(eeprom_chip - CHIP_ADDRESS);
	nvram_enable = true;
	return true;
}

uint16_t fletcher16(uint8_t *data, size_t len) {
  uint16_t sum1 = 0;
  uint16_t sum2 = 0;
  while( len-- ) {
    sum1 = (sum1 + *(data++)) % 255;
    sum2 = (sum1 + sum2) % 255;
  }
  return (sum2 << 8) | sum1;
}
// LOG(println, fletcher16((uint8_t*)&gs, sizeof(gs)), HEX);

bool readBlock(uint8_t num, uint8_t *data, uint16_t block_size) {
	if( ! nvram_enable ) return false;
	uint16_t addr = 0;
	uint16_t next_addr = 0;
	uint16_t csum = 0;
	uint16_t size = 0;
	// поиск нужного блока с помощью последовательно чтения
	do {
		addr = next_addr;
		size = nvram->readInt(addr);
		csum = nvram->readInt(addr+2);
		next_addr += size + 4;
		LOG(printf_P, PSTR("seek read nvram #%u, size=%u, csum=%04x, addr=%u\n"), num, size, csum, addr);
	} while(num--);
	LOG(printf_P, PSTR("load read nvram size=%u, csum=%04x, addr=%u\n"), size, csum, addr);
	if( size != block_size ) return false; // размер блока не совпал
	nvram->read(addr+4, data, size);	
	LOG(printf_P, PSTR("loaded %u, csum %04x\n"), size, fletcher16(data, size));
	if( fletcher16(data, size) != csum ) return false; // контрольная сумма не совпала
	return true;
}

bool writeBlock(uint8_t num, uint8_t *data, uint16_t block_size, uint8_t chunk_num, uint16_t chunk_size) {
	if( ! nvram_enable ) return false;
	uint16_t addr = 0;
	uint16_t size = 0;
	uint16_t csum = fletcher16(data, block_size);
	// поиск нужного блока с помощью последовательно чтения
	while(num--) {
		size = nvram->readInt(addr);
		addr += size + 4;
		LOG(printf_P, PSTR("seek write nvram #%u, size=%u, addr=%u\n"), num, size, addr);
	}
	LOG(printf_P, PSTR("write nvram size=%u, csum=%04x, addr=%u\n"), block_size, csum, addr);
	if(addr+4+block_size>CHIP_CAPACITY) return false; // выход за пределы памяти
	nvram->writeInt(addr, block_size);
	nvram->writeInt(addr+2, csum);
	if(chunk_num<255) { // запись не всего блока, а только его части. Для ускорения.
		uint16_t start_addr = chunk_num*chunk_size;
		uint16_t finish_addr = (chunk_num+1)*chunk_size-1;
		LOG(printf_P, PSTR("write chunk #%u, size=%u, addr=%u\n"), chunk_num, chunk_size, start_addr);
		if(finish_addr>block_size) return false; // блоков больше, чем должно быть
		nvram->write(addr+4+start_addr, data+start_addr, chunk_size);
	} else // запись блока целиком
		nvram->write(addr+4, data, block_size);
	return true;
}

