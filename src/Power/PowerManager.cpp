#include "PowerManager.h"
#include <Wire.h>
#include <avr/sleep.h>

/* ------------- private state ------------------------------------------- */
static uint8_t g_pmosPin   = 0;
static bool    g_usePmos   = false;

static inline void railOn_()  { if (g_usePmos) digitalWrite(g_pmosPin, LOW ); }
static inline void railOff_() { if (g_usePmos) digitalWrite(g_pmosPin, HIGH); }

void powerRailOn()  { railOn_();  }
void powerRailOff() { railOff_(); }

static void i2cHiZ()
{
  Wire.end();
  pinMode(SDA, INPUT);
  pinMode(SCL, INPUT);
}
static void i2cBegin()
{
  Wire.begin();
}

/* ------------- wake-up ISR --------------------------------------------- */
static volatile bool g_wakeFlag = false;
static void wakeISR() { g_wakeFlag = true; }

/* ------------- public API ---------------------------------------------- */
void powerInit(uint8_t pmosGatePin, bool usePmos)
{
  g_pmosPin = pmosGatePin;
  g_usePmos = usePmos;

  pinMode(g_pmosPin, OUTPUT);
  railOff_();                          // rail defaults OFF while MCU boots
}

void powerEnterDeepSleep(uint8_t   buttonPin,
                         Display  &display,
                         Sensor   &sensor,
                         uint32_t &lastActivityStamp,
                         bool     &showMainScreen)
{
  /* ----- shut peripherals down ---------------------------------------- */
  display.powerDown();
  sensor.powerOff();

  if (g_usePmos)
  {
    i2cHiZ();
    railOff_();
  }

  /* ----- arm wake-up edge --------------------------------------------- */
  attachInterrupt(digitalPinToInterrupt(buttonPin), wakeISR, RISING);
  delay(5);      // debounce

  /* ----- full PWR-DOWN ------------------------------------------------- */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();                 // MCU sleeps → wakes in ISR → returns here
  sleep_disable();

  /* ----- wake path ----------------------------------------------------- */
  detachInterrupt(digitalPinToInterrupt(buttonPin));

  railOn_();
  if (g_usePmos)
  {
    delay(5);                  // rail settle
    i2cBegin();
  }

  display.setup();
  sensor.setup();

  /* wait until button released so next press is clean edge -------------- */
  while (digitalRead(buttonPin) == HIGH) {}

  /* ----- reset UI state & timers -------------------------------------- */
  lastActivityStamp = millis();
  showMainScreen    = true;
}
