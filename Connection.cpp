#include "Connection.h"
#include "StringSplit.h"
#include "dist.h"
#include "editpage.h"
#include "LedModi.h"

#include <Arduino.h>
#include <cstring>
#include <string>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <SPI.h>
#include <SD.h>


#pragma region WifiConnection
bool connectionestablished = false;

void wifiConnect() {
   WiFi.setSleepMode(WIFI_NONE_SLEEP);
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(WiFi.status());
    delay(500);
    Serial.print(".");
    if ( WiFi.status() == 1 || WiFi.status() == 4){
      WiFi.mode(WIFI_OFF);
      WiFi.softAP("esp8266", "88888888");
      return;
    }
  }
  connectionestablished = true;
}

void wifiCheck() {
   if (WiFi.status() != WL_CONNECTED && connectionestablished == true) {
      WiFi.mode(WIFI_OFF);
      wifiConnect();
   }
}
#pragma endregion

#pragma region MDNS
void mdnsInit(){
  MDNS.begin(mdnsName);
}
void mdnsUpdate(){
  MDNS.update();
}
#pragma endregion

#pragma region WebServer
ESP8266WebServer server(80);

File uploadFile;

void websitesend(){
  server.send(200, "text/html", editpage);
}
void returnOK() {
  server.send(200, "text/plain", "");
}
void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}
bool loadFromSdCard(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js"))  dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = SD.open(path.c_str());
  if(dataFile.isDirectory()){
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }
  if (!dataFile)
    return false;
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }
  dataFile.close();
  return true;
}
void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    if(SD.exists((char *)upload.filename.c_str())) SD.remove((char *)upload.filename.c_str());
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(uploadFile) uploadFile.close();
  }
}
void deleteRecursive(String path){
  File file = SD.open((char *)path.c_str());
  if(!file.isDirectory()){
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }
  file.rewindDirectory();
  while(true) {
    File entry = file.openNextFile();
    if (!entry) break;
    String entryPath = path + "/" +entry.name();
    if(entry.isDirectory()){
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  SD.rmdir((char *)path.c_str());
  file.close();
}
void handleDelete(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}
void handleCreate(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  if(path.indexOf('.') > 0){
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if(file){
      file.write((const char *)0);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
}
void printDirectory() {
  if(!server.hasArg("dir")) return returnFail("BAD ARGS");
  String path = server.arg("dir");
  if(path != "/" && !SD.exists((char *)path.c_str())) return returnFail("BAD PATH");
  File dir = SD.open((char *)path.c_str());
  path = String();
  if(!dir.isDirectory()){
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");
  WiFiClient client = server.client();
  server.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
    break;

    String output;
    if (cnt > 0)
      output = ',';

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.name();
    output += "\"";
    output += "}";
    server.sendContent(output);
    entry.close();
 }
 server.sendContent("]");
 dir.close();
}
void handleNotFound(){
  if(loadFromSdCard(server.uri())) return;
  String message = "SDCARD Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
void webServerInit(){
  server.on("/list", HTTP_GET, printDirectory);
  server.on("/edit", HTTP_DELETE, handleDelete);
  server.on("/edit", HTTP_PUT, handleCreate);
  server.on("/editsettings.htm", websitesend);
  server.on("/edit", HTTP_POST, [](){ returnOK(); }, handleFileUpload);
  server.onNotFound(handleNotFound);
  server.begin();
}
void webServerHandle(){
  server.handleClient();
}
#pragma endregion

#pragma region WebSocket
WebSocketsServer webSocket(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

void webSocketInit(){
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.println((char*)payload);
      String WebContent = (char*) payload;
      String* sa = splitString(WebContent, ':');
      String msg = sa[0];
      String param = sa[1];
      if (msg == "mode"){MODUS = param; if(param == "ma_one"){e_off();}}
      else if(msg == "hue"){H = atoi(param.c_str()); STATIC_TEST = 1;}
      else if(msg == "sat"){S = atoi(param.c_str()); STATIC_TEST = 1;}
      else if(msg == "val"){V = atoi(param.c_str()); STATIC_TEST = 1;}
      else if(msg == "D"){DELAY_P = atoi(param.c_str());}
      else if(msg == "A"){WAHRSCHEINLICHKEIT = atoi(param.c_str()); }
      else if(msg == "W"){L_W_WIDTH = atoi(param.c_str()); }
      else if(msg == "MA"){MA_PARAM = param; MA_TEST = 1;}
      delay(1);
  }
}
void webSocketHandle(){
  webSocket.loop();
}
#pragma endregion