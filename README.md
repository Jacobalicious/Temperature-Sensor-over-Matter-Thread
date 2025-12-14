# 🌡️ ESP32-C3 Matter & Thread Sleepy Sensor

![License](https://img.shields.io/badge/license-MIT-green) ![Matter](https://img.shields.io/badge/Protocol-Matter-blue) ![Platform](https://img.shields.io/badge/Platform-ESP32-orange)

A high-efficiency, battery-powered environmental sensor (Temperature, Humidity, Pressure) that runs on the Matter protocol over Thread. 

Designed for the **ESP32-C3 SuperMini**, this device utilizes Deep Sleep and adaptive reporting to maximize battery life. It features a custom 3D-printed enclosure and a rechargeable 1000mAh LiPo battery via a USB-C TP4056 module.

![Internal Electronics](images/internal_wiring.jpg)

## ✨ Features
* **Matter over Wi-Fi/Thread Support:** Natively integrates with Apple Home, Google Home, Alexa, and Home Assistant.
* **Deep Sleep Logic:** The device sleeps 99% of the time, checking sensors every 15 minutes.
* **Adaptive Reporting:** Only connects to radio if:
    * Temperature changes by > 0.8°C
    * Humidity changes by > 3%
    * Pressure changes by > 2hPa
    * Or 24 hours have passed (Heartbeat)
* **Rechargeable:** Integrated TP4056 charging circuit with USB-C.

## 🛠️ Hardware Required
* **Microcontroller:** ESP32-C3 SuperMini.
* **Sensor:** GY-BME280 (I2C).
* **Power:** * MakerHawk 3.7V 1000mAh LiPo Battery.
  * TP4056 USB-C Charging Module (with protection).
* **Case:** Custom 3D Printed Enclosure (STLs in `/3d_files`).

## 🔌 Wiring Guide

```mermaid
graph TD
    %% Define Styles
    classDef power fill:#f9f,stroke:#333,stroke-width:2px;
    classDef sensor fill:#bbf,stroke:#333,stroke-width:2px;
    classDef mcu fill:#bfb,stroke:#333,stroke-width:2px;

    %% Components
    Battery(LiPo Battery 3.7V):::power
    Charger(TP4056 Module):::power
    ESP(ESP32-C3 SuperMini):::mcu
    BME(BME280 Sensor):::sensor

    %% Wiring Connections
    Battery -- Red Wire --> Charger_B_Plus[TP4056 B+]
    Battery -- Black Wire --> Charger_B_Minus[TP4056 B-]
    
    Charger_Out_Plus[TP4056 OUT+] -- 5V --> ESP_5V[ESP32 5V Pin]
    Charger_Out_Minus[TP4056 OUT-] -- GND --> ESP_GND[ESP32 GND]

    ESP_3V3[ESP32 3.3V] -- Power --> BME_VCC[BME280 VCC]
    ESP_GND -- Ground --> BME_GND[BME280 GND]
    ESP_0[ESP32 GPIO 0] -- SDA --> BME_SDA[BME280 SDA]
    ESP_1[ESP32 GPIO 1] -- SCL --> BME_SCL[BME280 SCL]
```

## 💻 Installation & Flashing

### 1. Arduino IDE Setup
1.  Install ESP32 Board Manager (v2.0.11+).
2.  Install the **"Matter"** library (Espressif).
3.  Install **Adafruit BME280** library.

### 2. Critical Settings
You **must** change the partition scheme or the upload will fail.
* **Board:** ESP32C3 Dev Module
* **USB CDC On Boot:** Enabled
* **Partition Scheme:** `Huge App (3MB No OTA/1MB SPIFFS)`

### 3. Flash
Connect the ESP32 directly via its USB port (or the TP4056 if data lines are connected, though direct ESP32 connection is recommended for flashing) and upload `src/MatterSensor.ino`.

## 🚦 Usage & Factory Reset
* **Pairing:** On first boot, the LED turns **Solid Blue**. You have 5 minutes to pair using the Matter QR code printed in the Serial Monitor.
* **Factory Reset:** If you change networks, unplug the device, plug it back in, and hold the **Boot Button** for 3 seconds while the LED is **Green** (first 10 seconds of boot).

## 📂 3D Printing
STLs are located in the `3d_files` directory. 
* **Material:** PLA or PETG
* **Infill:** 15%

![Battery Installation](images/battery_installation.jpg)

## 📜 License
MIT License.