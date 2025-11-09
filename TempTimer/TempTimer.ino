/*  --------- tinyAVR-0/1  –  10-s PIT wake-up, 2-s flash on pin 2 ----------
 *
 *  • Sleep mode:      Power-Down   (MCU core ≈ 0.7-1.0 µA with PIT running)
 *  • Wake source:     RTC PIT (1 Hz)  →  software counts 10 ticks
 *  • Flash pin:       Arduino pin 2  (PA2 on ’412 / ’1614)
 *
 *  Power-saving notes
 *  ------------------
 *  1.  BOD *must* be fused off (LVL=0, BODACT=0, BODPD=0).
 *  2.  DISABLE_MILLIS prevents the core from starting TCA0, so delay() costs 0.
 *  3.  All unused pins are driven low to stop input-buffer leakage.
 */

#define DISABLE_MILLIS         // removes millis()/micros() overhead
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

constexpr uint8_t LED_PIN       = 2;     // Arduino numbering

constexpr uint32_t WAKE_INTERVAL = 4UL*60*60;  // 14 400 s = 4 h
constexpr uint16_t FLASH_TIME   = 2000;  // msec LED is on

volatile bool pitFlag = false;           // set in ISR

ISR(RTC_PIT_vect)                // 1-Hz interrupt from PIT
{
  RTC.PITINTFLAGS = RTC_PI_bm;   // clear the flag
  pitFlag = true;
}

void setup()
{
  // ----- minimise quiescent current -----
  // Make every GPIO an output-low unless you actively use it
  PORTA.DIRSET = 0xFF;
  PORTA.OUTCLR = 0xFF;

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,HIGH);
  delay(2000);
  digitalWrite(LED_PIN, LOW);

  // Disable watchdog (saves a few µA if ever left on)
  WDT.CTRLA = 0;

  // ----- configure RTC PIT -----
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;           // 32 768 Hz ULP RC osc
  while (RTC.STATUS & RTC_CNTBUSY_bm);  // wait for sync

  RTC.PITCTRLA   = RTC_PITEN_bm | RTC_PERIOD_CYC32768_gc; // 1-Hz
  RTC.PITINTCTRL = RTC_PI_bm;                             // enable ISR

  sei();                         // global interrupts on
}

void loop()
{
static uint32_t seconds = 0;                   // 32-bit counter


  // --- SLEEP ---
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();                   // wakes on PIT ISR
  sleep_disable();

  if (!pitFlag) return;          // rare, but belt-and-braces
  pitFlag = false;

  // --- every 10 s do the user task ---
  if (++seconds >= WAKE_INTERVAL)
  {
    seconds = 0;

    digitalWrite(LED_PIN, HIGH);
    _delay_ms(FLASH_TIME);       // ~2 s active; DISABLE_MILLIS makes this cheap
    digitalWrite(LED_PIN, LOW);
  }
}
