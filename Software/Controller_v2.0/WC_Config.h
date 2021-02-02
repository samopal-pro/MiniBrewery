#ifndef WC_CONFIG_h
#define WC_CONFIG_h
#include <EEPROM.h>
#include <arduino.h>
#include <SPIFFS.h>
#include <FS.h>
#include "src/WiFiManager.h"   

void eepromBegin();
void eepromDefault();
void eepromRead();
void eepromSave();
void eepromParamDefault();
void eepromParamRead();
void eepromParamSave();


//extern  struct WC_CONFIG Config;
//extern struct INPUT_STATUS param;
#define TOPIC_PREFIX_SIZE 64
extern char TOPIC_PREFIX1[];
extern char TOPIC_PREFIX2[];

class stringList {
  private:
     std::vector <String> Strings;
     char FileName[64];
     int getIndex(String _name);
  public:
     stringList(char *_filename);
     void load();
     void save();
     void print();
     void clear();
     int size();
     String get(String _name, String _def="");
     bool set(String _name, String _value);       
};
extern stringList config_ini, param_ini;


#endif
