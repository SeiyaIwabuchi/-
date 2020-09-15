#include<Servo.h>

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

Servo myservo;

ESP8266WiFiMulti WiFiMulti;

void setup(){ //初期化タスク
    myservo.attach(0); //モーター制御用ピンとして使用
    pinMode(2,OUTPUT); //エラー表示用LEDピンとして使用

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

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP("TP-Link_74D0", "33878317");
}
unsigned long times = 0;
void loop(){ //メインタスク
    
    if(millis() - times > 10 * 1000){
        times = millis();
        polling();
    }
}

void ctlServo(int sc){ //サーボ制御タスク
  Serial.printf("[ctlServo] arg:%d\n",sc);
}

void errorHandling(int e){ //エラー処理タスク
  Serial.printf("[errorHandling] arg:%d\n",e);
}

void polling(){ //ポーリングタスク
    //wifi接続待ち
    int wstate = WiFiMulti.run();
    Serial.printf("WiFIState:%d\n",wstate);
    if ((wstate == WL_CONNECTED)) {
        
        WiFiClient client;

        HTTPClient http;

        Serial.print("[HTTP] begin...\n");
        if (http.begin(client, "http://192.168.1.46:645/svctl")) {  // HTTP


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
            Serial.println(payload);
            //サーボ制御タスク
            char payloadCharArray[255];
            payload.toCharArray(payloadCharArray, payload.length());
            int svctl = atoi(payloadCharArray);
            if(svctl != NULL) ctlServo(svctl);
            else ctlServo(CLOSE);
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
        Serial.println(wstate);
        errorHandling(2);
    }
}
