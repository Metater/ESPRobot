#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include <Servo.h>

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>

#include "Constants.h"

Servo leftServo;
Servo rightServo;

const String url = "http://api.metater.tk:5000/postlocalrobotip?ip=";

unsigned long lastGetRequestTime = 0;
unsigned long getRequestTimerDelay = 3000;

unsigned long lightTurnOffTime = 0;

int lastLeftSpeed = 90;
int lastRightSpeed = 90;

bool localRobotIPSynced = false;

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(81);

#define USE_SERIAL Serial

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch(type)
  {
    case WStype_DISCONNECTED:
        USE_SERIAL.printf("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED:
        {
          IPAddress ip = webSocket.remoteIP(num);
          USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
          
          // send message to client
          webSocket.sendTXT(num, "Connected");
        }
        break;
    case WStype_TEXT:
        USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
  
        // send message to client
        // webSocket.sendTXT(num, "message here");
  
        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
        break;
    case WStype_BIN:
        USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
        //hexdump(payload, length);

        lastLeftSpeed = payload[0];
        lastRightSpeed = payload[1];

        USE_SERIAL.println("Got new speeds: " + String(lastLeftSpeed) + " : " + String(lastRightSpeed));
  
        // send message to client
        // webSocket.sendBIN(num, payload, length);
        break;
  }
}



void setup()
{
  leftServo.attach(5);
  rightServo.attach(4);
  pinMode(16, OUTPUT);

  USE_SERIAL.begin(74880);

  USE_SERIAL.setDebugOutput(true);
  
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();
  
  for(uint8_t t = 4; t > 0; t--)
  {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }
  
  WiFiMulti.addAP(ap0ssid, appass);
  WiFiMulti.addAP(ap1ssid, appass);
  WiFiMulti.addAP(ap2ssid, appass);
  
  while(WiFiMulti.run() != WL_CONNECTED)
  {
    delay(100);
  }
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}


void loop()
{
  if (!localRobotIPSynced)
  {
    leftServo.write(90);
    rightServo.write(90);
    lastLeftSpeed = 90;
    lastRightSpeed = 90;
    tryGetRequest();
  }
  else
  {
    webSocket.loop();
    
    leftServo.write(lastLeftSpeed);
    rightServo.write(lastRightSpeed);
  }

  tryStatusLight();
}

void tryStatusLight()
{
  if (lightTurnOffTime > millis()) digitalWrite(16, HIGH);
  else digitalWrite(16, LOW);
}

void tryGetRequest()
{
  if (millis() - lastGetRequestTime > getRequestTimerDelay)
  {
    getRequest(url + WiFi.localIP().toString());
    lastGetRequestTime = millis();
  }
}

void getRequest(String link)
{
  if(WiFi.status()== WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, link.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
      if (payload == "200") localRobotIPSynced = true;
      lightTurnOffTime = millis() + 50;
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
    localRobotIPSynced = false;
  }
}
