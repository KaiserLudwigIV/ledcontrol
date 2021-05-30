#include "Connection.h"
#include "LedModi.h"

#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

const char *filename = "settings.txt";

char ssid[] = "your-ssid";
char password[] = "your-password";
char mdnsName[] = "your-mdns-name";
int lednum;

bool loadConfig(){
  File file = SD.open(filename);
  StaticJsonDocument <256> doc;
  DeserializationError error = deserializeJson(doc, file);
  strcpy(ssid, doc["ssid"]);
  strcpy(password, doc["password"]);
  strcpy(mdnsName, doc["mdnsname"]);
  lednum = doc["lednum"];
  return true;
}
void setup(){
  Serial.begin(115200);
  if(!SD.begin(4)){
    Serial.println("SD CARD failed!");
  }
  if(!loadConfig()){
    Serial.println("LoadConfig failed!");
  }
  wifiConnect();
  mdnsInit();
  webServerInit();
  webSocketInit();
  LED_BEGIN();
}

void loop(){
  wifiCheck();
  mdnsUpdate();
  webServerHandle();
  webSocketHandle();
  LED_MODE_SORT();
}
