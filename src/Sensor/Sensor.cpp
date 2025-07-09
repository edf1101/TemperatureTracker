/*
 * Enhanced Sensor.cpp with better wake-up handling
 * Created by Ed Fillingham on 01/07/2025.
 * Enhanced for better sleep/wake reliability
 */

#include "Sensor.h"
#include <Wire.h>

/**
 * Write a single byte value to a BME280 register.
 */
void Sensor::writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

/**
 * Read an 8-bit unsigned value from the BME280.
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
 */
int16_t Sensor::readS16(uint8_t reg) {
  return (int16_t) read16(reg);
}

/**
 * Check if BME280 is responding and ready
 */
bool Sensor::isReady() {
  // Try to read chip ID register (should return 0x60)
  for (int attempts = 0; attempts < 5; attempts++) {
    uint8_t chipId = read8(0xD0);
    if (chipId == 0x60) {
      return true;
    }
    delay(10);
  }
  return false;
}

/**
 * Reads the BME280's factory calibration coefficients into class variables.
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
 * Applies the Bosch BME280 temperature compensation formula.
 */
int32_t Sensor::compensateTemperature(int32_t adc_T) {
  int32_t var1 = ((((adc_T >> 3) - ((int32_t) dig_T1 << 1))) *
                  ((int32_t) dig_T2)) >> 11;

  int32_t var2 = (((((adc_T >> 4) - ((int32_t) dig_T1)) *
                    ((adc_T >> 4) - ((int32_t) dig_T1))) >> 12) *
                  ((int32_t) dig_T3)) >> 14;

  t_fine = var1 + var2;
  return (t_fine * 5 + 128) >> 8;
}

/**
 * Applies the Bosch BME280 humidity compensation formula.
 */
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
  return (uint32_t) (v_x1 >> 12);
}

/**
 * Enhanced initialization with better error handling
 */
void Sensor::setup() {
  Wire.begin();
  delay(200); // Increased startup delay

  // Verify sensor is responding
  if (!isReady()) {
    // If sensor not ready, try full reset sequence
    reset();
    delay(100);
    if (!isReady()) {
      // Still not ready - this is a problem
      return;
    }
  }

  readCalibrationData();

  // Set humidity oversampling ×1
  writeRegister(0xF2, 0x01);
  // Set temperature oversampling ×1 and forced mode
  writeRegister(0xF4, 0x25);

  // Wait for initial measurement to complete
  delay(100);
}

/**
 * Enhanced reset with better timing
 */
void Sensor::reset() {
  // Send software reset command
  writeRegister(0xE0, 0xB6);
  delay(500); // Increased reset delay
}

/**
 * Enhanced wake function for post-sleep initialization
 */
void Sensor::wake() {
  // Re-initialize I2C
  Wire.begin();
  delay(100);

  // Check if sensor is responding
  if (!isReady()) {
    // Try reset if not responding
    reset();
    delay(100);
    if (!isReady()) {
      return; // Sensor not responding
    }
  }

  // Re-read calibration data (may have been lost)
  readCalibrationData();

  // Reconfigure sensor
  writeRegister(0xF2, 0x01); // Humidity oversampling
  writeRegister(0xF4, 0x25); // Temperature oversampling + forced mode

  delay(100);
}

/**
 * Enhanced data reading with validation
 */
Sensor::Data Sensor::readData() {
  // Check sensor is ready before reading
  if (!isReady()) {
    // Try to wake/reinitialize
    wake();
    if (!isReady()) {
      // Return error values
      return {.temperature = 0.0f, .humidity = 0.0f};
    }
  }

  // Trigger forced measurement
  writeRegister(0xF4, 0x25);
  delay(100); // Increased conversion delay

  // Check measurement is complete
  uint8_t status = read8(0xF3);
  int timeout = 0;
  while ((status & 0x08) && timeout < 10) { // Wait for measurement complete
    delay(10);
    status = read8(0xF3);
    timeout++;
  }

  // Read sensor data
  Wire.beginTransmission(BME280_ADDR);
  Wire.write(0xF7);
  Wire.endTransmission();
  Wire.requestFrom(BME280_ADDR, 8);

  // Skip pressure readings
  Wire.read();
  Wire.read();
  Wire.read();

  // Read temperature
  int32_t adc_T = ((uint32_t) Wire.read() << 12) |
                  ((uint32_t) Wire.read() << 4) |
                  (Wire.read() >> 4);

  // Read humidity
  int32_t adc_H = ((uint32_t) Wire.read() << 8) | Wire.read();

  // Validate readings (raw values shouldn't be 0x80000 or 0x8000)
  if (adc_T == 0x80000 || adc_H == 0x8000) {
    return {.temperature = 0.0f, .humidity = 0.0f};
  }

  // Compensate values
  int32_t temp = compensateTemperature(adc_T);
  uint32_t hum = compensateHumidity(adc_H);

  // Convert to float and validate ranges
  float temperature = temp / 100.0f;
  float humidity = hum / 1024.0f;

  // Sanity check - BME280 ranges
  if (temperature < -40.0f || temperature > 85.0f) {
    temperature = 0.0f;
  }
  if (humidity < 0.0f || humidity > 100.0f) {
    humidity = 0.0f;
  }

  return {.temperature = temperature, .humidity = humidity};
}

void Sensor::powerOff() {
  // Put sensor in sleep mode before powering off
  writeRegister(0xF4, 0x00); // Sleep mode
  delay(10);
  Wire.end();
}