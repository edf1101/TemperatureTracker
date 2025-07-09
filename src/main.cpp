/**
 * Low-power temperature / humidity logger
 * --------------------------------------
 *  * ATTiny1614
 *  * BME280
 *  * SSD1306 OLED
 */

#include <Arduino.h>
#include <Wire.h>
#include "Display/Display.h"
#include "Sensor/Sensor.h"
#include "Power/PowerManager.h"
#include "Logger/Logger.h"


/* ---------- project configuration -------------------------------------- */
constexpr bool USE_PMOS_RAIL = true;     // set false to bypass PMOS
constexpr uint8_t EXT_PWR_PIN = 3;        // PMOS gate (active LOW)
constexpr uint8_t BUTTON_PIN = 9;        // pressed = HIGH
constexpr uint8_t PULSE_PIN = 2;        // pulse pin (active HIGH, 50 ms) – NEW
constexpr uint32_t SLEEP_TIMEOUT_MS = 10'000;   // ms of inactivity

/* ---------- globals ---------------------------------------------------- */
Sensor sensor;
Display display;
Logger logger(0);       // 0-based sector index (0-3). Change if a sector is bad.

unsigned long lastScreenUpdate = 0; // last activity timestamp
unsigned long lastPulseUpdate = 0; // last pulse timestamp
uint32_t lastActivityStamp = 0;
bool showMainScreen = true;
WAKE_REASON currentWakeReason = WAKE_NONE;

void setup() {
  /* power / rail manager ------------------------------------------------ */
  powerInit(EXT_PWR_PIN, USE_PMOS_RAIL);

  /* button input (external pulldown) ----------------------------------- */
  pinMode(BUTTON_PIN, INPUT);
  pinMode(PULSE_PIN, INPUT); // <-- NEW: pulse pin

  /* bring rail up, start I²C, init peripherals ------------------------- */
  powerRailOn();
  delay(5);
  Wire.begin();

  display.setup();
  sensor.setup();
  logger.begin();

  lastActivityStamp = millis();
}


void loop() {
  /* ---- handle button press ------------------------------------------ */
  if ((currentWakeReason == WAKE_NONE || currentWakeReason == WAKE_BUTTON) &&
      digitalRead(BUTTON_PIN) == HIGH &&
      millis() - lastActivityStamp > 250) {
    lastActivityStamp = millis();
    showMainScreen = !showMainScreen;
  }

  /* ---- inactivity → sleep ------------------------------------------- */
  if (millis() - lastActivityStamp > SLEEP_TIMEOUT_MS) {
    currentWakeReason = powerEnterDeepSleep(BUTTON_PIN,
                                            PULSE_PIN,
                                            display,
                                            sensor,
                                            lastActivityStamp,
                                            showMainScreen);
  }

  /* ---- read sensor, update display ---------------------------------- */
  if ((currentWakeReason == WAKE_NONE || currentWakeReason == WAKE_BUTTON) && millis() - lastScreenUpdate > 50) {
    Sensor::Data d = sensor.readData();
    lastScreenUpdate = millis();
    if (showMainScreen) {
      display.displayMain(d.temperature, d.humidity);
    } else {
      /* demo chart */
      static int8_t chartBuf[Logger::NUM_SAMPLES];

      /* example: show temperature history */
      logger.readTemperature(chartBuf);
      display.displayChart(chartBuf, /*temp=*/true);
    }
  }

  if (currentWakeReason == WAKE_PULSE) {
    delay(100);
    Sensor::Data d = sensor.readData();
    int8_t temp = static_cast<int8_t>(d.temperature);
    int8_t hum = static_cast<int8_t>(d.humidity);
    logger.push(temp,hum);
    delay(100);
    currentWakeReason = powerEnterDeepSleep(BUTTON_PIN,
                                            PULSE_PIN,
                                            display,
                                            sensor,
                                            lastActivityStamp,
                                            showMainScreen);
  }

  if ((currentWakeReason == WAKE_NONE || currentWakeReason == WAKE_BUTTON) &&
      millis() - lastPulseUpdate > 200 &&
      digitalRead(PULSE_PIN) == HIGH) {


    lastPulseUpdate = millis();
    // It has been pulsed while in the main UI loop
    delay(100);
    Sensor::Data d = sensor.readData();
    /* ---- log data to EEPROM ------------------------------------------- */
    int8_t temp = static_cast<int8_t>(d.temperature);
    int8_t hum = static_cast<int8_t>(d.humidity);
    logger.push(temp,hum);
    delay(100);

  }

}
