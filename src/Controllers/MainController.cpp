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
 * Function to update the display based on the current screen state.
 */
void MainController::updateDisplay() {
  if (currentScreen == MAIN_SCREEN) { // Show the main screen with temperature and humidity
    Sensor::Data sensorData = sensor.readData(); // read the sensor data
    display.displayMain(sensorData.temperature, sensorData.humidity);
  } else if (currentScreen == TEMP_GRAPH) { // Show the temperature graph
    static int8_t temperatureHistory[28];
    logger.readTemperature(temperatureHistory);
    display.displayChart(temperatureHistory, true);
  } else if (currentScreen == HUMIDITY_GRAPH) { // Show the humidity graph
    static int8_t humidityHistory[28];
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
  int8_t temperature = static_cast<int8_t>(sensorData.temperature); // get the temperature from the sensor data
  int8_t humidity = static_cast<int8_t>(sensorData.humidity); // get the humidity from the sensor data

  logger.push(temperature, humidity); // write the measurement to the logger
}

#endif