#include "PowerManager.h"
#include <Wire.h>
#include <avr/sleep.h>

/* -------- private state ------------------------------------------------ */
static uint8_t g_pmosPin = 0;
static bool g_usePmos = false;

static uint8_t g_buttonPin = 0;
static uint8_t g_pulsePin = 0;

static inline void railOn_() { if (g_usePmos) digitalWrite(g_pmosPin, LOW); }
static inline void railOff_() { if (g_usePmos) digitalWrite(g_pmosPin, HIGH); }

static volatile WAKE_REASON g_wakeReason = WAKE_NONE;

void powerRailOn() { railOn_(); }
void powerRailOff() { railOff_(); }

/* ---- Enhanced I2C management ------------------------------------------ */
static void i2cHiZ() {
  Wire.end();
  pinMode(SDA, INPUT);
  pinMode(SCL, INPUT);
  // Clear any pull-ups to prevent phantom powering
  digitalWrite(SDA, LOW);
  digitalWrite(SCL, LOW);
}

static void i2cBegin() {
  Wire.begin();
  // Small delay to let I2C settle
  delay(10);
}

/* ---- wake ISR (edge only, do nothing) --------------------------------- */
static void wakeISRButton() { g_wakeReason = WAKE_BUTTON; }
static void wakeISRPulse() { g_wakeReason = WAKE_PULSE; }

/* ----------------------------------------------------------------------- */
/*                P U B L I C   A P I                                      */
/* ----------------------------------------------------------------------- */

void powerInit(uint8_t pmosGatePin, bool usePmos) {
  g_pmosPin = pmosGatePin;
  g_usePmos = usePmos;

  pinMode(g_pmosPin, OUTPUT);
  railOff_();
}

WAKE_REASON powerEnterDeepSleep(uint8_t buttonPin,
                                uint8_t pulsePin,
                                Display &display,
                                Sensor &sensor,
                                uint32_t &lastActivityStamp,
                                bool &showMainScreen) {
  g_buttonPin = buttonPin;
  g_pulsePin = pulsePin;

  /* -------- Enhanced shutdown sequence -------------------------------- */
  display.powerDown();
  sensor.powerOff();  // This now puts BME280 in sleep mode first

  if (g_usePmos) {
    i2cHiZ();
    delay(20);  // Give I2C time to settle before rail off
    railOff_();
  }

  /* -------- arm wake-up edges ----------------------------------------- */
  attachInterrupt(digitalPinToInterrupt(g_buttonPin), wakeISRButton, RISING);
  attachInterrupt(digitalPinToInterrupt(g_pulsePin), wakeISRPulse, RISING);

  delay(10);  // Increased debounce time

  /* -------- enter full POWER-DOWN ------------------------------------- */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  sleep_disable();

  /* -------- Enhanced wake-up sequence --------------------------------- */
  detachInterrupt(digitalPinToInterrupt(g_buttonPin));
  detachInterrupt(digitalPinToInterrupt(g_pulsePin));

  railOn_();
  if (g_usePmos) {
    delay(50);  // Longer rail settle time
    i2cBegin();
  }

  // Enhanced peripheral initialization
  display.setup();
  delay(10);
  sensor.setup();  // Now includes better error handling
  delay(10);

  /* Wait for inputs to go low */
  while (digitalRead(g_buttonPin) == HIGH ||
         digitalRead(g_pulsePin) == HIGH) {
    delay(1);
  }

  /* -------- reset UI state & timers ----------------------------------- */
  lastActivityStamp = millis();
  showMainScreen = true;

  return g_wakeReason;
}