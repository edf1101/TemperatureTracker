#include "PowerManager.h"
#include <Wire.h>
#include <avr/sleep.h>

/* -------- private state ------------------------------------------------ */
static uint8_t g_pmosPin = 0;
static bool g_usePmos = false;

static uint8_t g_buttonPin = 0;
static uint8_t g_pulsePin = 0;     // <-- NEW

static inline void railOn_() { if (g_usePmos) digitalWrite(g_pmosPin, LOW); }

static inline void railOff_() { if (g_usePmos) digitalWrite(g_pmosPin, HIGH); }

static volatile WAKE_REASON g_wakeReason = WAKE_NONE;

void powerRailOn() { railOn_(); }

void powerRailOff() { railOff_(); }

/* ---- helpers to tri-state / restart I²C bus when rail is off ---------- */
static void i2cHiZ() {
  Wire.end();
  pinMode(SDA, INPUT);
  pinMode(SCL, INPUT);
}

static void i2cBegin() { Wire.begin(); }

/* ---- wake ISR (edge only, do nothing) --------------------------------- */
static void wakeISRButton() { g_wakeReason = WAKE_BUTTON; }

static void wakeISRPulse() { g_wakeReason = WAKE_PULSE; }

/* ----------------------------------------------------------------------- */
/*                P U B L I C   A P I                                      */
/* ----------------------------------------------------------------------- */

/* Call once at start-up */
void powerInit(uint8_t pmosGatePin, bool usePmos) {
  g_pmosPin = pmosGatePin;
  g_usePmos = usePmos;

  pinMode(g_pmosPin, OUTPUT);
  railOff_();                        // keep rail off while MCU boots
}

/* Call whenever you want to power down.
 *  - buttonPin … your existing front-panel push-button
 *  - pulsePin  … line driven by the ATtiny412 (normally LOW, 50 ms HIGH)
 */
WAKE_REASON powerEnterDeepSleep(uint8_t buttonPin,
                         uint8_t pulsePin,      // <-- NEW
                         Display &display,
                         Sensor &sensor,
                         uint32_t &lastActivityStamp,
                         bool &showMainScreen) {
  /* remember pins so we can test levels after wake */
  g_buttonPin = buttonPin;
  g_pulsePin = pulsePin;

  /* -------- shut peripherals down ------------------------------------- */
  display.powerDown();
  sensor.powerOff();

  if (g_usePmos) {
    i2cHiZ();
    railOff_();
  }

  /* -------- arm wake-up edges ----------------------------------------- */
  attachInterrupt(digitalPinToInterrupt(g_buttonPin), wakeISRButton, RISING);
  attachInterrupt(digitalPinToInterrupt(g_pulsePin), wakeISRPulse, RISING);

  delay(5);        // mechanical debounce for the button (pulse is digital)

  /* -------- enter full POWER-DOWN ------------------------------------- */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();     // z-z-z… wakes in ISR → continues here
  sleep_disable();

  /* -------- wake path -------------------------------------------------- */
  detachInterrupt(digitalPinToInterrupt(g_buttonPin));
  detachInterrupt(digitalPinToInterrupt(g_pulsePin));

  railOn_();
  if (g_usePmos) {
    delay(100);      // rail settle before I²C pulls current
    i2cBegin();
  }

  display.setup();
  sensor.setup();

  /* Wait until all wake inputs are back LOW so we’ll catch the next edge */
  while (digitalRead(g_buttonPin) == HIGH ||
         digitalRead(g_pulsePin) == HIGH) {}

  /* -------- reset UI state & timers ----------------------------------- */
  lastActivityStamp = millis();
  showMainScreen = true;

  return g_wakeReason;
}
