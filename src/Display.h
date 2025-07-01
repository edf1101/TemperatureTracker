/*
 * Created by Ed Fillingham on 01/07/2025.
*/

#ifndef TEMPSENSOR_DISPLAY_H
#define TEMPSENSOR_DISPLAY_H

#include <U8g2lib.h>
#include <avr/pgmspace.h>

class Display {
public:
    void setup();

    void displayMain(float temperature, float humidity);

    void displayChart(float *data, bool temp);


private:
    static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    static const uint8_t font6x8_digits[][6] PROGMEM;

    uint8_t charIndex(char c);

    void drawCharScale(uint8_t x0, uint8_t y0, char c, const int scale);

    void drawStringScale(uint8_t x, uint8_t y, const char *s, const int scale);

    const char *formattedTempString(float temperature);

    char *formattedHumString(float humidity);

    char* formatAxisLabels(int value);

};


#endif //TEMPSENSOR_DISPLAY_H
