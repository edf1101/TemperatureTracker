/*
 * Power- & sleep-manager for ATTiny1614 projects
 *
 *  * Handles PMOS high-side rail
 *  * Puts TWI pins Hi-Z while the rail is off (no phantom powering)
 *  * Wraps enter-/exit-deep-sleep sequence
 */

#ifndef TEMPERATURETRACKER_POWERMANAGER_H
#define TEMPERATURETRACKER_POWERMANAGER_H

#include <Arduino.h>
#include "Display/Display.h"
#include "Sensor/Sensor.h"

/* call once from setup() -------------------------------------------------- */
void powerInit(uint8_t pmosGatePin, bool usePmos);

/* optionally drive the rail from user code (setup) ------------------------ */
void powerRailOn();
void powerRailOff();
enum WAKE_REASON {
    WAKE_BUTTON,
    WAKE_PULSE,
    WAKE_NONE
};

/* blocks until next button press, then re-initialises everything ---------- */
WAKE_REASON powerEnterDeepSleep(uint8_t   buttonPin,
                         uint8_t   pulsePin,      // <-- NEW
                         Display  &display,
                         Sensor   &sensor,
                         uint32_t &lastActivityStamp,
                         bool     &showMainScreen);



#endif //TEMPERATURETRACKER_POWERMANAGER_H
