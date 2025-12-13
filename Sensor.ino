// !!! IMPORTANT: PARTITION SCHEME SETTING !!!
// The Matter library is large (~2.5MB). You MUST change the Partition Scheme in Arduino IDE:
// Go to: Tools -> Partition Scheme
// Select: "Huge App (3MB No OTA/1MB SPIFFS)"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Matter.h>
#include <nvs_flash.h>
#include <esp_sleep.h>

// --- CONFIGURATION ---
#define TEST_MODE false

// --- SENSITIVITY ---
#define TEMP_THRESHOLD  0.8 
#define HUM_THRESHOLD   3.0 
#define PRESS_THRESHOLD 2.0

// --- TIME SETTINGS ---
#define CHECK_INTERVAL_MINS 15
#define FORCE_REPORT_HOURS  24
#define RADIO_WARMUP_MS     500 
#define RADIO_COOLDOWN_MS   500 
#define SETUP_WINDOW_MS     300000 // 5 Minutes to pair after plugging in

// --- HARDWARE ---
#define I2C_SDA 0      
#define I2C_SCL 1      
#define LED_PIN 8 
#define BOOT_BUTTON_PIN 9 // The tiny "B" button on the SuperMini

// --- MEMORY (Survives Deep Sleep) ---
RTC_DATA_ATTR float lastReportedTemp = -999.0;
RTC_DATA_ATTR float lastReportedHum = -999.0;
RTC_DATA_ATTR float lastReportedPress = -999.0;
RTC_DATA_ATTR int wakeupsSinceLastReport = 0;

Adafruit_BME280 bme;

// Global sensors
MatterTemperatureSensor temperatureSensor;
MatterHumiditySensor humiditySensor;
MatterPressureSensor pressureSensor;

// --- FACTORY RESET LOGIC ---
void checkFactoryReset() {
  // Only check this if we just plugged it in (Not waking from deep sleep)
  if (esp_reset_reason() != ESP_RST_DEEPSLEEP) {
    Serial.println(">> FRESH BOOT DETECTED: Entering Setup Window...");
    
    // 1. Indicate "Ready for Reset" (Solid Green)
    neopixelWrite(LED_PIN, 0, 50, 0); // Green
    
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    
    // Wait 10 seconds to see if user wants to reset
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
      if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        // Button Pressed! Wait to see if held for 3 seconds
        unsigned long pressTime = millis();
        bool longPress = false;
        while(digitalRead(BOOT_BUTTON_PIN) == LOW) {
           if (millis() - pressTime > 3000) {
             longPress = true;
             break;
           }
           delay(10);
        }
        
        if (longPress) {
          // --- FACTORY RESET SEQUENCE ---
          Serial.println(">> FACTORY RESET TRIGGERED!");
          // Fast Red Blinks
          for(int i=0; i<10; i++) {
            neopixelWrite(LED_PIN, 50, 0, 0); delay(100);
            neopixelWrite(LED_PIN, 0, 0, 0); delay(100);
          }
          // Wipe NVS
          nvs_flash_erase();
          nvs_flash_init();
          ESP.restart(); // Reboot as fresh device
        }
      }
      delay(10); // Debounce
    }
    
    // 2. If we are here, no reset happened.
    // Turn off LED and stay awake for pairing for 5 mins
    neopixelWrite(LED_PIN, 0, 0, 50); // Blue = Pairing Mode
    
    Matter.begin();
    Serial.println(">> Matter Started. Waiting for pairing...");
    
    // Stay awake for SETUP_WINDOW_MS to allow phone pairing
    unsigned long setupStart = millis();
    while(millis() - setupStart < SETUP_WINDOW_MS) {
       // Just keep the loop alive so Matter can handle commissioning
       // You can add Serial.print(".") here if you want debug
       delay(1000);
    }
    
    // 3. Setup window over. Flash off and start normal life.
    neopixelWrite(LED_PIN, 0, 0, 0);
    Serial.println(">> Setup Window Closed. Going to sleep.");
    
    // Force a sleep so next wake is a "Deep Sleep" wake
    esp_sleep_enable_timer_wakeup(1000000); // Sleep 1 sec
    esp_deep_sleep_start();
  }
}

void setup() {
  // 1. HARDWARE INIT
  if (TEST_MODE || esp_reset_reason() != ESP_RST_DEEPSLEEP) {
    Serial.begin(115200);
    delay(1000);
  }

  // 2. CHECK FOR FRIEND'S HOUSE (Factory Reset Logic)
  checkFactoryReset();

  // --- NORMAL OPERATION BELOW ---
  
  // Kill LED (Crucial for battery)
  neopixelWrite(LED_PIN, 0, 0, 0);

  // Init Sensors
  Wire.begin(I2C_SDA, I2C_SCL);
  bool sensorFound = bme.begin(0x76, &Wire);
  if (!sensorFound) sensorFound = bme.begin(0x77, &Wire);

  if (TEST_MODE) Serial.println("--- Waking Up (Sensor Check) ---");

  // Read Data
  float currTemp = 0, currHum = 0, currPress = 0;
  if (sensorFound) {
    bme.setSampling(Adafruit_BME280::MODE_NORMAL);
    delay(10); 
    currTemp = bme.readTemperature();
    currHum = bme.readHumidity();
    currPress = bme.readPressure() / 100.0F;
    bme.setSampling(Adafruit_BME280::MODE_SLEEP); 
  }

  // Decision Engine
  bool significantChange = false;
  int wakeupsNeeded = (FORCE_REPORT_HOURS * 60) / CHECK_INTERVAL_MINS;
  
  if (wakeupsSinceLastReport >= wakeupsNeeded || lastReportedTemp == -999.0) {
      significantChange = true;
      if (TEST_MODE) Serial.println("Reason: Heartbeat");
  }
  
  if (abs(currTemp - lastReportedTemp) >= TEMP_THRESHOLD) significantChange = true;
  if (abs(currHum - lastReportedHum) >= HUM_THRESHOLD) significantChange = true;
  if (abs(currPress - lastReportedPress) >= PRESS_THRESHOLD) significantChange = true;

  // Branching Logic
  if (significantChange || TEST_MODE) {
    if (TEST_MODE) Serial.println(">> Radio ON. Reporting...");
    
    // Micro-Blink (Green)
    neopixelWrite(LED_PIN, 0, 10, 0); 
    delay(10);
    neopixelWrite(LED_PIN, 0, 0, 0);

    Matter.begin();
    delay(RADIO_WARMUP_MS); 
    
    temperatureSensor.setTemperature(currTemp);
    humiditySensor.setHumidity(currHum);
    pressureSensor.setPressure(currPress);
    
    lastReportedTemp = currTemp;
    lastReportedHum = currHum;
    lastReportedPress = currPress;
    wakeupsSinceLastReport = 0;

    delay(RADIO_COOLDOWN_MS); 

  } else {
    wakeupsSinceLastReport++;
    if (TEST_MODE) Serial.println(">> Silent Check.");
  }

  // Deep Sleep
  neopixelWrite(LED_PIN, 0, 0, 0);
  
  uint64_t sleepTime = (uint64_t)CHECK_INTERVAL_MINS * 60 * 1000000;
  if (TEST_MODE) sleepTime = 5 * 1000000; 

  esp_sleep_enable_timer_wakeup(sleepTime);
  esp_deep_sleep_start();
}

void loop() {
  // Unreachable
}