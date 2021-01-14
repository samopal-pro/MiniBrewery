/**
* Реализация HTTP сервера
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#include "WC_HTTP.h"

//WebServer server = wifiManager.server;

uint32_t HTTP_TM=180000, http_ms;
/**
 * Старт WEB сервера
 */
void HTTP_begin(void){
//   dnswifiManager.server->setErrorReplyCode(DNSReplyCode::NoError);
//   dnswifiManager.server->start(53, "*", WiFi.softAPIP());
//   wifiManager.server->start();
//   Serial.println(WiFi.softAPIP()); 
// Поднимаем WEB-сервер  
   wifiManager.server->on ( "/", HTTP_handleRoot );
   wifiManager.server->on ( "/param", HTTP_handleParam );
   wifiManager.server->on ( "/config", HTTP_handleConfig );
   wifiManager.server->on("/update", HTTP_POST, []() {
      wifiManager.server->sendHeader("Connection", "close");
      wifiManager.server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
      },HTTP_handleUpdate);
    wifiManager.server->on("/upload", HTTP_GET, HTTP_handleUpload);

    wifiManager.server->on ( "/reboot", HTTP_handleReboot );

    wifiManager.server->onNotFound ( HTTP_handleNotFound );
  //here the list of headers to be recorded
    const char * headerkeys[] = {"User-Agent","Cookie"} ;
    size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
    wifiManager.server->collectHeaders(headerkeys, headerkeyssize );


   
    wifiManager.server->begin();
    Serial.printf( "HTTP server started ...\n" );
    http_ms = millis();
  
}

void HTTP_stop(void){ 
//  wifiManager.server->reset();
//  dnswifiManager.server->stop();
//  wifiManager.server->stop();
}


/**
 * Вывод в буфер одного поля формы
 */
void HTTP_printInput(String &out,const char *label, const char *name, const char *value, int size, int len, bool is_pass){
   char str[10];
   if( strlen( label ) > 0 ){
      out += "<td>";
      out += label;
      out += "</td>\n";
   }
   out += "<td><input name ='";
   out += name;
   out += "' value='";
   out += value;
   out += "' size=";
   sprintf(str,"%d",size);  
   out += str;
   out += " length=";    
   sprintf(str,"%d",len);  
   out += str;
   if( is_pass )out += " type='password'";
   out += "></td>\n";  
}

/**
 * Выаод заголовка файла HTML
 */
void HTTP_printHeader(String &out,const char *title, uint16_t refresh){
  out += "<html>\n<head>\n<meta charset=\"utf-8\" />\n";
  if( refresh ){
     char str[10];
     sprintf(str,"%d",refresh);
     out += "<meta http-equiv='refresh' content='";
     out +=str;
     out +="'/>\n"; 
  }
  out += "<title>";
  out += title;
  out += "</title>\n";
  out += "<style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n</head>\n";
  out += "<body>\n";

//  out += "<img src=/logo>\n";
  out += "<h3>";
  out += title;
  out += " ";
  out += Config.ID;
  out += "</h3>";
  http_ms = millis();
//  DisplayText2((char *)title);
}   
 
/**
 * Выаод окочания файла HTML
 */
void HTTP_printTail(String &out){
  out += "</body>\n</html>\n";
}

//Check if header is present and correct
bool is_authentified() {
  Serial.println("Enter is_authentified");
  if (wifiManager.server->hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = wifiManager.server->header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}

/**
 * Выдача формы ввода авторизации
 */
void HTTP_handleLogin() {
  String msg;
  if (wifiManager.server->hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = wifiManager.server->header("Cookie");
    Serial.println(cookie);
  }
  if (wifiManager.server->hasArg("DISCONNECT")) {
    Serial.println("Disconnection");
    wifiManager.server->sendHeader("Location", "/login");
    wifiManager.server->sendHeader("Cache-Control", "no-cache");
    wifiManager.server->sendHeader("Set-Cookie", "ESPSESSIONID=0");
    wifiManager.server->send(301);
    return;
  }
  if (wifiManager.server->hasArg("PASSWORD")) {
    if (wifiManager.server->arg("PASSWORD") == "12345" ) {
      wifiManager.server->sendHeader("Location", "/");
      wifiManager.server->sendHeader("Cache-Control", "no-cache");
      wifiManager.server->sendHeader("Set-Cookie", "ESPSESSIONID=1");
      wifiManager.server->send(301);
      Serial.println("Log in Successful");
      return;
    }
    msg = "Wrong password! try again.";
    Serial.println("Log in Failed");
  }
  Serial.println("OK");
  String content;
  HTTP_printHeader(content,"Login form");

  content += "<form action='/login' method='POST'><br>";
//  content += "User:<input type='text' name='USERNAME' placeholder='user name'><br>";
  content += "Admin password: <input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
  content += "You also can go <a href='/inline'>here</a>";
  HTTP_printTail(content);

  wifiManager.server->send(200, "text/html", content);
}


//root page can be accessed only if authentification is ok
void HTTP_handleParam() {
  if (wifiManager.captivePortal()) return;
  char s[64];
  String content = "";
  content += "<html>\n<head>\n<meta charset=\"utf-8\" />\n";
  content += "<meta http-equiv='refresh' content='5' />\n";
  content += "<title>Params page</title>\n";
  content += "<style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n</head>\n";
  content += "<body>\n";
  content += "<p><a href=/>Home</a>";
   
  content += "<p>Temperature, C: ";
  sprintf(s,"T1=%d.%01d T2=%d.%01d<br>\n",(int)DT0,(int)(DT0*10)%10,(int)DT1,(int)(DT1*10)%10);
  content += s;
  content += "<p>PID, C: ";
  sprintf(s,"PID=%d.%01d Temp=%d.%01d\n",(int)tempPID,(int)(tempPID*10)%10,(int)_temp,(int)(_temp*10)%10);
  content += s;
  sprintf(s,"<p>Type=%d POW=%d\n",(int)isPID,(int)proc);
  content += s;
 sprintf(s,"<p>Motor=%d Valve=%d %d %d %d>\n",(int)Motor.Stat(),(int)Out0.Stat(),(int)Out1.Stat(),(int)Out2.Stat(),(int)Out3.Stat());
  content += s;

    content += "</body>\n</html>\n";
  wifiManager.server->send(200, "text/html", content);
}

//root page can be accessed only if authentification is ok
void HTTP_handleRoot() {
  if (wifiManager.captivePortal()) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  Serial.println("Home Page");
  if ( wifiManager.server->hasArg("Apply") || wifiManager.server->hasArg("SUBMIT") ){
     if( wifiManager.server->hasArg("TEMP")){
         int t = atoi(wifiManager.server->arg("TEMP").c_str());
         if( t>20 && t<=100 )tempPID = t;
     }
     if( wifiManager.server->hasArg("PID")){
         PID_TYPE savePID = isPID;
         isPID = (PID_TYPE)atoi(wifiManager.server->arg("PID").c_str());
         isPID_save = PT_NONE;
/*
         if( isPID != savePID ){
            if( isPID == PT_OFF ){
               myPID.SetMode(MANUAL);
               Heater.Set(LOW);
               TT1 = 0;         
            }
            else {
               myPID.SetMode(AUTOMATIC); 
               ms_pid = millis();             
            }
         }
         */
     }  
     if( wifiManager.server->hasArg("TIME_PID")){
         timePID = (uint32_t)atoi(wifiManager.server->arg("TIME_PID").c_str());
     }
/*
     if( wifiManager.server->hasArg("TUNER_PID5")){
         uint16_t x = (uint16_t)atoi(wifiManager.server->arg("TUNER_PID5").c_str());
         if( x <= 100 )tunerPID5 = x;
     }
     if( wifiManager.server->hasArg("TUNER_PID2")){
         uint16_t x = (uint16_t)atoi(wifiManager.server->arg("TUNER_PID2").c_str());
         if( x <= 100 )tunerPID2 = x;
     }
     if( wifiManager.server->hasArg("TUNER_PID1")){
         uint16_t x = (uint16_t)atoi(wifiManager.server->arg("TUNER_PID1").c_str());
         if( x <= 100 )tunerPID1 = x;
     }
*/
     if( wifiManager.server->hasArg("OUT0"))Out0.Set(HIGH); 
     else Out0.Set(LOW); 
     if( wifiManager.server->hasArg("OUT1"))Out1.Set(HIGH); 
     else Out1.Set(LOW); 
     if( wifiManager.server->hasArg("OUT2"))Out2.Set(HIGH); 
     else Out2.Set(LOW); 
     if( wifiManager.server->hasArg("OUT3"))Out3.Set(HIGH); 
     else Out3.Set(LOW); 
     if( wifiManager.server->hasArg("MOTOR"))Motor.Set(HIGH); 
     else Motor.Set(LOW); 
     if( wifiManager.server->hasArg("SERVO"))servoPos = atoi(wifiManager.server->arg("SERVO").c_str());
//     String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
     return;
  }
 if ( wifiManager.server->hasArg("Auto") || wifiManager.server->hasArg("AUTO") ){
     xTaskCreateUniversal(taskPivo, "pivo", 2048, NULL, 3, NULL,1);   

     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
     return;
  }
  char s[32];
  String content;
  HTTP_printHeader(content,"Home",0);
 content += "<p><a href=/param>Param</a><br>";
  content += "<p><a href=/config>Config</a><br>";
  content += "<a href=/upload>Firmware</a><br>";
  content += "<a href=/reboot>Reboot</a><br>";
//  content
//  content += "<a href=\"/login?DISCONNECT=YES\">Sign out</a>";
// Печатаем время в форму для корректировки
  content += "<form action='/' method='POST'><table><tr>";
  content += "<b>NANOBREWERY CONTROL</b><br>";
 // content += "<iframe src='/param' width='100%'>Frame erroe</iframe>\n";
/*  
  content += "<p>T0=";
  sprintf(s,"%d.%01d",(int)DT0,(int)(DT0*10)%10);
  content += s; 
  sprintf(s,"%d.%01d",(int)DT1,(int)(DT1*10)%10);
  content += "<p>T1=";
  content += s; 
  sprintf(s,"%d",(int)DP1);
  content += "<p>DP1=";
  content += s; 
  sprintf(s,"%d.%01d",(int)TT1,(int)(TT1*10)%10);
  content += "<p>TT1=";
  content += s; 
  content += "</p>\n";
*/  
  content += "<table><tr>";
  content += "<td>Valves</td>";
  content += "<td>0:<input name=\"OUT0\" type=\"checkbox\" ";
  if( Out0.Stat() )content += "checked=\"checked\"";
  content += "/>&nbsp;</td>";
  content += "<td>1:<input name=\"OUT1\" type=\"checkbox\" ";
  if( Out1.Stat() )content += "checked=\"checked\"";
  content += "/>&nbsp;</td>";
  content += "<td>2:<input name=\"OUT2\" type=\"checkbox\" ";
  if( Out2.Stat() )content += "checked=\"checked\"";
  content += "/>&nbsp;</td>";
  content += "<td>3:<input name=\"OUT3\" type=\"checkbox\" ";
  if( Out3.Stat() )content += "checked=\"checked\"";
  content += "/>&nbsp;</td>";
  content += "</tr></table>";
  content += "<table><tr>";
  sprintf(s,"%d",(int)tempPID);
  HTTP_printInput(content,"Heater:","TEMP",s,6,6);
  content +="<td>Off:<input type='radio' name='PID' value='0'";
  if( isPID == PT_OFF )content +="checked";
  content += "></td>";
  content +="<td>T0:<input type='radio' name='PID' value='1'";
  if( isPID == PT_T0 )content +="checked";
  content += "></td>";
  content +="<td>T1:<input type='radio' name='PID' value='2'";
  if( isPID == PT_T1 )content +="checked";
  content += "></td>";
  sprintf(s,"%d",(int)timePID);
  HTTP_printInput(content,"Time,s:","TIME_PID",s,8,8);
  content += "</tr></table>";
/*
  content += "<table><tr>";
  sprintf(s,"%d",(int)tunerPID5);
  HTTP_printInput(content,"Tune heart,Dt5:","TUNER_PID5",s,8,8);
  sprintf(s,"%d",(int)tunerPID2);
  HTTP_printInput(content,"Dt2:","TUNER_PID2",s,8,8);
  sprintf(s,"%d",(int)tunerPID1);
  HTTP_printInput(content,"Dt1:","TUNER_PID1",s,8,8);
  content += "</tr></table>";
*/
  
  content += "<table><tr>";
  content += "<td>Motor:<input name=\"MOTOR\" type=\"checkbox\" ";
  if( Motor.Stat() )content += "checked=\"checked\"";
  content += "/>&nbsp;</td>";
  content += "</tr></table>";
  content += "<table><tr>";
  content += "<td>Servo: </td>";
  content +="<td>0:<input type='radio' name='SERVO' value='0'";
  if( servoPos == 0 )content +="checked";
  content += "></td>";
  content +="<td>1:<input type='radio' name='SERVO' value='1'";
  if( servoPos == 1 )content +="checked";
  content += "></td>";
  content +="<td>2:<input type='radio' name='SERVO' value='2'";
  if( servoPos == 2 )content +="checked";
  content += "></td>";
  content +="<td>3:<input type='radio' name='SERVO' value='3'";
  if( servoPos == 3 )content +="checked";
  content += "></td>";
  content +="<td>4:<input type='radio' name='SERVO' value='4'";
  if( servoPos == 4 )content +="checked";
  content += "></td>";
  content +="<td>5:<input type='radio' name='SERVO' value='5'";
  if( servoPos == 5 )content +="checked";
  content += "></td>";
  content += "</tr></table>";
  
  
   
/*   
   out += "<table><tr>";
   HTTP_printInput(out,"ID:","ID",Config.ID,16,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Pass:","ID_PASS",Config.ID_PASS,16,32,true);
   out += "</tr></table>";
   out += "<b>WiFi Setting</b><br>";
  
   out += "<table><tr>";
   HTTP_printInput(out,"SSID:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;","AP_SSID",Config.AP_SSID,16,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Pass:","AP_PASS",Config.AP_PASS,16,32,true);
   out += "</tr></table>";
   out += "<b>MQTT Setting</b><br>";
   out += "<table><tr>";
   HTTP_printInput(out,"Server:","MQTT_SERVER",Config.MQTT_SERVER,16,32);
   out += "</tr><tr>";
   sprintf(str,"%u",Config.MQTT_PORT);
   HTTP_printInput(out,"Port:","MQTT_PORT",str,16,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"User:","MQTT_USER",Config.MQTT_USER,16,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Pass:","MQTT_PASS",Config.MQTT_PASS,16,32,true);
   out += "</tr><tr>";
   sprintf(str,"%u",Config.MQTT_TM);
   HTTP_printInput(out,"Send TM:","MQTT_TM",str,16,32);
   out += "</tr></table>";
*/   
  content +="<input type='submit' name='SUBMIT' value='Apply'>"; 
  content +="<input type='submit' name='AUTO' value='Auto'>"; 


  
  HTTP_printTail(content);
  wifiManager.server->send(200, "text/html", content);
}



//no need authentification
void HTTP_handleNotFound() {
  if (wifiManager.captivePortal()) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += wifiManager.server->uri();
  message += "\nMethod: ";
  message += (wifiManager.server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += wifiManager.server->args();
  message += "\n";
  for (uint8_t i = 0; i < wifiManager.server->args(); i++) {
    message += " " + wifiManager.server->argName(i) + ": " + wifiManager.server->arg(i) + "\n";
  }
  wifiManager.server->send(404, "text/plain", message);
}

/**
 * Обработчик событий WEB-сервера
 */
bool HTTP_loop(void){
//  dnswifiManager.server->processNextRequest();
//  wifiManager.server->handleClient();
  return HTTP_checkTM();
}


bool HTTP_checkTM(){
  uint32_t _ms = millis();
  if( _ms < http_ms || ( _ms - http_ms ) > HTTP_TM )return false; 
  return true;
}
/**
 * Отображение логотипа
 */
void HTTP_handleLogo(void) {
//  wifiManager.server->send_P(200, PSTR("image/png"), logo, sizeof(logo));
}
  



 
/*
 * Оработчик страницы настройки сервера
 */
void HTTP_handleConfig(void) {
  Serial.println("Config Page");
/*  
  if (!is_authentified()) {
    wifiManager.server->sendHeader("Location", "/login");
    wifiManager.server->sendHeader("Cache-Control", "no-cache");
    wifiManager.server->send(301);
    return;
  }
*/
// Сохранение настроек

  if ( wifiManager.server->hasArg("ID") ){
     if( wifiManager.server->hasArg("ID")          )strcpy(Config.ID,wifiManager.server->arg("ID").c_str());
     if( wifiManager.server->hasArg("ID_PASS")          )strcpy(Config.ID_PASS,wifiManager.server->arg("ID_PASS").c_str());
     if( wifiManager.server->hasArg("NET")         ){
        if(wifiManager.server->arg("NET") == "GSM")Config.NET = NT_GSM;
        else Config.NET = NT_WIFI; 
        Serial.printf("NET=%s\n",wifiManager.server->arg("NET").c_str());
     }
     if( wifiManager.server->hasArg("AP_SSID")     )strcpy(Config.AP_SSID,wifiManager.server->arg("AP_SSID").c_str());
     if( wifiManager.server->hasArg("AP_PASS")     )strcpy(Config.AP_PASS,wifiManager.server->arg("AP_PASS").c_str());
     if( wifiManager.server->hasArg("MQTT_SERVER") )strcpy(Config.MQTT_SERVER,wifiManager.server->arg("MQTT_SERVER").c_str());
     if( wifiManager.server->hasArg("MQTT_PORT")   )Config.MQTT_PORT = atoi(wifiManager.server->arg("MQTT_PORT").c_str());
     if( wifiManager.server->hasArg("MQTT_USER")   )strcpy(Config.MQTT_USER,wifiManager.server->arg("MQTT_USER").c_str());
     if( wifiManager.server->hasArg("MQTT_PASS")   )strcpy(Config.MQTT_PASS,wifiManager.server->arg("MQTT_PASS").c_str());
     if( wifiManager.server->hasArg("MQTT_TM")     )Config.MQTT_TM = atoi(wifiManager.server->arg("MQTT_TM").c_str());
     eepromSave();    
     String header = "HTTP/1.1 301 OK\r\nLocation: /config\r\nCache-Control: no-cache\r\n\r\n";
     wifiManager.server->sendContent(header);
     return;
  }

   String out = "";
   char str[10];
   HTTP_printHeader(out,"Config");
   out += "<p><a href=/>Home</a><br>";
   out += "<a href=/upload>Firmware</a><br>";
   out += "<a href=/reboot>Reboot</a><br>";
//   out += "<a href=\"/login?DISCONNECT=YES\">Sign out</a>";
// Печатаем время в форму для корректировки
   out += "<form action='/config' method='POST'><table><tr>";
   out += "<b>Controller Setting</b><br>";
   out += "<table><tr>";
   HTTP_printInput(out,"ID:","ID",Config.ID,16,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Pass:","ID_PASS",Config.ID_PASS,16,32,true);
/*
   out += "</tr><tr>";
   out += "<td>Network:&nbsp;</td><td>";
   out +="<p>GSM <input type='radio' name='NET' value='GSM'";
   if( Config.NET == NT_GSM )out +="checked";
   out += ">";
   out +="WiFi <input type='radio' name='NET' value='WIFI'";
   if( Config.NET == NT_WIFI )out +="checked";
   out += "></td>"; 
*/ 
   out += "</tr></table>";
   out += "<b>WiFi Setting</b><br>";
  
   out += "<table><tr>";
   HTTP_printInput(out,"SSID:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;","AP_SSID",Config.AP_SSID,16,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Pass:","AP_PASS",Config.AP_PASS,16,32,true);
   out += "</tr></table>";
   out += "<b>MQTT Setting</b><br>";
   out += "<table><tr>";
   HTTP_printInput(out,"Server:","MQTT_SERVER",Config.MQTT_SERVER,16,32);
   out += "</tr><tr>";
   sprintf(str,"%u",Config.MQTT_PORT);
   HTTP_printInput(out,"Port:","MQTT_PORT",str,16,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"User:","MQTT_USER",Config.MQTT_USER,16,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Pass:","MQTT_PASS",Config.MQTT_PASS,16,32,true);
   out += "</tr><tr>";
   sprintf(str,"%u",Config.MQTT_TM);
   HTTP_printInput(out,"Send TM:","MQTT_TM",str,16,32);
   out += "</tr></table>";
   
   out +="<input type='submit' name='SUBMIT' value='Save Setting'>"; 
    
   out += "</form></body></html>";
   wifiManager.server->send ( 200, "text/html", out );
   

}        



/*
 * Сброс настрое по умолчанию
 */
void HTTP_handleDefault(void) {
/*  
// Проверка прав администратора  
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 

  EC_default();
  HTTP_handleConfig();  
  */
}





/*
 * Перезагрузка часов
 */
void HTTP_handleReboot(void) {
  Serial.println("Reboot Conttroller");
/*  
  if (!is_authentified()) {
    wifiManager.server->sendHeader("Location", "/login");
    wifiManager.server->sendHeader("Cache-Control", "no-cache");
    wifiManager.server->send(301);
    return;
  }
*/  
  String content;
  HTTP_printHeader(content,"Reboot Controller");
  content += "<p><b>Controller reset...</b>";
  HTTP_printTail(content);
  wifiManager.server->send(200, "text/html", content);
  delay(2000);
  ESP.restart();    
}

/*
 * Оработчик просмотра одного файла
 */
void HTTP_handleView(void) {
/*  
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  } 
   String file = wifiManager.server->arg("file");
   if( SPIFFS.exists(file) ){
      File f = SPIFFS.open(file, "r");
      size_t sent = wifiManager.server->streamFile(f, "text/plain");
      f.close();
   }
   else {
      wifiManager.server->send(404, "text/plain", "FileNotFound");
   }
*/   
}

/*
 * Оработчик скачивания одного файла
 */
void HTTP_handleDownload(void) {
/*  
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  } 
   String file = wifiManager.server->uri();
   if( SPIFFS.exists(file) ){
      File f = SPIFFS.open(file, "r");
      size_t sent = wifiManager.server->streamFile(f, "application/octet-stream");
      f.close();
   }
   else {
      wifiManager.server->send(404, "text/plain", "FileNotFound");
   }
*/   
}

/*
 * Страница обновления прошивки
 */
void HTTP_handleUpdate() {
  
  Serial.println("Update Page");
/*
  if (!is_authentified()) {
    wifiManager.server->sendHeader("Location", "/login");
    wifiManager.server->sendHeader("Cache-Control", "no-cache");
    wifiManager.server->send(301);
    return;
  }
*/  
  HTTPUpload& upload = wifiManager.server->upload();
  if (upload.status == UPLOAD_FILE_START) {
     Serial.setDebugOutput(true);
     Serial.printf("Update: %s\n", upload.filename.c_str());
     if (!Update.begin()) { //start with max available size
        Update.printError(Serial);
     }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
     if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
       Update.printError(Serial);
     }
  } else if (upload.status == UPLOAD_FILE_END) {
     if (Update.end(true)) { //true to set the size to the current progress
       Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
     } else {
       Update.printError(Serial);
     }
     Serial.setDebugOutput(false);
   } else {
        Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
   }
}


void HTTP_handleUpload() {
  Serial.println("Upload Page");
/*  
  if (!is_authentified()) {
    wifiManager.server->sendHeader("Location", "/login");
    wifiManager.server->sendHeader("Cache-Control", "no-cache");
    wifiManager.server->send(301);
    return;
  }
*/  
  String content;
  HTTP_printHeader(content,"Firmware");
  content += "<p><a href=/>Home</a><br>";  
  content += "<p><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

  HTTP_printTail(content);
  wifiManager.server->send(200, "text/html", content);
}
