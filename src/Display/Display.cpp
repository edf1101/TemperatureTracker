/*
 * Created by Ed Fillingham on 01/07/2025.
*/

#include "Display.h"
#include "math.h"
#include "string.h"

// Initialize U8G2 library for 128x64 I2C OLED (no reset pin used)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C Display::u8g2 = U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);

// Font bitmap definitions for custom characters 6x8 pixels
const uint8_t Display::font6x8_digits[][6] PROGMEM = {
        // Each entry represents 1 character using 6 bytes (columns)
        // Digits 0–9
        {0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00},  // 0
        {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00},  // 1
        {0x42, 0x61, 0x51, 0x49, 0x46, 0x00},  // 2
        {0x21, 0x41, 0x45, 0x4B, 0x31, 0x00},  // 3
        {0x18, 0x14, 0x12, 0x7F, 0x10, 0x00},  // 4
        {0x27, 0x45, 0x45, 0x45, 0x39, 0x00},  // 5
        {0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00},  // 6
        {0x01, 0x71, 0x09, 0x05, 0x03, 0x00},  // 7
        {0x36, 0x49, 0x49, 0x49, 0x36, 0x00},  // 8
        {0x06, 0x49, 0x49, 0x29, 0x1E, 0x00},  // 9

        // Special characters: . d % -
        {0x00, 0x00, 0x60, 0x60, 0x00, 0x00},  // .
        {0x06, 0x09, 0x09, 0x06, 0x00, 0x00},  // d (degree)
        {0x62, 0x64, 0x08, 0x13, 0x23, 0x00},  // %
        {0x08, 0x08, 0x08, 0x08, 0x08, 0x00},  // -

        // Letters used for labels
        {0x01, 0x01, 0x7F, 0x01, 0x01, 0x00},  // T
        {0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00},  // H
        {0x7F, 0x49, 0x49, 0x49, 0x41, 0x00},  // E
        {0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00},  // M
        {0x7F, 0x09, 0x09, 0x09, 0x06, 0x00},  // P
        {0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00},  // U
        {0x00, 0x41, 0x7F, 0x41, 0x00, 0x00},  // I
        {0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00},  // D
};

// Maps a character to its index in the font array
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

  return 0xFF; // not found
}

// Draw a single character scaled by `scale` factor at (x0, y0)
void Display::drawCharScale(uint8_t x0, uint8_t y0, char c, const int scale) {
  uint8_t idx = charIndex(c);
  if (idx == 0xFF) return; // ignore unknown characters
  for (uint8_t col = 0; col < 6; col++) {
    uint8_t bits = pgm_read_byte(&font6x8_digits[idx][col]);
    for (uint8_t row = 0; row < 8; row++) {
      if (c == ' ') continue; // skip spaces
      if (bits & (1 << row)) {
        // draw each "on" pixel as a box of (scale x scale) size
        u8g2.drawBox(
                x0 + col * scale,
                y0 + row * scale,
                scale, scale
        );
      }
    }
  }
}

// Draw a scaled string starting at (x, y)
void Display::drawStringScale(uint8_t x, uint8_t y, const char *s, const int scale) {
  while (*s) {
    drawCharScale(x, y, *s++, scale);
    x += 6 * scale; // move cursor right by width of character
  }
}

const char *Display::formattedTempString(float temperature) {
  /**
   * Formats a temperature float into a 5-character string (e.g., "25.6d" or "-5.5d").
   *
   * @param temperature The float temperature, which may be negative and have decimals.
   * @return A pointer to a static char array formatted as a 5-character C string.
   */

  static char result[7];  // buffer to store final string with null terminator

  // Determine sign
  bool negative = (temperature < 0);
  if (negative) temperature = -temperature;

  // Separate integer part
  unsigned int whole = (unsigned int) temperature;

  // Get two digits after the decimal point
  unsigned int digs2AfterPoint = int(temperature * 100 - (floor(temperature)) * 100);

  // Format output string depending on range and sign
  if (!negative) {
    if (whole < 10) {
      // X.XXd format
      result[0] = '0' + whole;
      result[1] = '.';
      result[2] = '0' + (digs2AfterPoint / 10);
      result[3] = '0' + (digs2AfterPoint % 10);
      result[4] = 'd';
      result[5] = '\0';
    } else if (whole < 100) {
      // XX.Xd format
      result[0] = '0' + (whole / 10);
      result[1] = '0' + (whole % 10);
      result[2] = '.';
      result[3] = '0' + (digs2AfterPoint / 10);
      result[4] = 'd';
      result[5] = '\0';
    } else {
      // XXXd format (no decimals)
      result[0] = '0' + ((whole / 100) % 10);
      result[1] = '0' + ((whole / 10) % 10);
      result[2] = '0' + (whole % 10);
      result[3] = 'd';
      result[4] = '\0';
    }
  } else {
    if (whole < 10) {
      // -X.Xd format
      result[0] = '-';
      result[1] = '0' + whole;
      result[2] = '.';
      result[3] = '0' + (digs2AfterPoint / 10);
      result[4] = 'd';
      result[5] = '\0';
    } else if (whole < 100) {
      // -XXd format
      result[0] = '-';
      result[1] = '0' + (whole / 10);
      result[2] = '0' + (whole % 10);
      result[3] = 'd';
      result[4] = '\0';
    } else {
      // -XXXd format
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
  /**
   * Formats humidity (0–100) into a 5-character string like "55.0%".
   *
   * @param humidity A float humidity value.
   * @return Pointer to a static char buffer.
   */

  static char result[6];

  // Clamp humidity to safe range
  if (humidity < 0) humidity = 0;
  if (humidity > 100) humidity = 100;

  unsigned int whole = (unsigned int) humidity;
  unsigned int digs2AfterPoint = (unsigned int) ((humidity - whole) * 100);

  if (whole < 10) {
    // X.XX%
    result[0] = '0' + whole;
    result[1] = '.';
    result[2] = '0' + (digs2AfterPoint / 10);
    result[3] = '0' + (digs2AfterPoint % 10);
    result[4] = '%';
    result[5] = '\0';
  } else if (whole < 100) {
    // XX.X%
    result[0] = '0' + (whole / 10);
    result[1] = '0' + (whole % 10);
    result[2] = '.';
    result[3] = '0' + (digs2AfterPoint / 10);
    result[4] = '%';
    result[5] = '\0';
  } else {
    // 100 %
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
  // Initialize the display
  u8g2.begin();
}

void Display::displayMain(float temperature, float humidity) {
  // Draw temperature and humidity to screen
  u8g2.clearBuffer();

  // T: temperature
  drawCharScale(0, 0, 'T', 2);
  drawStringScale(12, 0, formattedTempString(temperature), 4);

  // H: humidity
  drawCharScale(0, 36, 'H', 2);
  drawStringScale(12, 36, formattedHumString(humidity), 4);

  // Push buffer to screen
  u8g2.sendBuffer();
}

void Display::displayChart(signed char data[28], bool temp) {
  /**
   * Draws a bar chart with 28 float values for temperature or humidity.
   *
   * @param data Array of 28 float values to plot
   * @param temp If true, it's a temperature chart; otherwise humidity
   */

  // Find min and max values in dataset
  signed char maxVal = -127;
  signed char minVal = 126;
  for (int i = 0; i < 28; i++) {
    if (data[i] > maxVal) maxVal = data[i];
    if (data[i] < minVal) minVal = data[i];
  }

  u8g2.clearBuffer();

  for (int i = 0; i < 28; i++) {
    // Scale bar height to 53px max
    int barHeight = (int) ((float) (data[i] - minVal) / (float) (maxVal - minVal) * 53.0f);
    if (barHeight < 0) barHeight = 0;
    if (barHeight > 64) barHeight = 64;

    // Draw each bar: x offset starts at 16, each bar is 3px wide
    u8g2.drawBox(16 + i * 4, 64 - barHeight, 3, barHeight);
  }

  // Title: TEMP or HUMID
  const char *title = temp ? "TEMP" : "HUMID";
  int strSize = (int) strlen(title) * 6;
  int startPoint = 72 - (strSize / 2);
  drawStringScale(startPoint, 0, title, 1);

  // Y-axis labels
  drawStringScale(0, 12, formatAxisLabels((int) maxVal), 1);
  drawStringScale(0, 56, formatAxisLabels((int) minVal), 1);

  // Axes lines
  u8g2.drawLine(16, 64, 16, 0);   // y-axis
  u8g2.drawLine(0, 64, 128, 64);  // x-axis
  u8g2.drawLine(16, 10, 128, 10); // top cap

  u8g2.sendBuffer();
}

char *Display::formatAxisLabels(int value) {
  /**
   * Format chart axis labels for integers.
   * Returns 1–3 digit strings like "-9", "12", "100"
   */

  static char label[4];

  int absValue = (value < 0 ? -value : value);
  if (value < -9 && value > -100) {
    label[0] = '-';
    label[1] = '0' + (absValue / 10);
    label[2] = '0' + (absValue % 10);
    label[3] = '\0';
  } else if (value < 0 && value > -10) {
    label[0] = '-';
    label[1] = '0' + (absValue);
    label[2] = ' ';
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

void Display::powerDown() {
  u8g2.setPowerSave(1);
}
