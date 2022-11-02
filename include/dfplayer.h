#ifndef dfplayer_h
#define dfplayer_h

// возвращает, играет ли сейчас плейер
boolean mp3_isPlay();
// устанавливает громкость (t= от 1 до 30). p - запоминать ли уровень (p=false - не запоминать)
void mp3_volume(uint8_t t, boolean p=true);
// инициализация плейера
void mp3_init();
// Проверка данных от плейера
void mp3_check();
// играть трек с номером t
void mp3_play(int t);
// перечитать список файлов
void mp3_reread();
// обновить номер текущего трека
void mp3_update();
// запустить
void mp3_start();
void mp3_pause();
void mp3_stop();
void mp3_enableLoop();
void mp3_disableLoop();
void mp3_enableLoopAll();
void mp3_disableLoopAll();
void mp3_randomAll();

#endif