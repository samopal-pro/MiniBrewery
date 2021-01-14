#ifndef WC_CONFIG_h
#define WC_CONFIG_h
#include <EEPROM.h>
#include <arduino.h>
#include "src/WiFiManager.h"   

enum WC_NETWORK_TYPE {
   NT_NONE = 0, //Нет сети
   NT_GSM,      //GSM сеть
   NT_WIFI      //WIFI сеть
};

struct WC_CONFIG {
// Тип сети 
   WC_NETWORK_TYPE NET;
// Идентификатор контроллера
   char ID[32]; 
   char ID_PASS[32];  
// Параметры подключения WiFi
   char AP_SSID[32];
   char AP_PASS[32];
// Параметры подключения  GSM
   char APN[32];
// Параметры подключения MQTT
   char MQTT_SERVER[32];
   uint16_t MQTT_PORT;
   char MQTT_USER[32];
   char MQTT_PASS[64];
// Интервал отсылки на сервер, сек
   uint16_t MQTT_TM;
// Колибровочные коэффициенты аналоговых входов VAL = ADC*KA +KB;
   float INP0_KA, INP0_KB;
   float INP1_KA, INP1_KB;   
// Интервал гашения экрана 
   uint32_t TM_SCREEN;
// Интервал цикла опроса
   uint32_t TM_LOOP;     
   uint16_t CRC;   
};

void eepromBegin();
void eepromDefault();
void eepromRead();
void eepromSave();
uint16_t eepromCRC();
void eepromParamDefault();
void eepromParamRead();
void eepromParamSave();
uint16_t eepromParamCRC();

struct INPUT_STATUS {
  uint16_t INP0;
  uint16_t INP1;
  bool     INP2;
  bool     INP3;
  bool     OUT0;
  bool     OUT1;
  int      SERV;
  uint16_t CRC;
};


extern  struct WC_CONFIG Config;
extern struct INPUT_STATUS param;
#define TOPIC_PREFIX_SIZE 64
extern char TOPIC_PREFIX1[];
extern char TOPIC_PREFIX2[];




#endif
