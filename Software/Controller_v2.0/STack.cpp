#include "STask.h"
// Глобальные переменные
float DT0=-127, DT1=-127;
SButton btn0( PIN_BTN0 );
SButton btn1( PIN_BTN1 );
SButton btn2( PIN_BTN2 );
SLed bizzer(PIN_BIZZER);

SOutput Out0(PIN_OUT0);
SOutput Out1(PIN_OUT1);
SOutput Out2(PIN_OUT2);
SOutput Out3(PIN_OUT3);
SOutput Heater(PIN_TRIAC0);
SOutput Motor(PIN_TRIAC1);
//SOutput Bizzer(PIN_BIZZER);
bool DP1     = false;
float TT1    = 0;
bool isMQTT  = false;
bool isAP    = false;
bool isAUTO  = false;
bool isAlarm = false;
bool isPause = false;
bool isCheckDP = false;
bool servoTimer = false;
uint32_t msCheckDP = 0;
PID_TYPE isPID  = PT_OFF, isPID_save = PT_NONE;
// PID регулятор
float tempPID;
float proc,_temp;
uint32_t timePID = 600;
int PID_WINDOW = 2000;
uint32_t ms_pid =0, ms_start=0, ms_pause = 0;
uint32_t ms_main_tm = 0, ms_phase_tm = 0;

struct stageOfBrewing *stage;
struct stageOfBrewing stages[STAGES_MAX];
uint16_t CountStages = 0;

struct stageOfHop hops[HOP_MAX];
uint16_t CountHop = 0;
//uint16_t tunerPID1 = 60;
//uint16_t tunerPID2 = 75;
//uint16_t tunerPID5 = 100;

int servoAngle[6] = {5, 56, 105, 155, 155, 155};
uint8_t servoPos  = 0;
WiFiClient mqttWiFiClient;
PubSubClient mqttClient(mqttWiFiClient);
SemaphoreHandle_t serialSemaphore, lcdSemaphore;
TaskHandle_t sensorHandle, pidHandle, servoHandle, pivoHandle;
xQueueHandle btnQueue, pivoQueue;

TickType_t serialTick = 200, lcdTick = 200;
/**
 * Старт всех параллельных задач
 */
void tasksStart(){
   SPIFFS.begin(true);
   listDir("/");
   eepromBegin();
   eepromRead();
   eepromDefault();
   eepromSave();
   eepromParamRead();
   
   btnQueue  = xQueueCreate(sizeof(uint8_t), 10);
   pivoQueue = xQueueCreate(sizeof(uint8_t), 10);
   serialSemaphore = xSemaphoreCreateMutex();
   lcdSemaphore    = xSemaphoreCreateMutex();
   xTaskCreateUniversal(taskGetSensors, "get_sensors", 2048, NULL, 3, &sensorHandle,1);   
   xTaskCreateUniversal(taskNetwork, "mqtt", 4048, NULL, 2, NULL,1);   
   xTaskCreateUniversal(taskButtons, "buttons", 2048, NULL, 3, NULL,1);   
   xTaskCreateUniversal(taskPID, "pid", 2048, NULL, 2, &pidHandle,1);   
   xTaskCreateUniversal(taskServo, "servo", 2048, NULL, 3, &servoHandle,1);   
   xTaskCreateUniversal(taskBizzer, "bizzer", 2048, NULL, 3, NULL,1);   
}


/**
 * Обработчики прерывания по нажатию кнопки
 */
void IRAM_ATTR ISR_btn0(){
   ets_intr_lock();
   uint8_t ret = (uint8_t)btn0.Loop(); 
   if( ret != SB_NONE ){
       ret |= 0x10;
       xQueueSendFromISR( btnQueue, &ret, 0 );
   }
   ets_intr_unlock();      
}

void IRAM_ATTR ISR_btn1(){
   ets_intr_lock();
   uint8_t ret = (uint8_t)btn1.Loop(); 
   if( ret != SB_NONE ){
       ret |= 0x20;
       xQueueSendFromISR( btnQueue, &ret, 0 );
   }
   ets_intr_unlock();      
}

void IRAM_ATTR ISR_btn2(){
   ets_intr_lock();
   uint8_t ret = (uint8_t)btn2.Loop(); 
   if( ret != SB_NONE ){
       ret |= 0x30;
       xQueueSendFromISR( btnQueue, &ret, 0 );
   }
   ets_intr_unlock();      
}

/**
 * Задача опроса датчиков температуры и потока
 */
void taskBizzer( void *pvParameters ){ 
   semaphorePrint("Start Bizzer Task");
   bizzer.SeriesBlink(2,100);
   while(true){
       bizzer.Loop();
       vTaskDelay(250);
   }
}
/**
 * Задача опроса датчиков температуры и потока
 */
void taskGetSensors( void *pvParameters ){
  
   semaphorePrint("Start GetSensors Task");
// Инициализация АЦП
   analogReadResolution(12);   
   analogSetCycles(255);
   SInputNTC ntc0(PIN_INP0,10000,4092,0,1.024);
   SInputNTC ntc1(PIN_INP1,10000,4092,0,1.024);
   SFlowSensor flow0(2200,10,3,7);
// Определение NTC сенсоров
//   Thermistor ntc0(PIN_INP0,10000,100000,25,3950,4095);
   
// Цикл опроса датчиков   
   while( true ){
// Опрос АЦП      
      DT0 = ntc0.GetTemperature();
      DT1 = ntc1.GetTemperature();
      uint16_t a2 = analogRead(PIN_INP2);
      uint16_t a3 = analogRead(PIN_INP3);
      uint8_t _flow0 = flow0.Set(a2);
      if( _flow0 != 255 )DP1 = (bool)_flow0;
      if( isCheckDP ){
         if( millis() - msCheckDP > 10000 ){
            if( DP1 == false  ){
               msCheckDP = millis();
               isAlarm = true;
               
               bizzer.SetBlinkMode(0B10101010); 
               Serial.println("Flow Sensor Alarm ON");
            }
            else {
               msCheckDP = millis();
               isAlarm = false;
               bizzer.SetBlinkMode(0B00000000); 
              Serial.println("Flow Sensor Alarm OFF");
            }
         }
      }
      if( xSemaphoreTake( lcdSemaphore, portMAX_DELAY ) ){
         display.fillRect(0,90,320,22,ILI9341_BLACK);      
         display.setTextColor(ILI9341_RED);
         display.setTextSize(3);
         display.setCursor(1,90);
         display.print("T1:");
         display.print(DT0,1);
         display.setCursor(160,90);
         display.print("T2:");
         display.print(DT1,1);
         display.fillRect(0,180,160,22,ILI9341_BLACK);      
         display.setTextColor(ILI9341_WHITE);
         display.setTextSize(3);
         display.setCursor(1,180);
         if( DP1 )display.print("DP1:ON");
         else display.print("DP1:OFF");
         xSemaphoreGive( lcdSemaphore );
      }
#ifdef DEBUG_TASK      
      if( xSemaphoreTake( serialSemaphore, serialTick ) ){
         Serial.print("ADC0=");
         Serial.print(a2);
         Serial.print(",ADC1=");
         Serial.print(a3);
         Serial.print(",DT0=");
         Serial.print(DT0,1);
         Serial.print(",DT1=");
         Serial.print(DT1,1);
         Serial.print(",DP1=");
         Serial.println((int)DP1);
         xSemaphoreGive( serialSemaphore );
      }
#endif      
//         displaySensors();
         vTaskDelay(1000);
   }
  
}


/**
 * Задача проверки подключение к MQTT и отправки параметров на сервер
 */
void taskNetwork( void *pvParameters ){
   semaphorePrint("Start MQTT Task");
   uint16_t Count = 0;
   while(true){
      if( Count%10 == 0 ){
        if( isAP ){
///          vTaskSuspend( sensorHandle ); 
///          vTaskSuspend( pidHandle ); 
///          vTaskSuspend( servoHandle ); 
          if( WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_OFF ){
///               wifiManager.setConfigPortalTimeout(300);
               displayWiFi("AP");
//               displayMQTT("Off");
               displayIP();
               isMQTT = false;
               wifiManager.startConfigPortal(config_ini.get("AP_NAME").c_str(),config_ini.get("AP_PASS").c_str(),HTTP_begin);
               WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
               WiFi.mode(WIFI_OFF);
//               displayMode("");
           } 
           else {
               WiFi.mode(WIFI_OFF);
//               displayMode("");                 
           }
           isAP = false;
///           vTaskResume( sensorHandle );
///           vTaskResume( pidHandle );
///           vTaskResume( servoHandle );
      }
      else {
         Serial.printf("%s %s\n",config_ini.get("WIFI_NAME").c_str(),config_ini.get("WIFI_PASS").c_str());
         wifiManager.checkWiFi(config_ini.get("WIFI_NAME").c_str(),config_ini.get("WIFI_PASS").c_str());
///         if( WiFi.status() == WL_CONNECTED )mqttReconnect();
///         mqttPublishParam();
      }
     }
     Count++;
     if( WiFi.status() == WL_CONNECTED ){
        if( wifiManager.isHttp == false )wifiManager.httpStart(HTTP_begin);
        wifiManager.server->handleClient();
     }
     vTaskDelay(500);
   }
  
}

/**
 * Задача работы с кнопками
 */
void taskButtons( void *pvParameters ){
   semaphorePrint("Start Button Task");
// Инициализация кнопок   
   btn1.SetLongClick(2000);   
   btn2.SetLongClick(2000);   
//   btn2.SetMultiClick(500);
//   btn2.SetAutoClick(20000,500);   
   attachInterrupt(digitalPinToInterrupt(PIN_BTN0), ISR_btn0, CHANGE);   
   attachInterrupt(digitalPinToInterrupt(PIN_BTN1), ISR_btn1, CHANGE);   
   attachInterrupt(digitalPinToInterrupt(PIN_BTN2), ISR_btn2, CHANGE);   
   while(true){
      uint8_t ret;
// Вычитываем все значение из очереди и обрабатываем последнее
      while( true ){
// Ожидаем, когда значение срабатывания кнопки запишет а очереди обработчик прерывания      
         xQueueReceive( btnQueue, &ret, portMAX_DELAY);
         if( uxQueueMessagesWaiting(btnQueue) == 0 )break;
      }
      switch( ret&0x0f ){
         case SB_CLICK :
            Serial.printf("BTN 0x%x click\n",ret&0xf0);
            if( isAlarm ){
               msCheckDP = millis();
               isAlarm = false;
               bizzer.SetBlinkMode(0B00000000); 
               Serial.println("Flow Sensor Alarm OFF");
            }
            if((ret&0xf0) == 0x30)controlPause();
            break;
         case SB_LONG_CLICK :
            if( (ret&0xf0) == 0x30  ){
               Serial.printf("BTN2 long %d\n",btn2.Count);
               if( isAP == true ){
                  wifiManager.httpLoopBreak = true;;
               }
               else {
                  isAP = true;
               }               
            }
            if( (ret&0xf0) == 0x20 && isAUTO )stopTaskPivo();
            Serial.printf("BTN 0x%x long click\n",ret&0xf0);
            break;
         case SB_AUTO_CLICK :
            Serial.printf("BTN 0x%x auto click\n",ret&0xf0);
            break;
      }

    
       vTaskDelay(100);
   }
  
}

/**
 * Задача обслуживающая PID регулятор
 */
void taskPID( void *pvParameters ){
   semaphorePrint("Start PID Task");
// Настройка PID   
   isPID   = PT_OFF; 
   tempPID = 60;
   float temp = 0;
   while( true ){
// Устанавливаем температуру для регулирования    
       switch( isPID ){
          case PT_POW0 :
          case PT_T0   :
          case PT_TC0  :temp = DT0;break;
          case PT_POW1 :
          case PT_T1   :
          case PT_TC1  :temp = DT1;break;
       }
       
// Если поменялся статус PID    
       if( isPID != isPID_save ){
          if( xSemaphoreTake( lcdSemaphore, portMAX_DELAY ) ){
             display.fillRect(0,120,320,22,ILI9341_BLACK);  
             if( isPID == PT_OFF )display.setTextColor(ILI9341_WHITE);
             else display.setTextColor(ILI9341_RED);
             display.setTextSize(3);
             display.setCursor(1,120);
             String pidMode = "";
             if( isPID == PT_OFF ){
                 display.print("PID:OFF");
                 display.fillRect(0,150,320,22,ILI9341_BLACK);
                 pidMode = "OFF";
                 bizzer.Off(); 
                 Heater.Set(false);                 
             }
             if( isPID == PT_PAUSE ){
                 display.print("PAUSE");
                 display.fillRect(0,150,320,22,ILI9341_BLACK);
                 pidMode = "OFF";
                 bizzer.SeriesBlink(2,100); 
                 Heater.Set(false);                 
             }
             else if( isnan(temp) || temp < 0 || temp > 110 ){
                 display.print("HEAT:ERROR");   
                 pidMode = "ERROR";
                 isPID = PT_OFF;               
             }
             else {
                 switch( isPID ){
                     case PT_T0:    pidMode = "HEAT:T1"; break; 
                     case PT_T1:    pidMode = "HEAT:T2"; break; 
                     case PT_TC0:   pidMode = "COOL:T1"; break; 
                     case PT_TC1:   pidMode = "COOL:T2"; break; 
                     case PT_POW0:  pidMode = "POW:T1 "; break; 
                     case PT_POW1:  pidMode = "POW:T2";  break; 
                     case PT_DRAIN: pidMode = "DRAIN";   break; 
                }
                display.print(pidMode);
                display.setCursor(160,120);
                display.print(tempPID,1);
                bizzer.SeriesBlink(1,100);
                ms_pid = 0;
                ms_start = millis();
            }
            isPID_save = isPID;
 //           Serial.printf("PID mode = %d %s\n",isPID,pidMode); 
            xSemaphoreGive( lcdSemaphore );
          }
       } //end if(isPID != isPID_save)
// Нсли PID включен       
       if( isPID != PT_OFF && isPID != PT_PAUSE ){
// Определяем размер окон работы симистора          
          uint32_t ON_WINDOW=0, OFF_WINDOW=PID_WINDOW; 
          if( isPID == PT_DRAIN ){
             OFF_WINDOW  = PID_WINDOW;
             ON_WINDOW = 0;               
             if( DP1 == false )ms_pid = millis(); 
          }
// Режим охлаждения до температуры          
          else if( isPID == PT_TC0 || isPID == PT_TC1 ){
             OFF_WINDOW  = PID_WINDOW;
             ON_WINDOW = 0;               
             if(  temp - tempPID  <= 0.5 && ms_pid == 0 )ms_pid = millis(); 
//             if( ms_pid == 0 )ms_pid = millis();        
          }
// Режим нагрева без контроля температуры          
          else if( isPID == PT_POW0 || isPID == PT_POW1 ){
             if( tempPID <0 || tempPID > 100 )tempPID = 0;
             ON_WINDOW = PID_WINDOW * tempPID / 100;
             OFF_WINDOW = PID_WINDOW - ON_WINDOW;  
             if( ms_pid == 0 )ms_pid = millis();              
          }
// Режим нагрева с регулированием температуры          
          else {
             ON_WINDOW  = pidCalcWindow( temp, tempPID, PID_WINDOW );
             OFF_WINDOW = PID_WINDOW - ON_WINDOW;  
             if(  tempPID - temp  <= 0.5 && ms_pid == 0 )ms_pid = millis(); 
          }
// Если разница меньше 0.5 запускаем таймер
          if( abs( tempPID - temp ) <= 0.5 && ms_pid == 0 )ms_pid = millis(); 
          proc = ON_WINDOW * 100 / PID_WINDOW;
          _temp = temp;
          Serial.print("PID: Set T=");
          Serial.print(tempPID,1);
          Serial.print(",T=");
          Serial.print(temp,1);
          Serial.print(" ");
          Serial.print(proc,0);
          Serial.print("% ");
// Отрабатываем окна работы симистора          
          if( ON_WINDOW > 0 ){
            if( Heater.Stat() == false )Heater.Set(true);
            vTaskDelay(ON_WINDOW);
          }
          if( OFF_WINDOW > 0 ){
            if( Heater.Stat() == true )Heater.Set(false);
            vTaskDelay(OFF_WINDOW);
          }
// Проверяем, если время таймера вышло          
          if( ms_pid!=0 && (millis()-ms_pid)/1000 > timePID ){
             bizzer.DoubleBlink();
             if( isAUTO ){
                 isPID = PT_OFF;
                 uint8_t ret = 1;
                 // Спускаем очередь 
                 xQueueSend( pivoQueue, &ret, 0 );
             }
          }
          else bizzer.Off();
// Выдаем информацию по таймерам на дисплей
          if( xSemaphoreTake( lcdSemaphore, portMAX_DELAY ) ){
             display.fillRect(0,150,320,22,ILI9341_BLACK);      
             display.setTextColor(ILI9341_WHITE);
             display.setTextSize(3);
             display.setCursor(1,150);
             uint32_t t=(millis()-ms_start)/1000;
             char s[10];
             sprintf(s,"%02d:%02d",(int)(t/60),(int)(t%60));
             Serial.printf(" %s",s);
             display.print(s);
             display.setCursor(160,150);
             if( ms_pid > 0 ){
                t=(millis()-ms_pid)/1000+ms_pause;
                sprintf(s,"%02d:%02d",(int)(t/60),(int)(t%60));
                display.print(s);            
                Serial.printf(" %s",s);
             }
             else display.print("00:00"); 
             xSemaphoreGive( lcdSemaphore );
          }
          Serial.println("");
       }
       else {
            vTaskDelay(PID_WINDOW);        
       }
   }
}


/**
 * Соединение с MQTT серввером
 */
void mqttReconnect(){ 
      if( !mqttClient.connected() ){
          isMQTT = false;   
//          displayMQTT(" Off");
          if( xSemaphoreTake( serialSemaphore, serialTick ) ){
             Serial.printf("RSSI=%d\n",WiFi.RSSI());
             Serial.printf("MQTT connect %s %d %s %s %s\n",
                config_ini.get("MQTT_SERVER").c_str(),
                config_ini.get("MQTT_PORT").toInt(),
                config_ini.get("AP_NAME").c_str(),
                config_ini.get("MQTT_NAME").c_str(),
                config_ini.get("MQTT_PASS").c_str());
             xSemaphoreGive( serialSemaphore );
          }
          mqttClient.setServer(config_ini.get("MQTT_SERVER").c_str(),config_ini.get("MQTT_PORT").toInt()); 
          if( mqttClient.connect(config_ini.get("AP_NAME").c_str(),config_ini.get("MQTT_NAME").c_str(),config_ini.get("MQTT_PASS").c_str() ) ){
//          if( mqttClient.connected() ){
//              displayMQTT(" ON");         
              semaphorePrint("MQTT ON");
          }
      }  
      else {
          semaphorePrint("MQTT is connect");
//          displayMQTT(" ON");
          isMQTT = true;   
      }
}

void mqttPublishParam(){
  char _topic[64];
  char _text[64];
//  if( Config.NET != NT_WIFI
  if( !isMQTT )return;
  sprintf(_topic,"%s/%s",TOPIC_PREFIX1,"DT0");
  sprintf(_text,"%d.%01d",(int)DT0,(int)(DT0*10)%10);
  mqttPublish(_topic,_text);
  sprintf(_topic,"%s/%s",TOPIC_PREFIX1,"DT1");
  sprintf(_text,"%d.%01d",(int)DT1,(int)(DT1*10)%10);
  mqttPublish(_topic,_text);
  sprintf(_topic,"%s/%s",TOPIC_PREFIX1,"DP0");
  sprintf(_text,"%d",(int)DP1);
  mqttPublish(_topic,_text);
  sprintf(_topic,"%s/%s",TOPIC_PREFIX1,"T0");
  sprintf(_text,"%d.%01d",(int)TT1,(int)(TT1*10)%10);
  mqttPublish(_topic,_text);
   
}

void mqttPublish(char *_topic, char *_text){
  bool _stat = mqttClient.publish(_topic,_text);
  if( xSemaphoreTake( serialSemaphore, serialTick ) ){
     Serial.printf("MQTT publish: %s %s %d\n",_topic,_text,(int)_stat);
     xSemaphoreGive( serialSemaphore );
  }
  
}


/**
 * Отображение значений сенсоров в 4-й строчке дисплея
 */
/* 
void displaySensors(){
  if( xSemaphoreTake( lcdSemaphore, lcdTick ) ){
     lcd.setCursor(0,3);
     lcd.print("                    ");
     lcd.setCursor(0,3);
     lcd.print(DT0,1); 
     lcd.print("  ");
     lcd.print(DT1,1); 
     lcd.print("  ");
     if( DP1 )lcd.print("ON");
     else lcd.print("OFF");
     xSemaphoreGive( lcdSemaphore );
  }
}
*/
/**
 * Отображение состояния MQTT
 */
 
/*void displayMQTT(char *s){
  if( xSemaphoreTake( lcdSemaphore, lcdTick ) ){
     lcd.setCursor(10,1);
     lcd.print("MQTT:     ");
     lcd.setCursor(15,1);
     lcd.print(s); 
     xSemaphoreGive( lcdSemaphore );
  }
}
*/
void displayWiFi(char *s){
  if( xSemaphoreTake( lcdSemaphore, portMAX_DELAY ) ){
       display.fillRect(0,40,160,16,ILI9341_BLACK);      
       display.setTextColor(ILI9341_GREEN);
       display.setTextSize(2);
       display.setCursor(1,40);
       display.print("WiFi:");
       display.print(s);
     xSemaphoreGive( lcdSemaphore );
  }
}

void displayIP(){
  if( xSemaphoreTake( lcdSemaphore, portMAX_DELAY ) ){
       display.fillRect(0,56,320,16,ILI9341_BLACK);      
       display.setTextColor(ILI9341_GREEN);
       display.setTextSize(2);
       display.setCursor(1,56);
       display.print("http://");
       if( isAP )display.print(WiFi.softAPIP());
       else display.print(WiFi.localIP()); 
     xSemaphoreGive( lcdSemaphore );
  }
}
/*
void displayMode(char *s){
  if( xSemaphoreTake( lcdSemaphore, lcdTick ) ){
     lcd.setCursor(0,2);
     lcd.print("                    ");
     lcd.setCursor(0,2);
     lcd.print(s); 
     xSemaphoreGive( lcdSemaphore );
  }
}
*/
void semaphorePrint( char *s, bool ln ){
  if( xSemaphoreTake( serialSemaphore, serialTick ) ){
     if(ln)Serial.println(s);
     else Serial.print(s);
     xSemaphoreGive( serialSemaphore );
  }
  
}

/**
 * Задача работы с сервоприводом
 */
void taskServo( void *pvParameters ){
   vTaskDelay(1000); // Ждем остальные задачи
   semaphorePrint("Start Servo Task");
   Servo   myServo;
/*
   myServo.attach(PIN_SERVO);
   myServo.write(0);
   vTaskDelay(5000);
   myServo.write(30);
   vTaskDelay(5000);
   myServo.write(58);
   vTaskDelay(5000);
   myServo.write(84);
   vTaskDelay(5000);
   myServo.write(111);
   vTaskDelay(5000);
   myServo.write(139);
   vTaskDelay(5000);
   */
   servoPos = 0;
   uint8_t servoSave = 255;
   uint32_t _ms, _ms0 = 0;
//   30, 58, 84, 111, 139, 166
   while( true ){
       if( isAUTO && servoTimer && !isPause && ms_pid > 0  ){
          _ms = millis();
          if( _ms0 == 0 || _ms0 > _ms || (_ms-_ms0)>=10000 ){
             _ms0 = _ms;
             if( servoPos < CountHop ){
                uint32_t t_hop = hops[CountHop].TIMER*60;
                uint32_t t=(millis()-ms_pid)/1000+ms_pause;
                if( t > t_hop )servoPos++;
                if( servoPos == 1 && t > (t_hop+120) && t <= (t_hop+180) )tempPID = 0;
                else tempPID = stage->T;
             }
          }
       }
    
       if( servoPos >= 6 )servoPos = 0;
       if( servoPos != servoSave ){
          if( xSemaphoreTake( lcdSemaphore, portMAX_DELAY ) ){
             display.fillRect(160,180,160,22,ILI9341_BLACK);      
             display.setTextColor(ILI9341_WHITE);
             display.setTextSize(3);
             display.setCursor(160,180);
             display.print("POS:");
             display.print(servoPos);
             xSemaphoreGive( lcdSemaphore );
          }
          semaphorePrint("Start Servo");
          myServo.attach(PIN_SERVO);
//          myServo.write((int)timePID);
          myServo.write(servoAngle[servoPos]);
          vTaskDelay(3000); 
          myServo.detach();
          semaphorePrint("Stop Servo");
          servoSave = servoPos;
       }
       vTaskDelay(1000);
   }
}

void taskPivo( void *pvParameters ){
  makeStages();
  isAUTO = true;
  servoTimer = false;
  ms_main_tm = millis();
  uint8_t ret;
  for( int i=0;i<CountStages;i++){
     if( !isAUTO )break;    
     stage = &stages[i]; 
     Serial.printf("Step #%d %s\n",i, stage->NAME.c_str());
     isCheckDP = stage->CHECKDP;
     msCheckDP = millis();
     isAlarm   = false;
     timePID = stage->TIMER;
     tempPID = stage->T;
     isPID   = stage->PID;     
     isPID_save = PT_NONE;
     Out0.Set(stage->OUT0);     
     Out1.Set(stage->OUT1);     
     Out2.Set(stage->OUT2);     
     Out3.Set(stage->OUT3);    
     Motor.Set(stage->MOTOR); 
     servoPos = 0; 
     servoTimer = stage->START_HOP;  
//     if( 
//     servoPos = Brewing[i].SERVO; 
//     if( timePID == 0 )break;
     xQueueReceive( pivoQueue, &ret, portMAX_DELAY);
  }
  isPID = PT_OFF; 
  Out0.Set(LOW);     
  Out1.Set(LOW);     
  Out2.Set(LOW);     
  Out3.Set(LOW);    
  Motor.Set(LOW);    
  Heater.Set(false);
  isAUTO = false;
  servoPos = 0; 
  servoTimer = false;
  ms_main_tm  = 0;
  ms_phase_tm = 0;
  Serial.println("Stop task PIVO");


  
  vTaskDelete( NULL ); 
}

void stopTaskPivo(){
  uint8_t ret = 0;
  isAUTO = false;
  xQueueSend( pivoQueue, &ret, 0 );  
}

uint32_t pidCalcWindow( float t_cur, float t_pid, uint32_t max_window ){
// Высчитываем температурный коэффициент
   float kt = (t_pid - 40)/60;
   float tune_min = 0, tune_max = 0;
   if( kt < 0 )kt = 0;
   if( kt > 1 )kt = 1; 
   if( t_pid - t_cur > 5.0 )return max_window;
   else if( t_pid - t_cur > 2.0 ){
      tune_min = 0.6;
      tune_max = 0.9;       
   }
   else if( t_pid - t_cur > 1.0 ){ 
      tune_min = 0.4;
      tune_max = 0.8;       
    
   }
   else if( t_pid - t_cur > 0.0 ){
      tune_min = 0.3;
      tune_max = 0.7;
   }
   else return (uint32_t)0;
   float tune = tune_min + ( tune_max - tune_min )*kt;
   uint32_t win = (uint32_t )( (float)max_window * tune );
   return win;
}

/**
 * Включить/выключить паузу
 */
void controlPause(){
   if( !isAUTO )return;
   if( isPause == false ){
      isPause = true;
      Motor.Set(false);
      Heater.Set(false);
      isPID   = PT_PAUSE; 
      if( ms_pid > 0 ){
         ms_pause += (millis()-ms_pid)/1000;
         timePID -= ms_pause;
      }
      Serial.println("Pause is ON");
   }
   else{
      isPause = false;
      Motor.Set(stage->MOTOR);
      isPID   = stage->PID; 
      ms_pid  = 0;
      Serial.println("Pause is OFF");
   }
}

void listDir(const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = SPIFFS.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
} 

void makeStages(){
   uint32_t tm;
   float t;
   CountStages = 0;
// Проверка воды
   makeStage("Water Test",120,PT_POW1,0.0,false,true,false,false,true,true,true,false);
// Предварительный нагрев воды
   makeStage("Preheating",1,PT_T1,50.0,false,true,false,false,true,true,true,false);
// Пауза 1
   if( param_ini.get("PAUSE1") != "" ){
      tm = ((uint32_t)param_ini.get("PAUSE1").toInt())*60;
      t = param_ini.get("PAUSE_TEMP1").toFloat();
      makeStage("Pause1",tm,PT_T0,t,true,false,false,false,true,true,true,false);
   }
// Пауза 2
   if( param_ini.get("PAUSE2") != "" ){
      tm = ((uint32_t)param_ini.get("PAUSE2").toInt())*60;
      t = param_ini.get("PAUSE_TEMP2").toFloat();
      makeStage("Pause2",tm,PT_T0,t,true,false,false,false,true,true,true,false);
   }
// Пауза 3
   if( param_ini.get("PAUSE3") != "" ){
      tm = ((uint32_t)param_ini.get("PAUSE3").toInt())*60;
      t = param_ini.get("PAUSE_TEMP3").toFloat();
      makeStage("Pause3",tm,PT_T0,t,true,false,false,false,true,true,true,false);
   }
// Пауза 4
   if( param_ini.get("PAUSE4") != "" ){
      tm = ((uint32_t)param_ini.get("PAUSE4").toInt())*60;
      t = param_ini.get("PAUSE_TEMP4").toFloat();
      makeStage("Pause4",tm,PT_T0,t,true,false,false,false,true,true,true,false);
   }
// Пауза 5
   if( param_ini.get("PAUSE5") != "" ){
      tm = ((uint32_t)param_ini.get("PAUSE5").toInt())*60;
      t = param_ini.get("PAUSE_TEMP5").toFloat();
      makeStage("Pause5",tm,PT_T0,t,true,false,false,false,true,true,true,false);
   }
// Пауза 6
   if( param_ini.get("PAUSE6") != "" ){
      tm = ((uint32_t)param_ini.get("PAUSE6").toInt())*60;
      t = param_ini.get("PAUSE_TEMP6").toFloat();
      makeStage("Pause6",tm,PT_T0,t,true,false,false,false,true,true,true,false);
   }
// Начало кипячения, сброс остатка сусла в трубке
   makeStage("Pipe Cleaning",120,PT_POW1,100.0,true,false,false,false,true,true,true,false);
// Нагрев до 100C
   makeStage("Heating 100C",1,PT_T1,100.0,false,false,false,false,false,true,true,false);
// Кипячение    
   uint32_t tm_boil = ((uint32_t)param_ini.get("BOILING").toInt())*60;
   makeStage("Boiling",tm_boil,PT_POW1,50.0,false,false,false,false,false,true,true,true);
// Охлаждение   
   t = param_ini.get("COOL_TEMP").toFloat();
   makeStage("Cooling",1,PT_TC1,t,false,true,false,true,true,true,true,false);
// Слив по датчику   
   makeStage("Drain",1,PT_DRAIN,0,false,false,true,true,true,false,true,false);
// Слить остатки 1 минута   
   makeStage("Drain All",60,PT_POW1,0,false,false,true,true,true,false,true,false);
// Сформировать рсписание по сбросу хмеля
   CountHop = 0;
   if( param_ini.get("HOP1") != "" ){
      tm = ((uint32_t)param_ini.get("HOP1").toInt())*60;
      if( tm < tm_boil ){
         hops[CountHop].TIMER = tm;
         hops[CountHop].SERVO = CountHop+1;
         CountHop++;
      }   
   } 
   if( param_ini.get("HOP2") != "" ){
      tm = ((uint32_t)param_ini.get("HOP2").toInt())*60;
      if( tm < tm_boil ){
         hops[CountHop].TIMER = tm;
         hops[CountHop].SERVO = CountHop+1;
         CountHop++;
      }   
   } 
   if( param_ini.get("HOP3") != "" ){
      tm = ((uint32_t)param_ini.get("HOP3").toInt())*60;
      if( tm < tm_boil ){
         hops[CountHop].TIMER = tm;
         hops[CountHop].SERVO = CountHop+1;
         CountHop++;
      }   
   } 
   
}

void makeStage(String _name, uint32_t _tm, PID_TYPE _pid, float _t, bool _o0, bool _o1, bool _o2, bool _o3, bool _m, bool _dp, bool _reset_tm, bool _start_hop){
   stages[CountStages].NAME        = _name;
   stages[CountStages].TIMER       = _tm;
   stages[CountStages].PID         = _pid;
   stages[CountStages].T           = _t;
   stages[CountStages].OUT0        = _o0;
   stages[CountStages].OUT1        = _o1;
   stages[CountStages].OUT2        = _o2;
   stages[CountStages].OUT3        = _o3;
   stages[CountStages].MOTOR       = _m;
//   stages[CountStages].SERVO       = _serv;
   stages[CountStages].CHECKDP     = _dp;
   stages[CountStages].TIMER_RESET = _reset_tm;
   stages[CountStages].START_HOP   = _start_hop;
   CountStages++; 
}
