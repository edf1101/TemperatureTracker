/*
 * Created by Ed Fillingham on 15/07/2025.
*/

#ifndef TEMPERATURETRACKER_CONTROLLER_H
#define TEMPERATURETRACKER_CONTROLLER_H


class Controller {
public:
    virtual void setup() = 0;
    virtual void loop() = 0;
};


#endif //TEMPERATURETRACKER_CONTROLLER_H
