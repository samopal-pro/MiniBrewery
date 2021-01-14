#include "WC_Config.h"

struct WC_CONFIG Config;
char TOPIC_PREFIX1[TOPIC_PREFIX_SIZE];
char TOPIC_PREFIX2[TOPIC_PREFIX_SIZE];
struct INPUT_STATUS param;

/**
   Инициализация EEPROM
*/
void eepromBegin() {
  Serial.println(F("EEPROM init"));
  EEPROM.begin(sizeof(param) + sizeof(Config));
}

/**
   Читаем конфигурацию из EEPROM
*/
void eepromRead() {
  int size1 = sizeof(Config);
  int offset1 = sizeof(param);
  for ( int i = 0; i < size1; i++ ) {
    uint8_t c = EEPROM.read(i + offset1);
    *((uint8_t*)&Config + i) = c;
  }
  if ( eepromCRC() == Config.CRC ) {
    Serial.println(F("EEPROM Config is correct"));
  }
  else {
    Serial.println(F("EEPROM Config CRC incorrect"));
    eepromDefault();
    eepromSave();
  }
  sprintf(TOPIC_PREFIX1, "/nanobrewery/%s", Config.ID);
  Serial.printf("TOPIC %s\n", TOPIC_PREFIX1);
}


/**
   Сохраняем конфигурацию в EEPROM
*/
void eepromSave() {
  Config.CRC = eepromCRC();
  int size1 = sizeof(Config);
  int offset1 = sizeof(param);
  for ( int i = 0; i < size1; i++ ) {
    EEPROM.write(i + offset1, *((uint8_t*)&Config + i));
  }
  EEPROM.commit();
  Serial.println(F("EEPROM Config save"));
}


/**
   Вычисляем контрольную сумму
*/
uint16_t eepromCRC() {
  uint16_t crc = 0;
  size_t sz1 = sizeof(Config);
  uint16_t crc_save = Config.CRC;
  Config.CRC = 0;
  for ( int i = 0; i < sz1; i++)crc += *((uint8_t*)&Config + i);
  Serial.printf("EEPROM CRC %u %u\n", crc, crc_save);
  Config.CRC = crc_save;
  return crc;
}

/**
   Сброс конфиг в значения "по умолчанию"
*/
void eepromDefault() {
  Config.NET = NT_WIFI;
  String id = "ESP" + ESP_getChipId();
  strncpy(Config.ID, id.c_str(), 31);
  //   sprintf(Config.ID,"ESP_%s",ESP_getChipId());
  strncpy(Config.ID_PASS, "12345678", 31);

//  strncpy(Config.AP_SSID, "ZYXEL24", 31);
//  strncpy(Config.AP_SSID, "DIR-320", 31);
//  strncpy(Config.AP_SSID, "USUS_58_2G", 31);
//  strncpy(Config.AP_PASS, "sav59vas", 31);
  strncpy(Config.AP_SSID, "DIRLIRCA", 31);
  strncpy(Config.AP_PASS, "Lirca2016", 31);

//  strncpy(Config.MQTT_SERVER, "5.101.66.67", 31);
  strncpy(Config.MQTT_SERVER, "192.168.1.16", 31);
  Config.MQTT_PORT = 1883;
//  strncpy(Config.MQTT_USER, "ellips", 31);
//  strncpy(Config.MQTT_PASS, "ell123ips", 63);
  strncpy(Config.MQTT_USER, "sav", 31);
  strncpy(Config.MQTT_PASS, "12345", 63);
  Config.MQTT_TM   = 30;

  Config.INP0_KA    = 1;
  Config.INP0_KB    = 0;
  Config.INP1_KA    = 1;
  Config.INP1_KB    = 0;
  Config.TM_LOOP    = 5000;
  Config.TM_SCREEN  = 20000;
}

/**
   Читаем конфигурацию из EEPROM
*/
void eepromParamRead() {
  int size1 = sizeof(param);
  for ( int i = 0; i < size1; i++ ) {
    uint8_t c = EEPROM.read(i);
    *((uint8_t*)&param + i) = c;
  }
  if ( eepromParamCRC() == param.CRC ) {
    Serial.print(F("EEPROM Param is correct "));
    Serial.print(param.OUT0);
    Serial.print(" ");
    Serial.println(param.OUT1);
  }
  else {
    Serial.println(F("EEPROM Param CRC incorrect"));
    eepromParamDefault();
    eepromParamSave();
  }
}


/**
   Сохраняем конфигурацию в EEPROM
*/
void eepromParamSave() {
  param.CRC = eepromParamCRC();
  int size1 = sizeof(param);
  for ( int i = 0; i < size1; i++ ) {
    EEPROM.write(i, *((uint8_t*)&param + i));
  }
  EEPROM.commit();
  Serial.print(F("EEPROM Param save "));
  Serial.print(param.OUT0);
  Serial.print(" ");
  Serial.println(param.OUT1);
}


/**
   Вычисляем контрольную сумму
*/
uint16_t eepromParamCRC() {
  uint16_t crc = 0;
  size_t sz1 = sizeof(param);
  uint16_t crc_save = param.CRC;
  param.CRC = 0;
  for ( int i = 0; i < sz1; i++)crc += *((uint8_t*)&param + i);
  Serial.printf("EEPROM Param CRC %u %u\n", crc, crc_save);
  param.CRC = crc_save;
  return crc;
}

/**
   Сброс конфиг в значения "по умолчанию"
*/
void eepromParamDefault() {
  param.INP0 = 0;
  param.INP1 = 0;
  param.INP2 = false;
  param.INP3 = false;
  param.OUT0 = false;
  param.OUT1 = false;
}
