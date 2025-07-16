// TimerController.cpp
/*
 * Created by Ed Fillingham on 15/07/2025.
*/
#ifdef TIMER_BOARD

#include "TimerController.h"
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/io.h>

ISR(RTC_CNT_vect) {
  // RTC Overflow Interrupt Service Routine
  RTC.INTFLAGS = RTC_OVF_bm;  // Clear the overflow flag by writing '1'
  // We do not toggle the LED here – just wake the CPU. The CPU will resume after sleep_cpu() and handle the LED.
}

void TimerController::setup() {
  pinMode(INTERRUPT_PIN, OUTPUT);
  digitalWrite(INTERRUPT_PIN, LOW);       // Start with LED off
  optimizePower();                 // Turn off unused peripherals and pins for low power
  initRTC();                       // Configure RTC (clock source, prescaler, interrupt)

  // Prepare sleep mode configuration (Standby mode gives lowest power with RTC active)
  set_sleep_mode(SLEEP_MODE_STANDBY);   // Use Standby so RTC can run
  sleep_enable();                      // Enable sleep (we will trigger it after setting up RTC each cycle)

}

void TimerController::loop() {
  // Blink the interrupt pin
  digitalWrite(INTERRUPT_PIN, HIGH);
  delay(PULSE_LENGTH);
  digitalWrite(INTERRUPT_PIN, LOW);


  // 2. Start the RTC timer for the OFF delay interval
  while (RTC.STATUS & RTC_CNTBUSY_bm) { /* wait if RTC registers are busy syncing */ }
  RTC.CNT = 0;                                      // Reset counter to 0 at start of interval
  RTC.PER = (uint16_t) (INTERVAL_SECONDS - 1);      // Set period = N-1 for N-second delay
  RTC.INTFLAGS = RTC_OVF_bm;                        // Clear any pending overflow flag
  RTC.CTRLA |= RTC_RTCEN_bm;                        // Enable RTC (starts counting now)
  // (We already enabled RTC_RUNSTDBY in initRTC, so it will keep running during sleep. Global interrupts for RTC already enabled.)

  // 3. Enter Standby Sleep – the device will sleep until the RTC triggers an interrupt
  sei();               // Enable global interrupts (so RTC interrupt can wake the CPU)
  sleep_cpu();         // Enter sleep. CPU will pause here until woken by RTC interrupt.
  // --- SLEEPING --- [During this time, current is ~1-2 µA, with RTC running off internal 32k clock] ---
  // Upon wake (RTC overflow), execution resumes here.

  // 4. We have woken up after OFF_DELAY_SECONDS. Disable RTC to conserve power while LED is about to blink.
  RTC.CTRLA &= ~RTC_RTCEN_bm;           // Disable RTC counting until next cycle (stop the 32k clock to save power a bit)

  // Loop repeats: LED on again, etc.
}

/**
 * Initializes the RTC to generate a 1 Hz tick using the internal 32k RC oscillator.
 */
void TimerController::initRTC() {
  // Select 32.768 kHz internal oscillator for RTC
  while (RTC.STATUS & RTC_CNTBUSY_bm) { /* wait */ }

  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;                    // Clock = internal 32.768kHz ULP oscillator
  // Set RTC prescaler to divide by 32768 to get 1 Hz ticks
  RTC.CTRLA = RTC_PRESCALER_DIV32768_gc                 // 32768 Hz / 32768 = 1 Hz
              | RTC_RUNSTDBY_bm;                        // Run RTC in Standby sleep
  // (RTC.ENABLE will be set later when we actually start the timer)

  // Enable the 32kHz oscillator in standby explicitly (good practice)
  CCP = CCP_IOREG_gc;                                   // Unlock protected IO
  CLKCTRL.OSC32KCTRLA |= CLKCTRL_RUNSTDBY_bm;           // Ensure internal 32k runs in standby

  // Enable RTC overflow interrupt:
  RTC.INTCTRL = RTC_OVF_bm;                            // Overflow interrupt enable (will wake from sleep)
  // Note: We set RTC.CTRLA’s ENABLE bit later, when starting each timing interval.
}

/**
 * Optimizes power consumption by configuring the timer and other components.
 */
void TimerController::optimizePower() {
  // Disable ADC to avoid current draw if not used
  ADC0.CTRLA &= ~ADC_ENABLE_bm;        // ADC off (if it was on by default)
  // Disable Analog Comparator (if not used)
  AC0.CTRLA &= ~AC_ENABLE_bm;          // AC off to save power
  // Optionally disable other analog modules (DAC, etc.) if present on this device.

  // Configure all unused pins to outputs (or inputs with pull-ups) to avoid floating inputs
  // Here we set all PORTA pins except the UPDI programming pin (PA0) and LED pin to outputs low.
  PORTA.DIRSET = PIN1_bm | PIN2_bm | PIN4_bm | PIN5_bm;  // Set PA1, PA2, PA4, PA5 as outputs (UPDI/PA0 left as input)
  PORTA.OUTCLR =
          PIN1_bm | PIN2_bm | PIN4_bm | PIN5_bm;  // Drive them low (avoids leakage through any attached circuitry)

  // If Brown-Out Detector (BOD) is enabled in fuses, it causes extra drain. For <4 µA, ensure BOD is disabled or in sample mode.
  // (BOD can only be configured via fuse settings – set BOD level to DISABLED or use Sleep mode BOD operation if available.)
}

#endif
