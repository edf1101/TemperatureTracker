/*
 * Enhanced Sensor.h with better wake-up handling
 * Created by Ed Fillingham on 01/07/2025.
 * Enhanced for better sleep/wake reliability
 */

#ifndef TEMPSENSOR_SENSOR_H
#define TEMPSENSOR_SENSOR_H

#include "Arduino.h"

#define BME280_ADDR 0x76

class Sensor {
public:
    struct Data {
        float temperature;
        float humidity;
    };

    void setup();
    void powerOff();
    void wake();
    bool isReady(); // New method to check sensor status

    Data readData();

private:
    uint16_t dig_T1;
    int16_t dig_T2, dig_T3;
    uint16_t dig_H1, dig_H3;
    int16_t dig_H2, dig_H4, dig_H5;
    int8_t dig_H6;
    int32_t t_fine;

    void writeRegister(uint8_t reg, uint8_t val);
    uint8_t read8(uint8_t reg);
    uint16_t read16(uint8_t reg);
    int16_t readS16(uint8_t reg);
    void readCalibrationData();
    int32_t compensateTemperature(int32_t adc_T);
    uint32_t compensateHumidity(int32_t adc_H);
    void reset();
};

#endif //TEMPSENSOR_SENSOR_H