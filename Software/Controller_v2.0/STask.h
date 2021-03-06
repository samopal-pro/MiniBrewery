#ifndef STASK_h
#define STASK_h
#include <SPIFFS.h>
#include <FS.h>
#include "src/SButton.h"
#include "src/SLed.h"
#include "src/WiFiManager.h"   
#include "src/MQTT/PubSubClient.h"
#include "src/PID_v1.h"
#include "src/ESP32_Servo.h"
#include "WC_Config.h"    
#include "WC_HTTP.h"
#include "WC_Classes.h"
#include "src/GFX/Adafruit_GFX.h"
#include "src/GFX/Adafruit_ILI9341.h"
#include "src/GFX/Fonts/FreeSans12pt7b.h";
#include "src/GFX/Fonts/FreeMonoBold12pt7b.h"
#include "src/GFX/Fonts/FreeMono18pt7b.h"
//#include "src/NTC/Thermistor.h"
//#include "src/NTC/NTC_Thermistor.h"
#define DEBUG_TASK

// Определение пинов контроллера
#define PIN_OUT0      25  
#define PIN_OUT1      26   
#define PIN_OUT2      27  
#define PIN_OUT3      14   
#define PIN_TRIAC0    13  //Тен
#define PIN_TRIAC1    12  //Мотор 

#define PIN_BIZZER    2

#define PIN_BTN0      0  
#define PIN_BTN1      32  
#define PIN_BTN2      33  

#define PIN_INP0      36
#define PIN_INP1      39
#define PIN_INP2      34
#define PIN_INP3      35

#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22

#define PIN_SERVO     15
//#define PIN_DIR       27
//#define PIN_EN        25

#define TFT_DC        17
#define TFT_CS        4
#define TFT_RST       16
#define TFT_MISO      19       
#define TFT_MOSI      23         
#define TFT_CLK       18
#define TFT_LED       5

enum PID_TYPE {
  PT_NONE  = -1,
  PT_OFF   = 0,
  PT_PAUSE   = 1000,
  PT_T0    = 1,
  PT_T1    = 2,
  PT_TC0   = 11, // Режим охлаждения
  PT_TC1   = 12, // Режим охлаждения
// Нагревания без контроля температуры 
// в tempPID записываентся процент нагрева 0-100  
  PT_POW0   = 21, // Отображается T1
  PT_POW1   = 22, // Отображается T2
// Слив пока датчик потока true
  PT_DRAIN  = 30,  
};

extern uint16_t tunerPID1, tunerPID2,tunerPID5;

#define STAGES_MAX 20
#define HOP_MAX    5
struct stageOfBrewing {
   String NAME;
   uint32_t TIMER; // Длительность варки, мс
   PID_TYPE PID;   // Настройка PID
   float T;        // Температура
   bool OUT0, OUT1, OUT2, OUT3, MOTOR;
//   uint8_t SERVO;  
   bool CHECKDP;     //Проверять воду
   bool TIMER_RESET; //Нужно ли сбрасывать таймер
   bool START_HOP; //Запустить задачу сброса хмеля по таймеру
};
extern uint16_t CountStages;
extern uint16_t CountHop;

struct stageOfHop {
   uint32_t TIMER; // Тамер
   uint8_t SERVO;   
};

// Определение глобальных переменных
extern float DT0, DT1;
extern bool DP1;
extern float TT1;
extern bool isMQTT,isPause,isAUTO;
extern PID_TYPE isPID,isPID_save;
extern float tempPID;
extern uint32_t timePID;
extern float proc,_temp;
extern int PID_WINDOW;

//extern PID myPID;
extern WiFiManager wifiManager;
extern SOutput Out0,Out1,Out2,Out3,Out4,Heater,Motor,Bizzer;
extern Adafruit_ILI9341 display;
extern int servoAngle[];
extern uint8_t servoPos;
extern uint32_t ms_pid, ms_start,ms_pause;
extern uint32_t ms_main_tm, ms_phase_tm;

void taskGetSensors( void *pvParameters ); 
void taskNetwork( void *pvParameters ); 
void taskButtons( void *pvParameters ); 
void taskPID( void *pvParameters ); 
void taskServo( void *pvParameters ); 
void taskBizzer( void *pvParameters ); 
void taskPivo( void *pvParameters ); 
void tasksStart();
float ntcRead( uint8_t pin );

void mqttReconnect();
void mqttPublishParam();
void mqttPublish(char *_topic, char *_text);

//void displaySensors();
//void displayMQTT(char *s);
void displayWiFi(char *s);
//void displayMode(char *s);
void displayIP();
//void displayTimer();
//void displayPID();
void semaphorePrint( char *s, bool ln = true);
uint32_t pidCalcWindow( float t_cur, float t_pid, uint32_t max_window );
void controlPause();
void listDir(const char * dirname, uint8_t levels = 0);
void stopTaskPivo();
void makeStages();
void makeStage(String _name, uint32_t _tm, PID_TYPE _pid, float _t, bool _o0, bool o1, bool o2, bool o3, bool _m, bool _dp, bool _reset_tm, bool _start_hop);
#endif
