/*
 * Created by Ed Fillingham on 01/07/2025.
*/

#include "Sensor.h"

#include <Wire.h>


void Sensor::writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t Sensor::read8(uint8_t reg) {
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDR, 1);
  return Wire.read();
}

uint16_t Sensor::read16(uint8_t reg) {
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDR, 2);
  uint16_t val = Wire.read();
  val |= (Wire.read() << 8);
  return val;
}

int16_t Sensor::readS16(uint8_t reg) {
  return (int16_t) read16(reg);
}

void Sensor::readCalibrationData() {
  dig_T1 = read16(0x88);
  dig_T2 = readS16(0x8A);
  dig_T3 = readS16(0x8C);

  dig_H1 = read8(0xA1);
  dig_H2 = readS16(0xE1);
  dig_H3 = read8(0xE3);
  dig_H4 = (read8(0xE4) << 4) | (read8(0xE5) & 0x0F);
  dig_H5 = (read8(0xE6) << 4) | (read8(0xE5) >> 4);
  dig_H6 = (int8_t) read8(0xE7);
}

int32_t Sensor::compensateTemperature(int32_t adc_T) {
  int32_t var1 = ((((adc_T >> 3) - ((int32_t) dig_T1 << 1))) *
                  ((int32_t) dig_T2)) >> 11;

  int32_t var2 = (((((adc_T >> 4) - ((int32_t) dig_T1)) *
                    ((adc_T >> 4) - ((int32_t) dig_T1))) >> 12) *
                  ((int32_t) dig_T3)) >> 14;

  t_fine = var1 + var2;
  return (t_fine * 5 + 128) >> 8;
}

uint32_t Sensor::compensateHumidity(int32_t adc_H) {
  int32_t v_x1 = t_fine - 76800;
  v_x1 = (((((adc_H << 14) - (((int32_t) dig_H4) << 20) -
             (((int32_t) dig_H5) * v_x1)) + 16384) >> 15) *
          (((((((v_x1 * ((int32_t) dig_H6)) >> 10) *
               (((v_x1 * ((int32_t) dig_H3)) >> 11) + 32768)) >> 10) + 2097152) *
            ((int32_t) dig_H2) + 8192) >> 14));

  v_x1 = v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((int32_t) dig_H1)) >> 4);
  v_x1 = (v_x1 < 0) ? 0 : v_x1;
  v_x1 = (v_x1 > 419430400) ? 419430400 : v_x1;
  return (uint32_t) (v_x1 >> 12); // in %RH * 1024
}

void Sensor::setup() {
  Wire.begin();
  delay(100); // BME280 power-up delay

  readCalibrationData();

  // ctrl_hum (oversampling = x1)
  writeRegister(0xF2, 0x01);
  // ctrl_meas (temp oversampling = x1, mode = forced)
  writeRegister(0xF4, 0x25);
}

Sensor::Data Sensor::readData() {
  // Trigger a measurement (forced mode)
  writeRegister(0xF4, 0x25);
  delay(50); // Wait for measurement (~10ms typical)

  // Read raw data
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(0xF7);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDR, 8);

  Wire.read();
  Wire.read();
  Wire.read(); // pressure (unused)
  int32_t adc_T = ((uint32_t) Wire.read() << 12) | ((uint32_t) Wire.read() << 4) | (Wire.read() >> 4);
  int32_t adc_H = ((uint32_t) Wire.read() << 8) | Wire.read();

  int32_t temp = compensateTemperature(adc_T); // in °C * 100
  uint32_t hum = compensateHumidity(adc_H);    // in %RH * 1024

  Sensor::Data data{
          .temperature = temp / 100.0f, // Convert to °C
          .humidity = hum / 1024.0f      // Convert to %RH
  };
  return data;
}
