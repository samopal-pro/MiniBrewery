/**
* Реализация HTTP сервера
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#include "WC_HTTP.h"
#define   ParamCheck(name,var) if( wifiManager.server->hasArg(name))param_ini.set(var,wifiManager.server->arg(name))
 
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
   wifiManager.server->on ( "/manual", HTTP_handleManual );
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
  out += config_ini.get("AP_NAME");
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
void HTTP_handleRoot() {
   if( wifiManager.server->hasArg("Start") ) {
     xTaskCreateUniversal(taskPivo, "pivo", 2048, NULL, 3, NULL,1);   
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
     return;
   }
   if ( wifiManager.server->hasArg("Pause") || wifiManager.server->hasArg("Resume") ){
     controlPause();
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
     return;
   }
   if( wifiManager.server->hasArg("Stop") ) {
     stopTaskPivo();
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
     return;
   }
  char s[64];
  String content = "";
//  content += "<html>\n<head>\n<meta charset=\"utf-8\" />\n";
//  content += "<meta http-equiv='refresh' content='5' />\n";
//  content += "<title>Params page</title>\n";
//  content += "<style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n</head>\n";
//  content += "<body>\n";
  HTTP_printHeader(content,"Main",5);
  content += "<ul>";
//  content += "<li><a href=/>Home</a>";
  content += "<li><a href=/param>Params</a><br>";
  content += "<li><a href=/manual>Manual</a><br>";
  content += "<li><a href=/config>Config</a><br>";
  content += "<li><a href=/upload>Firmware</a><br>";
  content += "<li><a href=/reboot>Reboot</a><br>";
  content += "</ul>";


  if( ms_main_tm > 0 ){
     uint32_t t=(millis()-ms_main_tm)/1000;
     char s[10];
     sprintf(s,"%02d:%02d",(int)(t/60),(int)(t%60));
     content += "<p>Timer: ";
     content += s;
  }

  
  if( ms_pid > 0 ){
     uint32_t t=(millis()-ms_pid)/1000+ms_pause;
     char s[10];
     sprintf(s,"%02d:%02d",(int)(t/60),(int)(t%60));
     content += " Phase: ";
     content += s;
  }

   
  content += "<p>Temperature, C: ";
  sprintf(s,"T1=%d.%01d T2=%d.%01d<br>\n",(int)DT0,(int)(DT0*10)%10,(int)DT1,(int)(DT1*10)%10);
  content += s;
  content += "<p>PID, C: ";
  sprintf(s,"PID=%d.%01d Temp=%d.%01d\n",(int)tempPID,(int)(tempPID*10)%10,(int)_temp,(int)(_temp*10)%10);
  content += s;
  sprintf(s,"<p>Type=%d POW=%d\n",(int)isPID,(int)proc);
  content += s;
  sprintf(s,"<p>Motor=%d Valve=%d %d %d %d\n",(int)Motor.Stat(),(int)Out0.Stat(),(int)Out1.Stat(),(int)Out2.Stat(),(int)Out3.Stat());
  content += s;
  if( DP1 )content += "<p>DP1=On";
  else content += "<p>DP1=Off";
  content += "<form action='/' method='POST'><table><tr>";
  if( isAUTO ){
     if( isPause )content +="<input type='submit' name='Resume' value='Resume'> "; 
     else content +="<input type='submit' name='Pause' value='Pause'> ";  
     content +="<input type='submit' name='Stop' value='Stop'> "; 
  }
  else {
     content +="<input type='submit' name='Start' value='Start'> "; 
  }
  content += "</form>";
  content += "</body>\n</html>\n";
  wifiManager.server->send(200, "text/html", content);
}

void HTTP_handleParam() {
  if( wifiManager.server->hasArg("Save") ) {
     param_ini.clear();
     ParamCheck("PREHEATING","PREHEATING");
     if(wifiManager.server->hasArg("PAUSE1")){
        ParamCheck("PAUSE_TM1","PAUSE1");  
        ParamCheck("PAUSE_TEMP1","PAUSE_TEMP1");  
     }
     if(wifiManager.server->hasArg("PAUSE2")){
        ParamCheck("PAUSE_TM2","PAUSE2");  
        ParamCheck("PAUSE_TEMP2","PAUSE_TEMP2");  
     }
     if(wifiManager.server->hasArg("PAUSE3")){
        ParamCheck("PAUSE_TM3","PAUSE3");  
        ParamCheck("PAUSE_TEMP3","PAUSE_TEMP3");  
     }
     if(wifiManager.server->hasArg("PAUSE4")){
        ParamCheck("PAUSE_TM4","PAUSE4");  
        ParamCheck("PAUSE_TEMP4","PAUSE_TEMP4");  
     }
     if(wifiManager.server->hasArg("PAUSE5")){
        ParamCheck("PAUSE_TM5","PAUSE5");  
        ParamCheck("PAUSE_TEMP5","PAUSE_TEMP5");  
     }
     if(wifiManager.server->hasArg("PAUSE6")){
        ParamCheck("PAUSE_TM6","PAUSE6");  
        ParamCheck("PAUSE_TEMP6","PAUSE_TEMP6");  
     }
     ParamCheck("BOILING","BOILING");
     if(wifiManager.server->hasArg("HOP1"))ParamCheck("HOP_TM1","HOP1");  
     if(wifiManager.server->hasArg("HOP2"))ParamCheck("HOP_TM2","HOP2");  
     if(wifiManager.server->hasArg("HOP3"))ParamCheck("HOP_TM3","HOP3");  
     ParamCheck("COOL_TEMP","COOL_TEMP");
     eepromParamSave();    
     eepromParamRead();    
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /param\r\nCache-Control: no-cache\r\n\r\n");
     return;
   }
   if( wifiManager.server->hasArg("Default") ) {
     eepromParamDefault();    
     eepromParamSave();    
     eepromParamRead();    
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /param\r\nCache-Control: no-cache\r\n\r\n");
     return;
   }
   if( wifiManager.server->hasArg("Start") ) {
     xTaskCreateUniversal(taskPivo, "pivo", 2048, NULL, 3, NULL,1);   
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
     return;
   }
   if ( wifiManager.server->hasArg("Pause") || wifiManager.server->hasArg("Resume") ){
     controlPause();
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /param\r\nCache-Control: no-cache\r\n\r\n");
     return;
   }
   if( wifiManager.server->hasArg("Stop") ) {
     stopTaskPivo();
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /param\r\nCache-Control: no-cache\r\n\r\n");
     return;
   }


   String content = "";
   HTTP_printHeader(content,"Params Setting");
   content += "<ul>";
   content += "<li><a href=/>Home</a>";
//   content += "<li><a href=/param>Params</a><br>";
   content += "<li><a href=/manual>Manual</a><br>";
   content += "<li><a href=/config>Config</a><br>";
   content += "<li><a href=/upload>Firmware</a><br>";
   content += "<li><a href=/reboot>Reboot</a><br>";
   content += "</ul>";
//   out += "<a href=\"/login?DISCONNECT=YES\">Sign out</a>";
// Печатаем время в форму для корректировки
   content += "<form action='/param' method='POST'><table><tr>";
   content += "<b>Params Setting</b><br>"; 
   content += "<table><tr>";
   HTTP_printInput(content,"PREHEATING, C:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;","PREHEATING",param_ini.get("PREHEATING").c_str(),16,32);
   content += "</tr></table>";

   content += "<table><tr>";
   content += "<td>PAUSE1</td><td><input name=\"PAUSE1\" type=\"checkbox\" ";
   if( param_ini.get("PAUSE1") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","PAUSE_TM1",param_ini.get("PAUSE1").c_str(),16,32);
   HTTP_printInput(content,"TEMP, C:","PAUSE_TEMP1",param_ini.get("PAUSE_TEMP1").c_str(),16,32);
   content += "</tr><tr>";
   content += "<td>PAUSE2</td><td><input name=\"PAUSE2\" type=\"checkbox\" ";
   if( param_ini.get("PAUSE2") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","PAUSE_TM2",param_ini.get("PAUSE2").c_str(),16,32);
   HTTP_printInput(content,"TEMP, C:","PAUSE_TEMP2",param_ini.get("PAUSE_TEMP2").c_str(),16,32);
   content += "</tr><tr>";
   content += "<td>PAUSE3</td><td><input name=\"PAUSE3\" type=\"checkbox\" ";
   if( param_ini.get("PAUSE3") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","PAUSE_TM3",param_ini.get("PAUSE3").c_str(),16,32);
   HTTP_printInput(content,"TEMP, C:","PAUSE_TEMP3",param_ini.get("PAUSE_TEMP3").c_str(),16,32);
   content += "</tr><tr>";
   content += "<td>PAUSE4</td><td><input name=\"PAUSE4\" type=\"checkbox\" ";
   if( param_ini.get("PAUSE4") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","PAUSE_TM4",param_ini.get("PAUSE4").c_str(),16,32);
   HTTP_printInput(content,"TEMP, C:","PAUSE_TEMP4",param_ini.get("PAUSE_TEMP4").c_str(),16,32);
   content += "</tr><tr>";
   content += "<td>PAUSE5</td><td><input name=\"PAUSE5\" type=\"checkbox\" ";
   if( param_ini.get("PAUSE5") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","PAUSE_TM5",param_ini.get("PAUSE5").c_str(),16,32);
   HTTP_printInput(content,"TEMP, C:","PAUSE_TEMP5",param_ini.get("PAUSE_TEMP5").c_str(),16,32);
   content += "</tr><tr>";
   content += "<td>PAUSE6</td><td><input name=\"PAUSE6\" type=\"checkbox\" ";
   if( param_ini.get("PAUSE6") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","PAUSE_TM6",param_ini.get("PAUSE6").c_str(),16,32);
   HTTP_printInput(content,"TEMP, C:","PAUSE_TEMP6",param_ini.get("PAUSE_TEMP6").c_str(),16,32);
   content += "</tr></table>";

   content += "<table><tr>";
   HTTP_printInput(content,"BOILING, MIN:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;","BOILING",param_ini.get("BOILING").c_str(),16,32);
   content += "</tr></table>";

   content += "<table><tr>";
   content += "<td>HOP1&nbsp;&nbsp;&nbsp;&nbsp</td><td><input name=\"HOP1\" type=\"checkbox\" ";
   if( param_ini.get("HOP1") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","HOP_TM1",param_ini.get("HOP1").c_str(),16,32);
   content += "</tr><tr>";
   content += "<td>HOP2&nbsp;&nbsp;&nbsp;&nbsp</td><td><input name=\"HOP2\" type=\"checkbox\" ";
   if( param_ini.get("HOP2") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","HOP_TM2",param_ini.get("HOP2").c_str(),16,32);
   content += "</tr><tr>";
   content += "<td>HOP3&nbsp;&nbsp;&nbsp;&nbsp</td><td><input name=\"HOP3\" type=\"checkbox\" ";
   if( param_ini.get("HOP3") != "" )content += "checked=\"checked\"";
   content += "/>&nbsp;</td>";
   HTTP_printInput(content,"TIME, min:","HOP_TM3",param_ini.get("HOP3").c_str(),16,32);
   content += "</tr></table>";

   content += "<table><tr>";
   HTTP_printInput(content,"COOLING, C:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;","COOL_TEMP",param_ini.get("COOL_TEMP").c_str(),16,32);
   content += "</tr></table>";

  if( isAUTO ){
     if( isPause )content +="<input type='submit' name='Resume' value='Resume'> "; 
     else content +="<input type='submit' name='Pause' value='Pause'> ";  
     content +="<input type='submit' name='Stop' value='Stop'> "; 
  }
  else {
     content +="<input type='submit' name='Save' value='Save'> "; 
     content +="<input type='submit' name='Default' value='Default'> "; 
     content +="<input type='submit' name='Start' value='Start'> "; 
  }
//  content +="<input type='submit' name='AUTO' value='Auto'>"; 
//  content +="<input type='submit' name='PAUSE' value='Pause'>"; 

   content += "</form>";

  HTTP_printTail(content);
  wifiManager.server->send(200, "text/html", content);
}

//root page can be accessed only if authentification is ok
void HTTP_handleManual() {
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
     }  
     if( wifiManager.server->hasArg("TIME_PID")){
         timePID = (uint32_t)atoi(wifiManager.server->arg("TIME_PID").c_str());
     }
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
 if ( wifiManager.server->hasArg("Pause") || wifiManager.server->hasArg("PAUSE") ){
     controlPause();
     wifiManager.server->sendContent("HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n");
     return;
  }
  char s[32];
  String content;
  HTTP_printHeader(content,"Control Manual",0);
  content += "<ul>";
  content += "<li><a href=/>Home</a>";
  content += "<li><a href=/param>Params</a><br>";
//  content += "<li><a href=/manual>Manual</a><br>";
  content += "<li><a href=/config>Config</a><br>";
  content += "<li><a href=/upload>Firmware</a><br>";
  content += "<li><a href=/reboot>Reboot</a><br>";
  content += "</ul>";

  content += "<form action='/' method='POST'><table><tr>";
  content += "<b>NANOBREWERY CONTROL</b><br>";
 // content += "<iframe src='/param' width='100%'>Frame erroe</iframe>\n";
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
  
  if( !isAUTO )content +="<input type='submit' name='SUBMIT' value='Apply'>"; 
  
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
// Сохранение настроек

  if ( wifiManager.server->hasArg("WIFI_NAME") ){
     if( wifiManager.server->hasArg("WIFI_NAME")   )config_ini.set("WIFI_NAME",wifiManager.server->arg("WIFI_NAME"));
     if( wifiManager.server->hasArg("WIFI_NAME")   )config_ini.set("WIFI_PASS",wifiManager.server->arg("WIFI_NAME"));
     if( wifiManager.server->hasArg("MQTT_SERVER") )config_ini.set("MQTT_SERVER",wifiManager.server->arg("MQTT_SERVER"));
     if( wifiManager.server->hasArg("MQTT_PORT")   )config_ini.set("MQTT_PORT",wifiManager.server->arg("MQTT_PORT"));
     if( wifiManager.server->hasArg("MQTT_USER")   )config_ini.set("MQTT_USER",wifiManager.server->arg("MQTT_USER"));
     if( wifiManager.server->hasArg("MQTT_PASS")   )config_ini.set("MQTT_PASS",wifiManager.server->arg("MQTT_PASS"));
     if( wifiManager.server->hasArg("MQTT_TM")     )config_ini.set("MQTT_TM",wifiManager.server->arg("MQTT_TM"));
     eepromSave();    
     eepromRead();    
     String header = "HTTP/1.1 301 OK\r\nLocation: /config\r\nCache-Control: no-cache\r\n\r\n";
     wifiManager.server->sendContent(header);
     return;
  }

   String content = "";
   char str[10];
   HTTP_printHeader(content,"Config");
   content += "<ul>";
   content += "<li><a href=/>Home</a>";
   content += "<li><a href=/param>Params</a><br>";
   content += "<li><a href=/manual>Manual</a><br>";
//  content += "<li><a href=/config>Config</a><br>";
   content += "<li><a href=/upload>Firmware</a><br>";
   content += "<li><a href=/reboot>Reboot</a><br>";
   content += "</ul>";
//   out += "<a href=\"/login?DISCONNECT=YES\">Sign out</a>";
// Печатаем время в форму для корректировки
   content += "<form action='/config' method='POST'><table><tr>";
   content += "<b>Controller Setting</b><br>";
//   content += "<table><tr>";
//   HTTP_printInput(content,"AP_NAME:","AP_NAME",Config.ID,16,32);
//   content += "</tr><tr>";
//   HTTP_printInput(content,"Pass:","ID_PASS",Config.ID_PASS,16,32,true);
//   content += "</tr></table>";
   content += "<b>WiFi Setting</b><br>";
  
   content += "<table><tr>";
   HTTP_printInput(content,"WIFI NAME:","WIFI_SSID",config_ini.get("WIFI_NAME").c_str(),16,32);
   content += "</tr><tr>";
   HTTP_printInput(content,"WIFI PASS:","WIFI_PASS",config_ini.get("WIFI_PASS").c_str(),16,32,true);
   content += "</tr></table>";
   content += "<b>MQTT Setting</b><br>";
   content += "<table><tr>";
   HTTP_printInput(content,"Server:","MQTT_SERVER",config_ini.get("MQTT_SERVER").c_str(),16,32);
   content += "</tr><tr>";
   HTTP_printInput(content,"Port:","MQTT_PORT",config_ini.get("MQTT_PORT").c_str(),16,32);
   content += "</tr><tr>";
   HTTP_printInput(content,"User:","MQTT_USER",config_ini.get("MQTT_USER").c_str(),16,32);
   content += "</tr><tr>";
   HTTP_printInput(content,"Pass:","MQTT_PASS",config_ini.get("MQTT_PASS").c_str(),16,32,true);
   content += "</tr><tr>";
   HTTP_printInput(content,"Send TM (ms):","MQTT_TM",config_ini.get("MQTT_TM").c_str(),16,32);
   content += "</tr></table>";
   
   content +="<input type='submit' name='SUBMIT' value='Save Setting'>"; 
    
   content += "</form></body></html>";
   wifiManager.server->send ( 200, "text/html", content );
   

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
  String content;
  HTTP_printHeader(content,"Reboot Controller");
  content += "<p><b>Controller reset...</b>";
  HTTP_printTail(content);
//  String header = "HTTP/1.1 301 OK\r\nLocation: /param\r\nCache-Control: no-cache\r\n\r\n";
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
  String content;
  HTTP_printHeader(content,"Firmware");
  content += "<ul>";
  content += "<li><a href=/>Home</a>";
  content += "<li><a href=/param>Params</a><br>";
  content += "<li><a href=/manual>Manual</a><br>";
  content += "<li><a href=/config>Config</a><br>";
//  content += "<li><a href=/upload>Firmware</a><br>";
  content += "<li><a href=/reboot>Reboot</a><br>";
  content += "</ul>";
  content += "<p><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

  HTTP_printTail(content);
  wifiManager.server->send(200, "text/html", content);
}
