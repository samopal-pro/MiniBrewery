#include <Ticker.h>
#include <Wire.h>
#include <driver/adc.h>
#include "esp_system.h"
#include <esp_wifi.h>
#include "STask.h"

const char *_SVERSION = PSTR("NANOBREWERY v1.0.2"); 

//#include "src/GyverPID.h"
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

WiFiManager wifiManager;

//Specify the links and initial tuning parameters
//GyverPID regulator(Kp, Ki, Kd, dt);  // коэф. П, коэф. И, коэф. Д, период дискретизации dt (мс)

//Ticker t_btn;
//Определение глобальных классов
//SButton btn0( PIN_BTN0 );
//SButton btn1( PIN_BTN1 );
////SButton btn2( PIN_BTN2 );

uint32_t ms, ms0=0, ms1=0, ms2=0;
//bool isPID  = false;

int  err_count = 0;

void initOut( uint8_t pin, uint16_t ms);
void configModeCallback (WiFiManager *myWiFiManager);
void statusCallback( WM_STATUS stat );

void setup() {
   delay(2000);
   Serial.begin(115200);
   Serial.println(_SVERSION);
   uint8_t mac[10];
// Инициализация файловой системы на внутренней памяти
   eepromBegin();
   eepromRead();
//   eepromDefault();
//   eepromSave();
//   Config.NET = NT_WIFI;
//   eepromParamRead();
// Проверка выходов
/*
   Out0.Impulse(250);
   Out1.Impulse(250);
   Out2.Impulse(250);
   Out3.Impulse(250);
//   Out4.Impulse(250);
   Heater.Impulse(250);
   Motor.Impulse(250);
   */
//  digitalWrite(PIN_BIZZER  ,HIGH);
//   Bizzer.Impulse(250);
//   delay(500);  
//   Bizzer.Impulse(250);  
//   delay(500);  
//   Bizzer.Impulse(250);  
///  digitalWrite(PIN_TRIAC0,HIGH);
///  digitalWrite(PIN_TRIAC1,HIGH);
///  digitalWrite(PIN_OUT0,HIGH);


//   Bizzer.Impulse(250);
   pinMode(PIN_BTN0,INPUT);  
   pinMode(PIN_BTN1,INPUT);  
   pinMode(PIN_BTN2,INPUT);  
// Инициализация кнопки   
//   btn2.SetLongClick(3000);   
//   btn2.SetAutoClick(20000,500);   

   Wire.begin(PIN_I2C_SDA,PIN_I2C_SCL);  //Сканирование I2C шины  
//   Wire.setClock(700000);
   Serial.println("Scan I2C ...");
   for(int i = 1; i < 127; i++ )  {
     Wire.beginTransmission(i);
     if(  Wire.endTransmission() == 0 ){
       Serial.print("Found I2C device 0x");
       Serial.println(i,HEX);
     }
  }

// Инициализация дисплея   
   pinMode(TFT_LED,OUTPUT);
   digitalWrite(TFT_LED,HIGH); 
   display.begin();
   display.setRotation(1);
   display.fillScreen(ILI9341_BLACK);  
//   display.drawRGBBitmap(120,0,logo,200,45);
   display.setTextColor(ILI9341_YELLOW);
   display.setTextSize(3);
   display.setCursor(20,2);
   display.print(_SVERSION);
  
// Инициализация WiFi
//   wifiManager.resetSettings();
   wifiManager.setAPCallback(configModeCallback);
   wifiManager.setStatusCallback(statusCallback);
//   wifiManager.setSTAStaticIPConfig(IPAddress(192,168,1,101),  
//      IPAddress(192,168,1,1), 
//      IPAddress(255,255,255,0), 
//      IPAddress(8,8,8,8));
   
// Старт всех параллельных задач
   tasksStart();   
//   displayMode("PID is OFF");
}



bool biz = false;

void loop(void) {
//   if( WiFi.status() == WL_CONNECTED )wifiManager.server->handleClient();
}





void statusCallback( WM_STATUS stat ){
  switch(stat){
     case WMS_OFF : 
        Serial.println(F("WiFi Off")); 
        displayWiFi(" OFF");
        break;
     case WMS_NOT_CONFIG : 
        displayWiFi(" NOT");
        Serial.println(F("WiFi Not Config")); 
        break;
     case WMS_AP_MODE : 
        displayWiFi(" AP");
        Serial.println(F("AP")); 
        break;
     case WMS_WAIT : 
        displayWiFi(" ...");
        Serial.println(F("WiFi wait to connect...")); 
        break;
     case WMS_ON: 
        displayWiFi(" ON");
        displayIP();
        Serial.println(F("WiFi On"));
        Serial.print("IP: ");
        Serial.print(WiFi.localIP());
        Serial.print(" MASK: ");
        Serial.print(WiFi.subnetMask());
        Serial.print(" GW: ");
        Serial.print(WiFi.gatewayIP());
        Serial.print(" DNS: ");
        Serial.println(WiFi.dnsIP());        
        break;
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}
