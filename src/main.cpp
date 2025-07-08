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
constexpr uint32_t SLEEP_TIMEOUT_MS = 10'000;   // ms of inactivity

/* ---------- globals ---------------------------------------------------- */
Sensor sensor;
Display display;
Logger logger(0);       // 0-based sector index (0-3). Change if a sector is bad.


uint32_t lastActivityStamp = 0;
bool showMainScreen = true;

void setup() {
  /* power / rail manager ------------------------------------------------ */
  powerInit(EXT_PWR_PIN, USE_PMOS_RAIL);

  /* button input (external pulldown) ----------------------------------- */
  pinMode(BUTTON_PIN, INPUT);

  /* bring rail up, start I²C, init peripherals ------------------------- */
  powerRailOn();
  delay(5);
  Wire.begin();

  display.setup();
  sensor.setup();
  logger.begin();

  lastActivityStamp = millis();
}

long lastlog;

void loop() {
  /* ---- handle button press ------------------------------------------ */
  if (digitalRead(BUTTON_PIN) == HIGH &&
      millis() - lastActivityStamp > 250) {
    lastActivityStamp = millis();
    showMainScreen = !showMainScreen;
  }

  /* ---- inactivity → sleep ------------------------------------------- */
  if (millis() - lastActivityStamp > SLEEP_TIMEOUT_MS) {
    powerEnterDeepSleep(BUTTON_PIN,
                        display,
                        sensor,
                        lastActivityStamp,
                        showMainScreen);
  }

  /* ---- read sensor, update display ---------------------------------- */
  Sensor::Data d = sensor.readData();

  if (showMainScreen) {
    display.displayMain(d.temperature, d.humidity);
  } else {
    /* demo chart */
    static int8_t chartBuf[Logger::NUM_SAMPLES];

    /* example: show temperature history */
    logger.readHumidity(chartBuf);
    display.displayChart(chartBuf, /*temp=*/true);
  }
  if (millis() - lastlog > 1000) {
    lastlog = millis();
    /* ---- log data to EEPROM ------------------------------------------- */
    logger.push(static_cast<int8_t>(round(d.temperature)),
                static_cast<int8_t>(round(d.humidity)));
  }
  delay(100);
}
