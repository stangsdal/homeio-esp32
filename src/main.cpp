
#include <Arduino.h>
#include <HardwareSerial.h>
#include <modbus/wavin/WavinAhc9000Gateway.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <PubSubClient.h>
#include "ota_helper.h"

#define SETUP_AP_SSID "HOMEIO-SETUP-AHC01"
#define SETUP_AP_PASS "homeio123"

// --- Device identity (from dev guide) ---
const char *HOME_ID = "stangsdal";
const char *DEVICE_ID = "ahc01";
const char *DEVICE_TYPE = "wavin_ahc9000";
const char *FW_VERSION = "1.0.0";

// --- WiFi/MQTT config (simulate NVS storage) ---
struct AppConfig {
  String wifi_ssid;
  String wifi_password;
  String mqtt_host;
  uint16_t mqtt_port = 1883;
  String mqtt_user;
  String mqtt_password;
  String home_id;
  String device_id;
} appConfig;

WiFiClient espClient;
PubSubClient mqttClient(espClient);


// --- Serial and gateway ---
HardwareSerial wavinSerial(1);
WavinAhc9000Gateway gateway;


void logLine(const String &msg) {
  Serial.println(msg);
}

void startCaptivePortal() {
  Serial.println("[WIFI] Starting AP mode for provisioning...");
  WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASS);
  // TODO: Start web server for captive portal (user input)
  // For demo: simulate config entry after delay
  delay(3000);
  appConfig.wifi_ssid = "MyWiFi";
  appConfig.wifi_password = "xxxxx";
  appConfig.mqtt_host = "192.168.30.10";
  appConfig.mqtt_port = 1883;
  appConfig.mqtt_user = "home_7f3a91";
  appConfig.mqtt_password = "secret";
  appConfig.home_id = HOME_ID;
  appConfig.device_id = DEVICE_ID;
  Serial.println("[WIFI] Simulated config entered. Rebooting...");
  ESP.restart();
}

void connectToWiFi() {
  Serial.printf("[WIFI] Connecting to %s...\n", appConfig.wifi_ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(appConfig.wifi_ssid.c_str(), appConfig.wifi_password.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected!");
  } else {
    Serial.println("[WIFI] Failed to connect. Starting AP mode.");
    startCaptivePortal();
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming MQTT messages (commands)
}

void connectToMQTT() {
  mqttClient.setServer(appConfig.mqtt_host.c_str(), appConfig.mqtt_port);
  mqttClient.setCallback(mqttCallback);
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting...");
    if (mqttClient.connect(appConfig.device_id.c_str(), appConfig.mqtt_user.c_str(), appConfig.mqtt_password.c_str())) {
      Serial.println("connected!");
      // Subscribe to command topic
      String cmdTopic = String("homeio/") + appConfig.home_id + "/" + appConfig.device_id + "/cmd";
      mqttClient.subscribe(cmdTopic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 2s");
      delay(2000);
    }
  }
}

void publishRegistration() {
  String regTopic = String("homeio/") + appConfig.home_id + "/" + appConfig.device_id + "/register";
  String payload = String("{") +
    "\"device_id\":\"" + appConfig.device_id + "\"," +
    "\"type\":\"" + DEVICE_TYPE + "\"," +
    "\"fw\":\"" + FW_VERSION + "\"," +
    "\"capabilities\":[\"climate\",\"heating\"]}";
  mqttClient.publish(regTopic.c_str(), payload.c_str(), true);
  Serial.println("[MQTT] Registration published:");
  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);

  // --- Configure Wavin gateway ---
  WavinAhc9000Gateway::Config cfg;
  cfg.rxPin = 20; // Set to your wiring
  cfg.txPin = 21;
  cfg.dePin = 5;
  cfg.deActiveHigh = true;
  cfg.baudRate = 38400;
  cfg.parity = 'N';
  cfg.stopBits = 1;
  cfg.slaveId = 1;

  gateway.begin(wavinSerial, cfg, logLine);

  // --- Provisioning logic ---
  if (appConfig.wifi_ssid.isEmpty()) {
    startCaptivePortal();
    return;
  }

  connectToWiFi();
  setupOTA(appConfig.device_id.c_str());
  connectToMQTT();
  publishRegistration();
}

void publishState() {
  // Eksempel: udvidet state med Wavin data
  String stateTopic = String("homeio/") + appConfig.home_id + "/" + appConfig.device_id + "/state";
  const auto &mainState = gateway.mainState();
  String payload = "{";
  payload += "\"fw\":\"" + String(FW_VERSION) + "\",";
  payload += "\"status\":" + String(mainState.status) + ",";
  payload += "\"dhw_temp\":" + String(mainState.dhwSensorTemperature / 10.0f) + ",";
  payload += "\"inlet_temp\":" + String(mainState.inletSensorTemperature / 10.0f) + ",";
  payload += "\"current\":" + String(mainState.totalCurrentRaw) + "}";
  mqttClient.publish(stateTopic.c_str(), payload.c_str(), true);
  // Backend integration: evt. HTTP POST kan tilføjes her
}

void loop() {
  gateway.service();
  handleOTA();
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  if (!mqttClient.connected()) {
    connectToMQTT();
    publishRegistration();
  }
  mqttClient.loop();
  // Periodically publish state
  static unsigned long lastState = 0;
  if (millis() - lastState > 10000) {
    publishState();
    lastState = millis();
  }
  delay(100);
}