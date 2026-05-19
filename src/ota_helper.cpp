#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "ota_helper.h"

void setupOTA(const char *hostname, const char *password) {
  ArduinoOTA.setHostname(hostname);
  if (password && strlen(password) > 0) {
    ArduinoOTA.setPassword(password);
  }
  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Start updating...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("[OTA] Ready");
}

void handleOTA() {
  ArduinoOTA.handle();
}
