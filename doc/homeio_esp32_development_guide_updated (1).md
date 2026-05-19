# HOMEIO ESP32 Development Guide (Updated)

# Formål

Denne guide beskriver udvikling af ESP32 HOMEIO devices med fokus på:

- Multi-home support
- Provisioning
- MQTT architecture
- Device registration
- OTA
- Wavin AHC 9000 integration
- Future scalability

---

# 1. Provisioning Philosophy

V1 provisioning sker direkte fra ESP32.

ESP32 fungerer som:

```text
self-provisioning MQTT client
```

Raspberry Pi fungerer som:

```text
central authority
```

---

# 2. First Boot Flow

```text
BOOT
 ↓
No config found
 ↓
Start AP mode
 ↓
Captive portal
 ↓
User enters:
    WiFi
    MQTT
    home_id
 ↓
Save config
 ↓
Reboot
 ↓
Connect MQTT
 ↓
Auto register
```

---

# 3. Access Point Mode

## SSID

```text
HOMEIO-SETUP-AHC01
```

## Password

```text
homeio123
```

---

# 4. Captive Portal

ESP32 hosts web UI.

## User Inputs

```text
WiFi SSID
WiFi password

MQTT server
MQTT username
MQTT password

HOME ID
Device name
```

---

# 5. Example Configuration

```json
{
  "wifi_ssid": "MyWiFi",
  "wifi_password": "xxxxx",

  "mqtt_host": "192.168.30.10",
  "mqtt_port": 1883,

  "mqtt_user": "home_7f3a91",
  "mqtt_password": "secret",

  "home_id": "stangsdal",
  "device_id": "ahc01"
}
```

---

# 6. Device Identity

## Required Fields

```json
{
  "home_id": "stangsdal",
  "device_id": "ahc01",
  "device_type": "wavin_ahc9000",
  "fw_version": "1.0.0"
}
```

---

# 7. NVS Storage

ESP32 gemmer:

- WiFi credentials
- MQTT credentials
- home_id
- device_id
- calibration
- OTA channel

---

# 8. MQTT Topic Structure

```text
homeio/{home_id}/{device_id}/state
homeio/{home_id}/{device_id}/cmd
homeio/{home_id}/{device_id}/availability
homeio/{home_id}/{device_id}/telemetry
homeio/{home_id}/{device_id}/ota
homeio/{home_id}/{device_id}/register
```

---

# 9. MQTT Security Model

## home_id

Namespace/routing.

## MQTT credentials

Authentication.

## ACL

Authorization.

---

# 10. MQTT Registration

Efter første MQTT connection:

## Topic

```text
homeio/stangsdal/ahc01/register
```

## Payload

```json
{
  "device_id": "ahc01",
  "type": "wavin_ahc9000",
  "fw": "1.0.0",
  "capabilities": [
    "climate",
    "heating"
  ]
}
```

---

# 11. Availability

## Topic

```text
homeio/stangsdal/ahc01/availability
```

## Online

```text
online
```

## Offline

```text
offline
```

Retained = true

---

# 12. Telemetry

## Topic

```text
homeio/stangsdal/ahc01/telemetry
```

## Payload

```json
{
  "uptime": 123456,
  "heap": 180000,
  "wifi_rssi": -61,
  "mqtt_connected": true,
  "fw": "1.0.0"
}
```

---

# 13. Recovery Mode

Hvis WiFi fejler:

```text
Start AP provisioning mode
```

SSID:

```text
HOMEIO-RECOVERY-AHC01
```

---

# 14. Factory Reset

Anbefalet:

```text
Hold setup button > 10 sec
```

Sletter:

- WiFi
- MQTT credentials
- provisioning data

---

# 15. OTA Architecture

## Flow

```text
ESP32
 ↓
MQTT OTA check
 ↓
Receive version + URL
 ↓
Download firmware
 ↓
Verify
 ↓
Install
 ↓
Reboot
```

---

# 16. OTA Channels

```text
stable
beta
dev
```

---

# 17. Recommended Firmware Structure

```text
firmware/
 ├── core/
 ├── mqtt/
 ├── ota/
 ├── drivers/
 ├── provisioning/
 ├── telemetry/
 └── app/
```

---

# 18. Recommended State Machine

```text
BOOT
 ↓
LOAD CONFIG
 ↓
CONNECT WIFI
 ↓
CONNECT MQTT
 ↓
REGISTER DEVICE
 ↓
START SERVICES
 ↓
RUN LOOP
```

---

# 19. Wavin RS485

## Settings

```text
38400 baud
8N1
```

---

# 20. Recommended Driver Layers

```text
RS485 Driver
 ↓
Wavin Driver
 ↓
HOMEIO Device Layer
 ↓
MQTT Layer
```

---

# 21. Recommended Libraries

| Function | Library |
|---|---|
| MQTT | esp-mqtt |
| JSON | ArduinoJson |
| OTA | esp_https_ota |
| WiFi provisioning | WiFiManager / ESP-IDF Provisioning |

---

# 22. Recommended Security

## V1

- MQTT username/password
- ACL isolation

## V2

- TLS
- Device certificates
- Signed OTA

---

# 23. Recommended Development Phases

## Phase 1

- WiFi
- MQTT
- Provisioning
- OTA

## Phase 2

- Wavin driver
- Zones
- Climate control

## Phase 3

- Discovery
- Monitoring
- Recovery

## Phase 4

- Fleet management
- Secure provisioning
- Cloud integration

---

# 24. Future Expansion

Senere kan tilføjes:

- QR onboarding
- Claim tokens
- Central registry
- Remote support
- Mobile app

---

# 25. Recommended Next Documents

1. Provisioning & Security Architecture
2. MQTT API Specification
3. OTA Design
4. Home Assistant Discovery Templates
5. Device Registry Design
