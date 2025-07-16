// TimerController.h
/*
 * Created by Ed Fillingham on 15/07/2025.
*/
#ifndef TEMPERATURETRACKER_TIMERCONTROLLER_H
#define TEMPERATURETRACKER_TIMERCONTROLLER_H

#ifdef TIMER_BOARD

#include "Controller.h"
#include "Arduino.h"

class TimerController : public Controller {
public:
    void setup() override;
    void loop() override;
private:
    static constexpr uint8_t  INTERRUPT_PIN = 2;            // pin for the interrupt
    static constexpr uint32_t INTERVAL_SECONDS   = 60 * 40;   // sleep interval in seconds
    static constexpr uint32_t PULSE_LENGTH  = 1500UL;       // pulse length in ms

    void initRTC(); // RTC initialization for 1 Hz tick (using internal 32k RC oscillator)
    void optimizePower(); // Power-down preparation: disable unused peripherals, configure I/O for low leak
};

#endif
#endif // TEMPERATURETRACKER_TIMERCONTROLLER_H
