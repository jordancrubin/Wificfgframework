/*
  Rubin Projects Boot Framework and ASYNC web Based Wifi configuration Framework 
  Standardized basic framework for my projects that require no special code nor
  any extra libraries beyond what I regularly use in my projects to put an ESP32
  on the network with an interactive Web based GUI. 
  https://www.youtube.com/c/jordanrubin6502
  2021 Jordan Rubin.
*/
#define WEBSERVPORT 80
#include <Arduino.h>  
#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <NTP.h>
#include <Sprinkler.h>

const char* ssid = "sprinklersystem";    //SSID of the netconfig Access point
const char* deviceName = "sprinkler32";  //Mdns name sprinkler32.local

AsyncWebServer server(WEBSERVPORT);
SPRINKLERSYSTEM sprinklersystem(2,23,5000);

WiFiUDP wifiUdp;
NTP ntp(wifiUdp);
void startup();                         // Pre-declaration for simplicity

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleRoot]-------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleRoot(AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  }
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleCfgRoot]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleCfgRoot(AsyncWebServerRequest *request) { 
  request->send(SPIFFS, "/cfgindex.html");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleCss]--------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleCss(AsyncWebServerRequest *request) { 
  request->send(SPIFFS, "/mystyle.css", "text/css");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleCheckStatus]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleCheckStatus(AsyncWebServerRequest *request) {
    char result[30];         
    sprinklersystem.readConfigFile(result,"/testresult.cnf","CONN");
    Serial.print(F("CONNECTION -> ")); Serial.println(result);
    request->send(200, "text/html", result);
    if (strcmp(result,"SUCCESS") == 0){
      Serial.println(F("Rebooting into normal operation."));
      ESP.restart();
    }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[stringToarray]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void stringToarray (char * convstr, String input) {
   int str_len = input.length() + 1; 
   input.toCharArray(convstr, str_len);   
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleWifiConnect]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleWifiConnect(AsyncWebServerRequest *request) {  
  AsyncWebParameter* p = request->getParam(0);
  char ssid[50];
  char password[75];
  stringToarray(ssid, p->value());
  p = request->getParam(1);
  stringToarray(password, p->value());
  File testfile = SPIFFS.open("/testnetwork.cnf","w");
  if (!testfile){
      Serial.print(F("Could not open test network file for writing"));
      return;  
  }
  testfile.println(F("\####### NETWORK CONFIG FILE"));
  testfile.println(F("\####### JRUBIN 2021 technocoma.blogspot.com DO NOT EDIT"));
  testfile.print(F("SSID=")); testfile.println(ssid);
  testfile.print(F("PASSWORD=")); testfile.println(password);
  testfile.close();
  request->send(200, "text/html", "RCVD"); 
  startup();
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleWifiList]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleWifiList(AsyncWebServerRequest *request) {
  int n = WiFi.scanNetworks(); 
  String s = "[";
  for (int i = 0; i < n; ++i) {
    s=s+"{\"name\":\""+WiFi.SSID(i)+"\",\"val\":\""+WiFi.SSID(i)+"\"},";        
  } 
  int lastIndex = s.length() - 1;
  s.remove(lastIndex);
  s+="]"; 
  request->send(200, "text/html", s);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[configNetwork]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void configNetwork() { 
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP:   ");
  Serial.println(myIP);
  if (!MDNS.begin(deviceName)){
    Serial.print(F("Error starting mDNS"));
  }
  if (SPIFFS.exists("/testnetwork.cnf")){
    char ssid[50];          
    sprinklersystem.readConfigFile(ssid,"/testnetwork.cnf","SSID");
    Serial.print(F("Attempting with SSID -> ")); Serial.println(ssid);
    char key[50];
    sprinklersystem.readConfigFile(key,"/testnetwork.cnf","PASSWORD");    
    if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
    File resultfile = SPIFFS.open("/testresult.cnf","w");
    if (!resultfile){
      Serial.print("Could not open testresult file for writing");
      return;  
    }    
    resultfile.println(F("\####### NETWORK RESULT FILE"));
    resultfile.println(F("\####### JRUBIN 2021 technocoma.blogspot.com DO NOT EDIT"));
    WiFi.begin(ssid,key);
    int i =0; 
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      sprinklersystem.statusLedBlink(1,100);
      i++;
      if (i==15){
        Serial.print("\nWifi Conn. Failed, Restarting\n");
        delay(100);
        SPIFFS.remove("/testnetwork.cnf");
        resultfile.print(F("CONN=")); resultfile.println("FAIL"); 
        resultfile.close();            
        startup();
        return;
      }
    }
  resultfile.print(F("CONN=")); resultfile.println("SUCCESS");
  resultfile.close();
  SPIFFS.rename("/testnetwork.cnf", "/network.cnf");
  Serial.println(WiFi.localIP());
  }
  server.on("/", HTTP_GET , handleCfgRoot);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[loadNetwork]------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void loadNetwork() {
    char ssid[50];   
    sprinklersystem.readConfigFile(ssid,"/network.cnf","SSID");
    Serial.print("    SSID -> "); Serial.println(ssid);
    char key[50];  
    sprinklersystem.readConfigFile(key,"/network.cnf","PASSWORD");  
    if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
    WiFi.begin(ssid,key);
    int i =0;
    Serial.print("Waiting.");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      sprinklersystem.statusLedBlink(1,100);
      i++;
      if (i==15){
        Serial.print("\nNETWORK: Wifi Conn. Failed, Restarting\n");
        delay(100);     
        startup();        
    }
  }
  sprinklersystem.statusLedBlink(1,0);
  if (!MDNS.begin(deviceName)){
    Serial.print("\nmDNS:    Error Starting...");
  }
  else {
    Serial.print("\nmDNS:    Listed as ");Serial.print(deviceName); Serial.println(".local");
  }  
    Serial.print("NETWORK: Connected at "); Serial.println(WiFi.localIP());
    ntp.begin();
    Serial.print("NTP:     ");
    Serial.print(ntp.formattedTime("%d. %B %Y  ")); // dd. Mmm yyyy
    Serial.println(ntp.formattedTime("%T")); // dd. Mmm yyyy
    server.on("/", HTTP_GET , handleRoot);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[startup]------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void startup(){
  sprinklersystem.factoryDefaultChk();
  if (SPIFFS.exists("/network.cnf")){
     Serial.println("NETWORK: Already configured, Loading......");
     loadNetwork();
  }
  else {
     Serial.println("NETWORK: Setting up for initial configuration.....");   
     configNetwork();    
       }
  server.on("/mystyle.css",  HTTP_GET, handleCss);
  server.on("/getWifiList",  HTTP_GET, handleWifiList);
  server.on("/connectwWifi", HTTP_POST, handleWifiConnect); 
  server.on("/checkStatus",  HTTP_GET, handleCheckStatus); 
  server.begin();
  Serial.print("WEBSVR:  Running on port ");Serial.println(WEBSERVPORT);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[setup]--------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n***Rubin Projects Boot Framework***\n   technocoma.blogspot.com 2021\n");
  if(!sprinklersystem.startSpiffFs()){
        Serial.println("SPIFFS: An Error has occurred while mounting SPIFFS");
        return;
  } 
  else {Serial.println("SPIFFS:  Mounted");}
  startup();
}

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[loop]---------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void loop() {
  sprinklersystem.factoryDefaultChk();
  delay(5000);
}
//-----------------------------------------------------------------------]