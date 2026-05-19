
#include <Arduino.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <modbus/wavin/WavinAhc9000Gateway.h>
#include <Homie.h>

#define SETUP_AP_PASS "homeio123"

const char *FW_VERSION = "1.0.0";

#ifndef HOMIE_USE_LITTLEFS
#define HOMIE_USE_LITTLEFS 0
#endif

// --- Serial and gateway ---
HardwareSerial wavinSerial(1);
WavinAhc9000Gateway gateway;
HomieNode gatewayNode("gateway", "Gateway", "bridge");
unsigned long lastStateSentMs = 0;

namespace {
const char* kHomieDirPath = "/homie";
const char* kHomieUiBundlePath = "/homie/ui_bundle.gz";

void logBootInfo() {
#if HOMIE_USE_LITTLEFS
  const char* fsName = "LittleFS";
#else
  const char* fsName = "SPIFFS";
#endif
  Serial.printf("[BOOT] homeio fw=%s fs=%s\n", FW_VERSION, fsName);
}

bool ensureHomieDir(fs::FS& filesystem) {
#if HOMIE_USE_LITTLEFS
  if (filesystem.exists(kHomieDirPath)) {
    return true;
  }
  return filesystem.mkdir(kHomieDirPath);
#else
  (void)filesystem;
  return true;
#endif
}

bool copyFile(fs::FS& srcFs, fs::FS& dstFs, const char* path) {
  File source = srcFs.open(path, "r");
  if (!source) {
    return false;
  }

  File dest = dstFs.open(path, "w");
  if (!dest) {
    source.close();
    return false;
  }

  uint8_t buf[1024];
  while (source.available()) {
    size_t readLen = source.read(buf, sizeof(buf));
    if (readLen == 0) {
      break;
    }
    if (dest.write(buf, readLen) != readLen) {
      source.close();
      dest.close();
      return false;
    }
  }

  source.close();
  dest.close();
  return true;
}

void preflightHomieUiBundle() {
#if HOMIE_USE_LITTLEFS
  const bool activeMounted = LittleFS.begin(false);
  if (!activeMounted) {
    Serial.println("[FS] LittleFS mount failed during preflight");
    return;
  }

  if (LittleFS.exists(kHomieUiBundlePath)) {
    Serial.println("[FS] Homie UI bundle found in LittleFS");
    return;
  }

  Serial.println("[FS] Homie UI bundle missing in LittleFS, probing SPIFFS");
  const bool fallbackMounted = SPIFFS.begin(false);
  if (!fallbackMounted) {
    Serial.println("[FS] SPIFFS mount failed, cannot recover UI bundle");
    return;
  }

  const bool fallbackHasBundle = SPIFFS.exists(kHomieUiBundlePath);
  if (!fallbackHasBundle) {
    Serial.println("[FS] Homie UI bundle not found in SPIFFS either");
    SPIFFS.end();
    return;
  }

  if (!ensureHomieDir(LittleFS)) {
    Serial.println("[FS] Could not create /homie in LittleFS");
    SPIFFS.end();
    return;
  }

  if (copyFile(SPIFFS, LittleFS, kHomieUiBundlePath)) {
    Serial.println("[FS] Recovered Homie UI bundle from SPIFFS to LittleFS");
  } else {
    Serial.println("[FS] Failed to copy Homie UI bundle from SPIFFS to LittleFS");
  }
  SPIFFS.end();
#else
  const bool activeMounted = SPIFFS.begin(false);
  if (!activeMounted) {
    Serial.println("[FS] SPIFFS mount failed during preflight");
    return;
  }

  if (SPIFFS.exists(kHomieUiBundlePath)) {
    Serial.println("[FS] Homie UI bundle found in SPIFFS");
    return;
  }

  Serial.println("[FS] Homie UI bundle missing in SPIFFS, probing LittleFS");
  const bool fallbackMounted = LittleFS.begin(false);
  if (!fallbackMounted) {
    Serial.println("[FS] LittleFS mount failed, cannot recover UI bundle");
    return;
  }

  const bool fallbackHasBundle = LittleFS.exists(kHomieUiBundlePath);
  if (!fallbackHasBundle) {
    Serial.println("[FS] Homie UI bundle not found in LittleFS either");
    return;
  }

  if (copyFile(LittleFS, SPIFFS, kHomieUiBundlePath)) {
    Serial.println("[FS] Recovered Homie UI bundle from LittleFS to SPIFFS");
  } else {
    Serial.println("[FS] Failed to copy Homie UI bundle from LittleFS to SPIFFS");
  }
#endif
}
} // namespace


void logLine(const String &msg) {
  Serial.println(msg);
}

void publishGatewayState() {
  const auto &mainState = gateway.mainState();
  gatewayNode.setProperty("status").send(String(mainState.status));
  gatewayNode.setProperty("dhw-temp").send(String(mainState.dhwSensorTemperature / 10.0f, 1));
  gatewayNode.setProperty("inlet-temp").send(String(mainState.inletSensorTemperature / 10.0f, 1));
  gatewayNode.setProperty("current").send(String(mainState.totalCurrentRaw));
}

void homieLoopHandler() {
  if (millis() - lastStateSentMs >= 10000UL || lastStateSentMs == 0) {
    publishGatewayState();
    lastStateSentMs = millis();
  }
}

void onHomieEvent(const HomieEvent& event) {
  switch (event.type) {
    case HomieEventType::CONFIGURATION_MODE:
      Serial.println("[HOMIE] Configuration mode active");
      break;
    case HomieEventType::NORMAL_MODE:
      Serial.println("[HOMIE] Normal mode active");
      break;
    case HomieEventType::MQTT_READY:
      Serial.println("[HOMIE] MQTT ready");
      publishGatewayState();
      lastStateSentMs = millis();
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      Serial.println("[HOMIE] MQTT disconnected");
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  logBootInfo();
  preflightHomieUiBundle();

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

  Homie_setBrand("homeio");
  Homie_setFirmware("wavin-ahc9000", "1.0.0");
  Homie.setConfigurationApPassword(SETUP_AP_PASS);
  Homie.onEvent(onHomieEvent);
  Homie.setLoopFunction(homieLoopHandler);

  gatewayNode.advertise("status").setName("Status").setDatatype("integer");
  gatewayNode.advertise("dhw-temp").setName("DHW Temperature").setDatatype("float").setUnit("C");
  gatewayNode.advertise("inlet-temp").setName("Inlet Temperature").setDatatype("float").setUnit("C");
  gatewayNode.advertise("current").setName("Current").setDatatype("integer").setUnit("A");

  Homie.setup();
}

void loop() {
  gateway.service();
  Homie.loop();
}