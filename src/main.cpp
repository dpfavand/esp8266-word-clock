/*
* A version of my word-clock software for esp8266 without MQTT
*/
#include <FS.h>

#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <ArduinoJson.h>

// var to save the timezone config from the WiFiManager UI
char timezone[3]; // not sure what format to save in - probably should be a signed int
bool shouldSaveConfig = false; // this flag is true if we have new values we need to save to file system

void saveConfigCallback() {
  Serial.println("shouldSaveConfig = true");
  shouldSaveConfig = true;
}

void setup() {

}
