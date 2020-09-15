#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include<Servo.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include<stdlib.h>

#ifndef STASSID
#define STASSID "TP-Link_74D0"
#define STAPSK  "33878317"
#endif

#define PIN_LED 2
#define PIN_SERVO 0

const char* ssid = STASSID;
const char* password = STAPSK;

Servo myservo;

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

  Serial.begin(115200); //1,2シリアル通信用に使用
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
      Serial.printf("[SETUP] WAIT %d...\n", t);
      Serial.flush();
      delay(1000);
  }
}

unsigned long times = 0;
void loop() {
  ArduinoOTA.handle();
  ctlLed();
  if(millis() - times > 10 * 1000){
        times = millis();
        polling();
    }
  if(millis() > 24 * 60 * 60 * 1000) ESP.restart();
}

void ctlServo(int sc){ //サーボ制御タスク
  Serial.printf("[ctlServo] arg:%d\n",sc);
  if(sc == 0) {
    myservo.write(70);
  }
  else {
    myservo.write(0);
  }
}

void errorHandling(int e){ //エラー処理タスク
  Serial.printf("[errorHandling] arg:%d\n",e);
  if(e == -1) setCtlLed(0,1000);
  else setCtlLed(2,(e+1)*100);
}

void polling(){ //ポーリングタスク
    //wifi接続待ち
    int wstate = WiFi.waitForConnectResult();
    Serial.printf("WiFIState:%d\n",wstate);
    if ((wstate == WL_CONNECTED)) {
        
        WiFiClient client;

        HTTPClient http;

        Serial.print("[HTTP] begin...\n");
        if (http.begin(client, "http://192.168.1.46:1919/svctl")) {  // HTTP


        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = http.getString();
            Serial.println("[polling] var payload:" + payload);
            //サーボ制御タスク
            char payloadCharArray[4];
            payload.toCharArray(payloadCharArray, sizeof payloadCharArray);
            Serial.printf("[polling] var payloadCharArray:%s\n",payloadCharArray);
            int svctl = atoi(payloadCharArray);
            Serial.printf("[polling] var svctl:%d\n",svctl);
            ctlServo(svctl);
            errorHandling(-1);
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            //エラー処理タスク実行
            errorHandling(0);
        }

        http.end();
        } else {
        Serial.printf("[HTTP} Unable to connect\n");
        //エラー処理タスク実行
        errorHandling(1);
        }
    }else{
        //エラー処理タスク実行
        Serial.println("Connection Failed!");
        errorHandling(2);
    }
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
