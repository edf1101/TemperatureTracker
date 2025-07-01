#include <Arduino.h>
#include "Display.h"
#include "Sensor.h"

Sensor sensor;
Display display;


void setup() {
  display.setup();
  sensor.setup();
}

void loop() {

//  Sensor::Data data = sensor.readData();
//
//  display.displayMain(data.temperature, data.humidity);
//  delay(3000);

  float demoData[28] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                        27, 28};
  display.displayChart(demoData, false);
  delay(3000);

}

