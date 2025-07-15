/**
 * Logger – persistent circular buffer in EEPROM.
 *
 * Layout inside one 57-byte sector:
 *   [0]          : uint8_t frontPtr  (index of NEXT cell to be written, 0-27)
 *   [1 .. 28]    : int8_t  temp[28]  (latest temperatures)
 *   [29 .. 56]   : int8_t  hum[28]   (latest humidities)
 */

#ifndef TEMPERATURETRACKER_LOGGER_H
#define TEMPERATURETRACKER_LOGGER_H


#include <Arduino.h>

class Logger {
public:
    static constexpr uint8_t NUM_SAMPLES = 28;
    static constexpr uint8_t SECTOR_SIZE = NUM_SAMPLES * 2 + 1; // 57
    static constexpr uint8_t MAX_SECTORS = 4;                   // fits 256-byte EEPROM

    explicit Logger(uint8_t sector = 0) { begin(sector); }

    /** Select which 57-byte sector to use (0-3). */
    void begin(uint8_t sector = 0);

    /** Push one sample; oldest data is overwritten when the buffer wraps. */
    void push(int8_t temp, int8_t hum);

    /** Read the 28-entry history (oldest → newest). Empty cells return 0. */
    void readTemperature(int8_t *dst);  // dst[28]
    void readHumidity   (int8_t *dst);  // dst[28]

    void resetEEPROM();
    void resetEEMPROM(int8_t defaultTemp = 0, int8_t defaultHum = 0);

private:
    uint8_t baseAddr = 0;   // start address of chosen sector

    uint8_t  readPtr()             const;
    void     writePtr(uint8_t ptr) const;
    uint16_t offsTemp(uint8_t i)   const { return baseAddr + 1 + i; }
    uint16_t offsHum (uint8_t i)   const { return baseAddr + 1 + NUM_SAMPLES + i; }
};

#endif //TEMPERATURETRACKER_LOGGER_H
