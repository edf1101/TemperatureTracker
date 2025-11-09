/*
 * Created by Ed Fillingham on 15/07/2025.
*/
#ifdef MAIN_BOARD

#include "MainController.h"

/**
 * Setup function to initialize the main controller.
 */
void MainController::setup() {

  // first turn on the power latch
  pinMode(POWER_CONTROL_PIN, OUTPUT);
  digitalWrite(POWER_CONTROL_PIN, HIGH);

  // make the push button and measurement pins inputs
  pinMode(PUSH_BUTTON_PIN, INPUT);
  pinMode(MEASUREMENT_INTERRUPT_PIN, INPUT);

  if (digitalRead(MEASUREMENT_INTERRUPT_PIN) == HIGH) { // do this early in case it runs out
    measurementState = MEASURE_ON_START; // if the measurement pin is high, we have a measurement to process
  }

  // set up other components
  sensor.setup();
  display.setup();
  logger.begin();

  // Query the EEPROM reset pin to see if we need to reset the EEPROM
  pinMode(EEPROM_RESET_PIN, INPUT_PULLUP);
  if (digitalRead(EEPROM_RESET_PIN) == LOW) {
    logger.resetEEMPROM(20, 30); // if the reset pin is low, reset the EEPROM to arbitrary values
  }

  if (measurementState == MEASURE_ON_START) {
    delay(250); // delay a bit to allow the sensor to stabilize
    takeMeasurement();
    delay(500);
    powerOff(); // turn off the power latch after taking the measurement
    while (1); // The forever loop makes sure it doesn't go into the main loop
  }

  // set up the RTC for periodic interrupts
  setupRTC();
  sei();

  lastActivity = millis(); // set the last activity time to now
}

/**
 * Main loop function that runs continuously.
 */
void MainController::loop() {
  if (millis() - lastActivity > POWER_OFF_TIMEOUT) {
    powerOff(); // if the user has not interacted for a while, power off
  }

  if (digitalRead(PUSH_BUTTON_PIN) == HIGH &&
      millis() - lastButtonPress > BUTTON_DEBOUNCE_INTERVAL &&
      millis() > 1500 /* Don't switch in the first 1.5s to prevent accidentally going too far on start */) {
    lastButtonPress = millis();
    lastActivity = millis();
    currentScreen = static_cast<ScreenState>((currentScreen + 1) % 3); // cycle through the 3 screens
    updateDisplay(); // force a displayUpdate
  }

  if (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) { // update the display based on the current screen state
    lastDisplayUpdate = millis();
    updateDisplay();
  }

  // if the measurement pin is high and no measurement taken -> take a measurement
  if (measurementState == NO_MEASUREMENT && digitalRead(MEASUREMENT_INTERRUPT_PIN) == HIGH) {
    lastActivity = millis();
    measurementState = MEASURE_IN_MAIN; // set the measurement state to indicate a measurement was taken
    takeMeasurement(); // take a measurement from the sensor
  }
}
/**
 * This is the C-style ISR.
 * It's just a "wrapper" that calls our static class function.
 */
ISR(RTC_PIT_vect)
{
  MainController::rtcInterruptHandler(); // <-- CHANGED
}

/**
 * This is the static C++ class function that does the real work.
 * Being static, it can access static private members like POWER_CONTROL_PIN.
 */
void MainController::rtcInterruptHandler() // <-- NEW FUNCTION
{
  static volatile uint8_t totalSecondsOn = 0;
  RTC.PITINTFLAGS = RTC_PI_bm; // Clear the interrupt flag

  totalSecondsOn++;

  // After 60 seconds, force power off.
  if (totalSecondsOn >= 60) {
    digitalWrite(POWER_CONTROL_PIN, LOW); // <-- This now works!
  }
}

/**
 * Configures the RTC's Periodic Interrupt Timer (PIT) to fire every 1 second.
 */
void MainController::setupRTC() {
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc; // Use the 32.768kHz ULP oscillator
  while (RTC.STATUS & RTC_CNTBUSY_bm); // Wait for sync

  // Set period to 32768 cycles (1 second) and enable
  RTC.PITCTRLA   = RTC_PITEN_bm | RTC_PERIOD_CYC32768_gc;
  RTC.PITINTCTRL = RTC_PI_bm; // Enable the PIT interrupt
}

/**
 * Function to update the display based on the current screen state.
 */
void MainController::updateDisplay() {
  if (currentScreen == MAIN_SCREEN) { // Show the main screen with temperature and humidity
    Sensor::Data sensorData = sensor.readData(); // read the sensor data
    display.displayMain(sensorData.temperature, sensorData.humidity);
  } else if (currentScreen == TEMP_GRAPH) { // Show the temperature graph
    static float temperatureHistory[28];
    logger.readTemperature(temperatureHistory);
    display.displayChart(temperatureHistory, true);
  } else if (currentScreen == HUMIDITY_GRAPH) { // Show the humidity graph
    static float humidityHistory[28];
    logger.readHumidity(humidityHistory);
    display.displayChart(humidityHistory, false);
  }
}

/**
 * Function to power off the device by turning off the power latch.
 */
void MainController::powerOff() {
  delay(10);
  digitalWrite(POWER_CONTROL_PIN, LOW);
}

/**
 * Function to take a measurement from the sensor and update the state.
 */
void MainController::takeMeasurement() {
  Sensor::Data sensorData = sensor.readData(); // read the sensor data
  logger.push(sensorData.temperature, sensorData.humidity); // write the measurement to the logger
}

#endif