#include "WC_Config.h"
stringList config_ini("/config.ini"), param_ini("/param.ini");

//struct WC_CONFIG Config;
char TOPIC_PREFIX1[TOPIC_PREFIX_SIZE];
char TOPIC_PREFIX2[TOPIC_PREFIX_SIZE];
//struct INPUT_STATUS param;

/**
   Инициализация EEPROM
*/
void eepromBegin() {
//  Serial.println(F("EEPROM init"));
//  EEPROM.begin(sizeof(param) + sizeof(Config));
}

/**
   Читаем конфигурацию из EEPROM
*/
void eepromRead() {
  config_ini.load();
  if( config_ini.size() == 0 ){
     eepromDefault();
     eepromSave();      
  }
  config_ini.print();
}


/**
   Сохраняем конфигурацию в EEPROM
*/
void eepromSave() {
   config_ini.save();
}


/**
   Сброс конфиг в значения "по умолчанию"
*/
void eepromDefault() { 
  String id = "ESP" + ESP_getChipId();
  config_ini.clear();
  config_ini.set("AP_NAME",id);
  config_ini.set("AP_PASS","12345678");
//  config_ini.set("WIFI_NAME", "DIRLIRCA");
//  config_ini.set("WIFI_PASS", "Lirca2016");
  config_ini.set("WIFI_NAME", "DIR-320");
  config_ini.set("WIFI_PASS", "sav59vas");

  config_ini.set("MQTT_SERVER","5.101.66.67");
  config_ini.set("MQTT_PORT","1883");
  config_ini.set("MQTT_USER","ellips");
  config_ini.set("MQTT_PASS","ell123ips");
  config_ini.set("MQTT_TM","30000");
/*
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
  */
}

/**
   Читаем конфигурацию из EEPROM
*/
void eepromParamRead() {
  param_ini.load();
  if( param_ini.size() == 0 ){
     eepromParamDefault();
     eepromParamSave();      
  }
  param_ini.print();
}


/**
   Сохраняем конфигурацию в EEPROM
*/
void eepromParamSave() {
  param_ini.save();
}

/**
   Сброс конфиг в значения "по умолчанию"
*/
void eepromParamDefault() {
   param_ini.clear();
   param_ini.set("PREHEATING","40");
   param_ini.set("PAUSE1","20");
   param_ini.set("PAUSE_TEMP1","55");
   param_ini.set("PAUSE2","75");
   param_ini.set("PAUSE_TEMP2","65");
   param_ini.set("PAUSE3","30");
   param_ini.set("PAUSE_TEMP3","73");
   param_ini.set("PAUSE4","5");
   param_ini.set("PAUSE_TEMP4","78");
   param_ini.set("BOILING","60");
   param_ini.set("HOP1","5");
   param_ini.set("HOP2","45");
   param_ini.set("HOP3","45");
   param_ini.set("COOL_TEMP","28"); 
}

stringList::stringList( char *_file_name ){
   strncpy(FileName,_file_name,63);
   Serial.printf("SL: Start %s\n",FileName);
   Strings.clear();
   load();
}

int stringList::getIndex( String _name ){
  for( int i=0; i<Strings.size();i++){
     if( Strings[i].startsWith(_name) )return i; 
  }
  return -1;
}

String stringList::get( String _name, String _def ){
  int i = getIndex( _name );
  if( i<0 ) return _def;
  else return Strings[i].substring(_name.length()+1);
}

bool stringList::set(String _name,String _val){
  int i = getIndex( _name );
  String s = _name+"="+_val;
  if( i < 0 ){
    Strings.push_back(s);
    return false;
  }
  else {
    Strings[i] = s;
    return true;
  }
}

void stringList::print(){
  for( int i=0; i<Strings.size();i++){
//    for( int j=0; j<Strings[i].length(); j++ ){
//      Serial.printf("0x%02x ",Strings[i][j]);
//    }
    Serial.println(Strings[i]);
  }
}

void stringList::load(){
   File f = SPIFFS.open(FileName, FILE_READ);
   if( f == NULL )return;
   Serial.printf("SL: Read %s strings\n",FileName);
   while( f.available() ){
       String s = f.readStringUntil('\n');
       s.replace(" ","");
       s.replace("\n","");
       s.replace("\r","");
       Strings.push_back(s);
   }
   f.close();
}

void stringList::save(){
   File f = SPIFFS.open(FileName, FILE_WRITE);
   if( f == NULL )return;
   Serial.printf("SL: Save to %s %d strings\n",FileName,Strings.size());
   for( int i=0; i<Strings.size();i++){
     f.print(Strings[i]);
     f.print("\n");
   }
   f.close();  
}

void stringList::clear(){ Strings.clear(); }

int stringList::size(){ return Strings.size(); }
