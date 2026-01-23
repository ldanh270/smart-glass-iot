#include "display/OledDisplay.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

bool OledDisplay::begin() {
  // Khởi tạo I2C cho OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    return false;
  }
  printWrapped("Booting...");
  return true;
}

String OledDisplay::safeTruncate(const String& s, size_t maxLen) {
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen);
}

String OledDisplay::stripVietnamese(const String& in) {
#if STRIP_VIETNAMESE_FOR_OLED
  String out; out.reserve(in.length());
  for (size_t i = 0; i < in.length(); i++) {
    unsigned char c = (unsigned char)in[i];
    if (c < 128) { out += (char)c; continue; }

    // ngây thơ: bỏ qua UTF-8 đa byte và thay thế bằng '?'
    // Tốt hơn: triển khai bản đồ Unicode đầy đủ; ở đây giữ đơn giản để ổn định.
    out += '?';
  }
  return out;
#else
  return in;
#endif
}

void OledDisplay::printWrapped(const String& text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const int lineHeight = 10;
  const int maxLines = OLED_H / lineHeight;  // ~6 dòng
  const int maxCharsPerLine = 21;            // xấp xỉ cho font 5x7 mặc định ở kích thước 1

  String t = stripVietnamese(text);
  t.trim();

  int y = 0;
  int lineCount = 0;

  int start = 0;
  while (start < (int)t.length() && lineCount < maxLines) {
    int remaining = (int)t.length() - start;
    int take = min(remaining, maxCharsPerLine);

    int end = start + take;
    int breakPos = -1;

    for (int i = end; i > start; i--) {
      if (t[i - 1] == ' ') { breakPos = i - 1; break; }
    }

    if (breakPos == -1 || remaining <= maxCharsPerLine) {
      breakPos = min(start + maxCharsPerLine, (int)t.length());
    }

    String line = t.substring(start, breakPos);
    line.trim();

    display.setCursor(0, y);
    display.print(line);

    y += lineHeight;
    lineCount++;

    start = breakPos;
    while (start < (int)t.length() && t[start] == ' ') start++;
  }

  display.display();
}
