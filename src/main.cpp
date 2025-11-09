#include "Arduino.h"


#include "Controllers/MainController.h"
Controller *controller = new MainController();


void setup() {
  controller->setup();
}


void loop() {
  controller->loop();
}
