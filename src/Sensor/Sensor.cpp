/*
 * Created by Ed Fillingham on 01/07/2025.
*/

#include "Sensor.h"
#include <Wire.h>

/**
 * Write a single byte value to a BME280 register.
 *
 * @param reg The register address to write to
 * @param val The value to write
 */
void Sensor::writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

/**
 * Read an 8-bit unsigned value from the BME280.
 *
 * @param reg The register address to read from
 * @return The 8-bit value read
 */
uint8_t Sensor::read8(uint8_t reg) {
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDR, 1);
  return Wire.read();
}

/**
 * Read a 16-bit unsigned value (LSB first) from the BME280.
 *
 * @param reg The starting register address
 * @return The 16-bit value read
 */
uint16_t Sensor::read16(uint8_t reg) {
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDR, 2);
  uint16_t val = Wire.read();
  val |= (Wire.read() << 8);
  return val;
}

/**
 * Read a 16-bit signed value from the BME280.
 *
 * @param reg The starting register address
 * @return The signed 16-bit value read
 */
int16_t Sensor::readS16(uint8_t reg) {
  return (int16_t) read16(reg);
}

/**
 * Reads the BME280's factory calibration coefficients into class variables.
 * These are needed to perform compensation calculations for raw sensor readings.
 */
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

/**
 * Applies the Bosch BME280 temperature compensation formula to raw temperature data.
 *
 * @param adc_T Raw temperature reading from sensor
 * @return Temperature in hundredths of a degree Celsius (°C × 100)
 */
int32_t Sensor::compensateTemperature(int32_t adc_T) {
  int32_t var1 = ((((adc_T >> 3) - ((int32_t) dig_T1 << 1))) *
                  ((int32_t) dig_T2)) >> 11;

  int32_t var2 = (((((adc_T >> 4) - ((int32_t) dig_T1)) *
                    ((adc_T >> 4) - ((int32_t) dig_T1))) >> 12) *
                  ((int32_t) dig_T3)) >> 14;

  t_fine = var1 + var2;
  return (t_fine * 5 + 128) >> 8; // Convert to °C × 100
}

/**
 * Applies the Bosch BME280 humidity compensation formula to raw humidity data.
 *
 * @param adc_H Raw humidity reading from sensor
 * @return Relative humidity in %RH × 1024
 */
uint32_t Sensor::compensateHumidity(int32_t adc_H) {
  int32_t v_x1 = t_fine - 76800;

  v_x1 = (((((adc_H << 14) - (((int32_t) dig_H4) << 20) -
             (((int32_t) dig_H5) * v_x1)) + 16384) >> 15) *
          (((((((v_x1 * ((int32_t) dig_H6)) >> 10) *
               (((v_x1 * ((int32_t) dig_H3)) >> 11) + 32768)) >> 10) + 2097152) *
            ((int32_t) dig_H2) + 8192) >> 14));

  // Clamp and return final value
  v_x1 = v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((int32_t) dig_H1)) >> 4);
  v_x1 = (v_x1 < 0) ? 0 : v_x1;
  v_x1 = (v_x1 > 419430400) ? 419430400 : v_x1;
  return (uint32_t) (v_x1 >> 12); // Convert to %RH × 1024
}

/**
 * Initializes the BME280 by beginning I2C, waiting for power-on,
 * reading calibration values, and setting oversampling/config registers.
 */
void Sensor::setup() {
  Wire.begin();    // Re-init bus cleanly
  delay(100); // BME280 power-up delay
  reset();
  readCalibrationData(); // Load calibration values

  // Set humidity oversampling ×1
  writeRegister(0xF2, 0x01);
  // Set temperature oversampling ×1 and forced mode
  writeRegister(0xF4, 0x25);
}

void Sensor::reset() {
  // Send software reset command to BME280
  writeRegister(0xE0, 0xB6);
  delay(300); // Wait for reboot
}

/**
 * Reads temperature and humidity from the BME280 and returns the compensated values.
 *
 * @return A Sensor::Data struct containing the temperature (°C) and humidity (%RH)
 */
Sensor::Data Sensor::readData() {
  // Trigger a forced measurement
  writeRegister(0xF4, 0x25);
  delay(50); // Wait for conversion to complete

  // Read 8 bytes starting at 0xF7 (pressure[3], temp[3], humidity[2])
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(0xF7);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDR, 8);

  Wire.read();
  Wire.read();
  Wire.read(); // Discard pressure bytes
  int32_t adc_T = ((uint32_t) Wire.read() << 12) |
                  ((uint32_t) Wire.read() << 4) |
                  (Wire.read() >> 4);
  int32_t adc_H = ((uint32_t) Wire.read() << 8) | Wire.read();

  int32_t temp = compensateTemperature(adc_T); // Temp in °C × 100
  uint32_t hum = compensateHumidity(adc_H);    // Humidity in % × 1024

  // Package into data struct and return as floating point values
  Sensor::Data data{
          .temperature = temp / 100.0f, // °C
          .humidity = hum / 1024.0f     // %RH
  };
  return data;
}

void Sensor::powerOff() {
  Wire.end();

}
