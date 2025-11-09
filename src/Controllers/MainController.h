/*
 * Created by Ed Fillingham on 15/07/2025.
*/

#ifndef TEMPERATURETRACKER_MAINCONTROLLER_H
#define TEMPERATURETRACKER_MAINCONTROLLER_H

#ifdef MAIN_BOARD

#include "Controller.h"
#include "Arduino.h"
#include "Display/Display.h"
#include "Sensor/Sensor.h"
#include "Logger/Logger.h"

class MainController : public Controller {
public:
    void setup() override;

    void loop() override;

    static void rtcInterruptHandler();

private:

    constexpr static byte PUSH_BUTTON_PIN = 0; // pin for the push button
    constexpr static byte MEASUREMENT_INTERRUPT_PIN = 1; // pin for the measurement interrupt
    constexpr static byte POWER_CONTROL_PIN = 2; // pin for the power latch control pin
    constexpr static byte EEPROM_RESET_PIN = 3; // pin for the EEPROM reset button

    constexpr static unsigned long DISPLAY_UPDATE_INTERVAL = 250; // interval to update the display in ms
    constexpr static unsigned long POWER_OFF_TIMEOUT = 1000 * 10; // time in ms to shut off after last interaction
    constexpr static unsigned long BUTTON_DEBOUNCE_INTERVAL = 500; // interval to write to EEPROM in ms


    Sensor sensor; // object to read the sensor data (temp and humidity)
    Display display; // object to handle the display
    Logger logger = Logger(0); // object to read / write EEPROM

    unsigned long lastActivity = 0; // last time the user interacted with the device (used for powering off)
    unsigned long lastDisplayUpdate = 0; // last time the display was updated
    unsigned long lastButtonPress = 0; // last time the button was pressed (used for debouncing)

    enum MeasurementState {
        NO_MEASUREMENT, // No measurement taken yet
        MEASURE_ON_START, // Measurement taken on wakeup then going back to power off
        MEASURE_IN_MAIN // measurement happened while in main screen
    };
    MeasurementState measurementState = NO_MEASUREMENT; // current measurement state


    enum ScreenState : uint8_t {
        MAIN_SCREEN,
        TEMP_GRAPH,
        HUMIDITY_GRAPH,
    };
    ScreenState currentScreen = MAIN_SCREEN; // current screen state

    void powerOff(); // function to power off the device

    void updateDisplay(); // function to update the display based on the current screen state

    void takeMeasurement(); // function to take a measurement from the sensor

    void setupRTC();
};

#endif
#endif //TEMPERATURETRACKER_MAINCONTROLLER_H
