#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include<Servo.h>

#ifndef STASSID
#define STASSID "TP-Link_74D0"
#define STAPSK  "33878317"
#endif

#define PIN_LED 2
#define PIN_SERVO 0

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);
Servo myservo;

char returnString[255];

int nowServoState = 0;

void handleRoot() {
  sprintf(returnString,"%d\n",nowServoState);
  server.send(200,"text/plain",returnString);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  myservo.attach(PIN_SERVO); //モーター制御用ピンとして使用
  pinMode(PIN_LED,OUTPUT); //エラー表示用LEDピンとして使用
  
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed!");
    errorHandling(2);
  }
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.on("/open", []() {
    ctlServo(1);
    errorHandling(-1);
    sprintf(returnString,"%d\n",nowServoState);
    server.send(200,"text/plain",returnString);
    });
    
  server.on("/close", []() {
    ctlServo(0);
    errorHandling(-1);
    sprintf(returnString,"%d\n",nowServoState);
    server.send(200,"text/plain",returnString);
    });
    
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  wifi_set_sleep_type(LIGHT_SLEEP_T);
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  MDNS.update();
  ctlLed();
  servoOffTimer();
  if(millis() > 24 * 60 * 60 * 1000) ESP.restart();
}

int ctlServoTime = 0;
void ctlServo(int sc){ //サーボ制御タスク
  Serial.printf("[ctlServo] arg:%d\n",sc);
  ctlServoTime = millis();
  if(sc == 0 && nowServoState != sc) {
    myservo.write(70);
    nowServoState = sc;
  }
  else if(nowServoState != sc){
    myservo.write(0);
    nowServoState = sc;
  }
}

void servoOffTimer(){
  if(millis() - ctlServoTime > 5 * 1000){
    digitalWrite(PIN_SERVO,0);
  }
}

void errorHandling(int e){ //エラー処理タスク
  Serial.printf("[errorHandling] arg:%d\n",e);
  if(e == -1) setCtlLed(0,1000);
  else setCtlLed(2,(e+1)*100);
}

unsigned long ctlLEDtime = 0;
int ledctl = 0;
unsigned long ledInterval = 0;
void setCtlLed(int ctl,unsigned long itvl){
  if(!(ctl == 0 || ctl == 1 || ctl == 2)) Serial.println("[setCtlLed] arg error.");
  else{
    ledctl = ctl;
    ledInterval = itvl;
    Serial.printf("[setCtlLed] ledctl:%d,ledInterval:%lu\n",ledctl,ledInterval);
  }
}
void ctlLed(){
  if(ledctl == 2){
    if(millis() - ctlLEDtime > ledInterval){
      digitalWrite(PIN_LED,!digitalRead(PIN_LED));
      ctlLEDtime = millis();
    }
  }else if(ledctl == 0 || ledctl == 1) digitalWrite(PIN_LED,ledctl);
}
