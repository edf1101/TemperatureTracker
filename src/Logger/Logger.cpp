#include "Logger.h"
#include <EEPROM.h>

void Logger::begin(uint8_t sector)
{
  if (sector >= MAX_SECTORS) sector = 0;
  baseAddr = sector * SECTOR_SIZE;

  /* Validate front pointer – if corrupt, reset to 0 */
  uint8_t p = EEPROM.read(baseAddr);
  if (p >= NUM_SAMPLES) writePtr(0);
}

uint8_t Logger::readPtr() const        { return EEPROM.read(baseAddr); }
void    Logger::writePtr(uint8_t p) const { EEPROM.update(baseAddr, p); }

void Logger::push(int8_t temp, int8_t hum)
{
  uint8_t p = readPtr();
  if (p >= NUM_SAMPLES) p = 0;

  EEPROM.update(offsTemp(p), static_cast<uint8_t>(temp));
  EEPROM.update(offsHum (p), static_cast<uint8_t>(hum));

  /* advance pointer and store it */
  p = (p + 1) % NUM_SAMPLES;
  writePtr(p);
}

static inline int8_t decode(uint8_t raw) { return raw == 0xFF ? 0 : static_cast<int8_t>(raw); }

void Logger::readTemperature(int8_t *dst)
{
  uint8_t p = readPtr();
  if (p >= NUM_SAMPLES) p = 0;

  for (uint8_t i = 0; i < NUM_SAMPLES; ++i) {
    uint8_t idx = (p + i) % NUM_SAMPLES;          // oldest → newest
    dst[i] = decode(EEPROM.read(offsTemp(idx)));
  }
}

void Logger::readHumidity(int8_t *dst)
{
  uint8_t p = readPtr();
  if (p >= NUM_SAMPLES) p = 0;

  for (uint8_t i = 0; i < NUM_SAMPLES; ++i) {
    uint8_t idx = (p + i) % NUM_SAMPLES;
    dst[i] = decode(EEPROM.read(offsHum(idx)));
  }
}
