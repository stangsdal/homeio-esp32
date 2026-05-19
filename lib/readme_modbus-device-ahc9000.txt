# modbus-device-ahc9000 integration notes

This file documents the integration of the modbus-device-ahc9000 library into the HOMEIO ESP32 project.

- Library: https://github.com/stangsdal/modbus-device-ahc9000
- PlatformIO lib_deps: https://github.com/stangsdal/modbus-device-ahc9000.git
- Provides: WavinAhc9000Gateway, device driver, and adapter for Wavin AHC-9000/AC-116
- See doc/homeio_esp32_development_guide_updated (1).md for device requirements and MQTT mapping.

Integration steps:
1. Add library to platformio.ini
2. Use WavinAhc9000Gateway in src/main.cpp
3. Implement MQTT registration and state reporting as described in the development guide.
