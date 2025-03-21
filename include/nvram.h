#ifndef nvram_h
#define nvram_h

bool nvram_init();
uint16_t fletcher16(uint8_t *data, size_t len);
bool readBlock(uint8_t num, uint8_t *data, uint16_t block_size);
bool writeBlock(uint8_t num, uint8_t *data, uint16_t block_size, uint8_t chunk_num=255, uint16_t chunk_size=0);

#endif