/*
 * Created by Ed Fillingham on 01/07/2025.
*/

#include "Display.h"
#include "math.h"
#include "string.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C Display::u8g2 = U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, /* reset=*/
                                                                                        U8X8_PIN_NONE);


const uint8_t Display::font6x8_digits[][6] PROGMEM = {
        // 0
        {0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00},
        // 1
        {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00},
        // 2
        {0x42, 0x61, 0x51, 0x49, 0x46, 0x00},
        // 3
        {0x21, 0x41, 0x45, 0x4B, 0x31, 0x00},
        // 4
        {0x18, 0x14, 0x12, 0x7F, 0x10, 0x00},
        // 5
        {0x27, 0x45, 0x45, 0x45, 0x39, 0x00},
        // 6
        {0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00},
        // 7
        {0x01, 0x71, 0x09, 0x05, 0x03, 0x00},
        // 8
        {0x36, 0x49, 0x49, 0x49, 0x36, 0x00},
        // 9
        {0x06, 0x49, 0x49, 0x29, 0x1E, 0x00},
        // '.'
        {0x00, 0x00, 0x60, 0x60, 0x00, 0x00},
        // 'd' (degree)
        {0x06, 0x09, 0x09, 0x06, 0x00, 0x00},
        // '%'
        {0x62, 0x64, 0x08, 0x13, 0x23, 0x00},
        // '-' (minus sign)
        {0x08, 0x08, 0x08, 0x08, 0x08, 0x00},
        // 'T'
        {0x01, 0x01, 0x7F, 0x01, 0x01, 0x00},
        // 'H'
        {0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00},
        // 'E'
        {0x7F, 0x49, 0x49, 0x49, 0x41, 0x00},
        // 'M'
        {0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00},
        // 'P'
        {0x7F, 0x09, 0x09, 0x09, 0x06, 0x00},
        // 'U'
        {0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00},
        // 'I'
        {0x00, 0x41, 0x7F, 0x41, 0x00, 0x00},
        // 'D'
        {0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00},
};

uint8_t Display::charIndex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c == '.') return 10;
  if (c == 'd') return 11;
  if (c == '%') return 12;
  if (c == '-') return 13;
  if (c == 'T') return 14;
  if (c == 'H') return 15;
  if (c == 'E') return 16;
  if (c == 'M') return 17;
  if (c == 'P') return 18;
  if (c == 'U') return 19;
  if (c == 'I') return 20;
  if (c == 'D') return 21;


  return 0xFF;
}

void Display::drawCharScale(uint8_t x0, uint8_t y0, char c, const int scale) {
  uint8_t idx = charIndex(c);
  if (idx == 0xFF) return;
  for (uint8_t col = 0; col < 6; col++) {
    uint8_t bits = pgm_read_byte(&font6x8_digits[idx][col]);
    for (uint8_t row = 0; row < 8; row++) {
      if (c == ' ') continue;
      if (bits & (1 << row)) {
        u8g2.drawBox(
                x0 + col * scale,
                y0 + row * scale,
                scale, scale
        );
      }
    }
  }
}

void Display::drawStringScale(uint8_t x, uint8_t y, const char *s, const int scale) {
  while (*s) {
    drawCharScale(x, y, *s++, scale);
    x += 6 * scale;
  }
}

const char *Display::formattedTempString(float temperature) {
  /**
   * This function formats a temperature float that has any precision and can be negative and formats it as a
   * 5 char (incl. deg sign excl. \0) C String.
   *
   * @param temperature The input temperature
   * @return A formatted C string representing the temperature
   */


  static char result[7];  // enough space for "-xx.xd\0"
  bool negative = (temperature < 0);
  if (negative) temperature = -temperature;

  unsigned int whole = (unsigned int) temperature;

  // Extract decimal digits:
  unsigned int digs2AfterPoint = int(temperature * 100 - (floor(temperature)) * 100);
  if (!negative) {
    if (whole < 10) {
      // Format: X.XXd e.g. 9.56d
      result[0] = '0' + whole;
      result[1] = '.';
      result[2] = '0' + (digs2AfterPoint / 10);
      result[3] = '0' + (digs2AfterPoint % 10);
      result[4] = 'd';
      result[5] = '\0';
    } else if (whole < 100) {
      // Format: XX.Xd e.g. 25.6d
      result[0] = '0' + (whole / 10);
      result[1] = '0' + (whole % 10);
      result[2] = '.';
      result[3] = '0' + (digs2AfterPoint / 10);
      result[4] = 'd';
      result[5] = '\0';
    } else {
      // Format: XXXd (no decimals)
      // max 3 digit integer assumed
      result[0] = '0' + ((whole / 100) % 10);
      result[1] = '0' + ((whole / 10) % 10);
      result[2] = '0' + (whole % 10);
      result[3] = 'd';
      result[4] = '\0';
    }
  } else {
    if (whole < 10) {
      // Format: -X.Xd e.g. -5.5d
      result[0] = '-';
      result[1] = '0' + whole;
      result[2] = '.';
      result[3] = '0' + (digs2AfterPoint / 10);
      result[4] = 'd';
      result[5] = '\0';
    } else if (whole < 100) {
      // Format: -XXd e.g. -10d
      result[0] = '-';
      result[1] = '0' + (whole / 10);
      result[2] = '0' + (whole % 10);
      result[3] = 'd';
      result[4] = '\0';
    } else {
      // Format: -XXXd no decimals, max 3-digit
      result[0] = '-';
      result[1] = '0' + ((whole / 100) % 10);
      result[2] = '0' + ((whole / 10) % 10);
      result[3] = '0' + (whole % 10);
      result[4] = 'd';
      result[5] = '\0';
    }
  }

  return result;
}

char *Display::formattedHumString(float humidity) {
  // This function formats a humidity float 0 - 100 range and returns it in a 5 char (incl. % excl /0) C string
  static char result[6];


  // If hum = 100 then out = 100 %
  // INP      OUT
  // 5.5678   5.56%
  // 5.0000   5.00%
  // 10.2456  10.2%
  // 100.2243 100 %
  // 0.013    0.01%

  // constrain humidity to 0 - 100 range
  if (humidity < 0) humidity = 0;
  if (humidity > 100) humidity = 100;

  unsigned int whole = (unsigned int) humidity;
  unsigned int digs2AfterPoint = (unsigned int) ((humidity - whole) * 100);

  if (whole < 10) {
    // Format: X.XX%
    result[0] = '0' + whole;
    result[1] = '.';
    result[2] = '0' + (digs2AfterPoint / 10);
    result[3] = '0' + (digs2AfterPoint % 10);
    result[4] = '%';
    result[5] = '\0';
  } else if (whole < 100) {
    // Format: XX.X%
    result[0] = '0' + (whole / 10);
    result[1] = '0' + (whole % 10);
    result[2] = '.';
    result[3] = '0' + (digs2AfterPoint / 10);
    result[4] = '%';
    result[5] = '\0';
  } else {
    // Format: 100%
    result[0] = '1';
    result[1] = '0';
    result[2] = '0';
    result[3] = ' ';
    result[4] = '%';
    result[5] = '\0';
  }

  return result;
}

void Display::setup() {
  u8g2.begin();
}

void Display::displayMain(float temperature, float humidity) {
  u8g2.clearBuffer();

  drawCharScale(0, 0, 'T', 2);
  drawStringScale(12, 0, formattedTempString(temperature), 4);

  drawCharScale(0, 36, 'H', 2);
  drawStringScale(12, 36, formattedHumString(humidity), 4);

  u8g2.sendBuffer();

}

void Display::displayChart(float data[28], bool temp) {
  /**
   * Draw a bar chart with 28 data points. The bars start at x:16 and are 4 wide each
   *
   * @param data An array of 28 float values representing the data points. They are in degrees
   * @param temp If true, the chart is for temperature, otherwise for humidity
   */

  // get max and min values to scale the chart
  float maxVal = -1000.0f;
  float minVal = 1000.0f;
  for (int i = 0; i < 28; i++) {
    if (data[i] > maxVal) maxVal = data[i];
    if (data[i] < minVal) minVal = data[i];
  }

  u8g2.clearBuffer();
  for (int i = 0; i < 28; i++) {
    // scale the data to fit the display height
    int barHeight = (int) ((data[i] - minVal) / (maxVal - minVal) * 53);
    if (barHeight < 0) barHeight = 0;
    if (barHeight > 64) barHeight = 64;

    // draw the bar
    u8g2.drawBox(16 + i * 4, 64 - barHeight, 3, barHeight);
  }

  char *title;
  int strSize;
  if (temp) {
    // draw title
    title = "TEMP";
    strSize = (int) strlen(title) * 6; // each char is 6 pixels wide
  } else {
    // draw title
    title = "HUMID";
    strSize = (int) strlen(title) * 6; // each char is 6 pixels wide
  }

  int startPoint = 72 - (strSize / 2); // center the title
  drawStringScale(startPoint, 0, title, 1);

  // draw labels
  drawStringScale(0, 12, formatAxisLabels((int)maxVal), 1);
  drawStringScale(0, 56, formatAxisLabels((int)minVal), 1);

  // draw axes
  u8g2.drawLine(16, 64, 16, 0); // y-axis
  u8g2.drawLine(0, 64, 128, 64); // x-axis
  u8g2.drawLine(16, 10, 128, 10); // top bar

  u8g2.sendBuffer();
}

char *Display::formatAxisLabels(int value) {
  /**
   * Creates a formatted string for axis labels based on the value. In an ideal world it is only 2 characters long.
   * If the value is <-10 or >=100 ONLY then can it take 3 characters.
   */

  static char label[4]; // enough space for "-99\0" or "100\0"

  int absValue = (value < 0 ? -value : value); // absolute value for formatting
  if (value < -9 && value > -100) {
    label[0] = '-';
    label[1] = '0' + (absValue / 10);
    label[2] = '0' + (absValue % 10);
    label[3] = '\0';
  } else if (value >= 0 && value < 10) {
    label[0] = '0' + value;
    label[1] = '\0';
  } else if (value >= 10 && value < 100) {
    label[0] = '0' + (value / 10);
    label[1] = '0' + (value % 10);
    label[2] = '\0';
  } else if (value >= 100) {
    label[0] = '1';
    label[1] = '0';
    label[2] = '0';
    label[3] = '\0';
  }

  return label;
}


