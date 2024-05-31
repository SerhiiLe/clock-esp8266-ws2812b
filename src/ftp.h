/*
    подключение FTP сервера, вынесено сюда, чтобы не захламлять основной код
*/
// #define ESP8266

#include <FTPServer.h>

FTPServer ftpSrv(LittleFS);
bool ftp_isStarted = false;
bool ftp_isAllow = false;

void ftp_process () {
    if( ftp_isStarted ) {
        if(ftp_isAllow) ftpSrv.handleFTP();
        else {
            ftpSrv.stop();
            ftp_isStarted = false;
        }
    }
    else {
        if(ftp_isAllow) {
            ftpSrv.begin(gs.web_login, gs.web_password);
            ftp_isStarted = true;
        }
    }
}