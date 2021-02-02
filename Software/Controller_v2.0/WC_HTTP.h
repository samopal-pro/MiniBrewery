/**
* Реализация HTTP сервера
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#ifndef WS_HTTP_h
#define WS_HTTP_h
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "WC_Config.h"
#include "STask.h"
#include "src/WiFiManager.h"
//#include "DallasTemperature.h"

//extern WebServer server;
extern WiFiManager wifiManager;



void HTTP_begin(void);
void HTTP_stop(void);
bool HTTP_loop();
bool HTTP_checkTM();
bool SetParamHTTP();
int  HTTP_isAuth();
void HTTP_handleRoot(void);
void HTTP_handleInline(void);
void HTTP_handleUpdate(void);
void HTTP_handleUpload(void);
void HTTP_handleNotFound(void);
void HTTP_handleConfig(void);
void HTTP_handleLogin(void);
void HTTP_handleReboot(void);
void HTTP_handleDownload(void);
void HTTP_handleDefault(void);
void HTTP_handleParam(void);
void HTTP_handleManual(void);
void HTTP_handleLogo(void);
void HTTP_printInput(String &out,const char *label, const char *name, const char *value, int size, int len,bool is_pass=false);
void HTTP_printHeader(String &out,const char *title, uint16_t refresh=0);
void HTTP_printTail(String &out);
//void SetPwm();
void HTTP_device(void);
//void HTTP_handleGraph(void);
//void HTTP_handleData(void);
//void DisplayText1( char *t ,int size = 2);
//void DisplayText2( char *t ,int size = 2);



#endif
