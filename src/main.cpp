#include "Arduino.h"


#ifdef MAIN_BOARD
#include "Controllers/MainController.h"
Controller *controller = new MainController();
#elif defined(TIMER_BOARD)
#include "Controllers/TimerController.h"
Controller *controller = new TimerController();
#endif

void setup() {
  controller->setup();
}


void loop() {
  controller->loop();
}
