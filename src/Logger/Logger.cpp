#include "Logger.h"
#include <EEPROM.h>

#ifdef MAIN_BOARD

// Constants for the temperature and humidity encoding
float Logger::maxTemp = 60.0f;  // max temperature to store
float Logger::minTemp = -50.0f; // min temperature to store

float Logger::maxHum = 100.0f;  // max humidity to store
float Logger::minHum = 0.0f;    // min humidity to store

/**
 * Logger – persistent circular buffer in EEPROM.
 * @param sector  The sector number to use (0-3). Each sector is 57 bytes.
 */
void Logger::begin(uint8_t sector) {
  if (sector >= MAX_SECTORS) sector = 0;
  baseAddr = sector * SECTOR_SIZE;
}

/**
 * Read the front pointer from EEPROM.
 *
 * @return The current front pointer value (0-27).
 */
uint8_t Logger::readPtr() const { return EEPROM.read(baseAddr); }

/**
 * Write the front pointer to EEPROM.
 *
 * @param p The new front pointer value (0-27).
 */
void Logger::writePtr(uint8_t p) const { EEPROM.update(baseAddr, p); }

/**
 * Push one sample to the circular buffer in EEPROM.
 *
 * @param temp The temperature value to store, as a float.
 * @param hum  The humidity value to store, as a float.
 */
void Logger::push(float temp, float hum) {

  // convert float temps to uint8_t

  uint8_t p = readPtr();
  if (p >= NUM_SAMPLES) p = 0;

  EEPROM.update(offsTemp(p), encodeTemp(temp));
  EEPROM.update(offsHum(p), encodeHum(hum));

  /* advance pointer and store it */
  p = (p + 1) % NUM_SAMPLES;
  writePtr(p);
}

/**
 * Read the 28-entry temperature history (oldest → newest).
 *
 * @param dst Pointer to an array of 28 int8_t elements where the temperature values will be stored.
 */
void Logger::readTemperature(float *dst) {
  uint8_t p = readPtr();
  if (p >= NUM_SAMPLES) p = 0;

  for (uint8_t i = 0; i < NUM_SAMPLES; ++i) {
    uint8_t idx = (p + i) % NUM_SAMPLES;          // oldest → newest
    dst[i] = decodeTemp(EEPROM.read(offsTemp(idx)));
  }
}

/**
 * Read the 28-entry humidity history (oldest → newest).
 *
 * @param dst Pointer to an array of 28 int8_t elements where the humidity values will be stored.
 */
void Logger::readHumidity(float *dst) {
  uint8_t p = readPtr();
  if (p >= NUM_SAMPLES) p = 0;

  for (uint8_t i = 0; i < NUM_SAMPLES; ++i) {
    uint8_t idx = (p + i) % NUM_SAMPLES;
    dst[i] = decodeHum(EEPROM.read(offsHum(idx)));
  }
}

/**
 * Reset the EEPROM sector to all zeroes.
 */
void Logger::resetEEPROM() {
// on start write all eeprom to unsigned char 0
  for (uint16_t i = 0; i < SECTOR_SIZE; ++i) {
    EEPROM.update(baseAddr + i, 0x00);
  }
}

/**
 * Reset the EEPROM sector to all default values.
 *
 * @param defaultTemp The default temperature value to write to the EEPROM.
 * @param defaultHum  The default humidity value to write to the EEPROM.
 */
void Logger::resetEEMPROM(int8_t defaultTemp, int8_t defaultHum) {
  for (uint16_t i = 0; i < SECTOR_SIZE; ++i) {
    if (i == 0) {
      EEPROM.update(baseAddr + i, 0x00); // reset front pointer
    } else if (i <= NUM_SAMPLES) {
      EEPROM.update(baseAddr + i, encodeTemp(defaultTemp)); // write default temperature
    } else {
      EEPROM.update(baseAddr + i, encodeHum(defaultHum)); // write default humidity
    }
  }
}

/**
 * The temperatures are floats but we store them as uint8_t in EEPROM. Do the conversion to some precision here.
 *
 * @param temperature The temperature value to encode, as a float.
 * @return  Encoded temperature value as uint8_t.
 */
inline uint8_t Logger::encodeTemp(float temperature) {
  // clamp the value to the encoding range
  if (temperature < minTemp) temperature = minTemp;
  if (temperature > maxTemp) temperature = maxTemp;

  // scale to 0-255 range as a float
  float scaled = (temperature - minTemp) * 255.0f / (maxTemp - minTemp) + 0.5f;
  // return as uint8_t
  return static_cast<uint8_t>(scaled);
}

/**
 * Decode an EEPROM temperature value to float.
 * @param encodedTemp The encoded temperature value as uint8_t.
 * @return Decoded temperature value as float.
 */
inline float Logger::decodeTemp(uint8_t encodedTemp) {
  float encodedValue = static_cast<float>(encodedTemp); // scale back to the original range
  if (encodedValue == 0xFF) return 0.0f; // handle special case for 0xFF

  return (encodedTemp * (maxTemp - minTemp) / 255.0f) + minTemp; // return the decoded temperature
}

/**
 * The humidities are floats but we store them as uint8_t in EEPROM. Do the conversion to some precision here.
 *
 * @param humidity The humidity value to encode, as a float.
 * @return  Encoded humidity value as uint8_t.
 */
inline uint8_t Logger::encodeHum(float humidity) {
  // clamp the value to the encoding range
  if (humidity < minHum) humidity = minHum;
  if (humidity > maxHum) humidity = maxHum;

  // scale to 0-255 range as a float
  float scaledHum = (humidity - minHum) * 255.0f / (maxHum - minHum) +0.5f;
  // return as uint8_t
  return static_cast<uint8_t>(scaledHum);
}

/**
 * Decode an EEPROM humidity value to float.
 * @param encodedHum The encoded humidity value as uint8_t.
 * @return Decoded humidity value as float.
 */
inline float Logger::decodeHum(uint8_t encodedHum) {
  float encodedValue = static_cast<float>(encodedHum); // scale back to the original range
  if (encodedValue == 0xFF) return 0.0f; // handle special case for 0xFF

  return (encodedHum * (maxHum - minHum) / 255.0f) + minHum; // return the decoded humidity
}

#endif