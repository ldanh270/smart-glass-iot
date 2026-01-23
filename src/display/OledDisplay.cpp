#include "display/OledDisplay.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Thư viện Adafruit SSD1306: tạo đối tượng display với kích thước từ `config.h`
static Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

// Khởi tạo OLED: bắt I2C và gọi begin của thư viện. Trả về false nếu thất bại.
bool OledDisplay::begin() {
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    return false;
  }
  printWrapped("Booting...");
  return true;
}

// Cắt chuỗi an toàn
String OledDisplay::safeTruncate(const String& s, size_t maxLen) {
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen);
}

// Tùy chọn: loại bỏ ký tự UTF-8 (dấu tiếng Việt) để tránh lỗi hiển thị trên OLED
// Nếu `STRIP_VIETNAMESE_FOR_OLED` = 1, sẽ thay các byte >=128 bằng '?'.
String OledDisplay::stripVietnamese(const String& in) {
#if STRIP_VIETNAMESE_FOR_OLED
  String out; out.reserve(in.length());
  for (size_t i = 0; i < in.length(); i++) {
    unsigned char c = (unsigned char)in[i];
    if (c < 128) { out += (char)c; continue; }

    // Cách làm đơn giản: bỏ qua UTF-8 đa byte và thay bằng '?'.
    // Ghi chú: cách tốt hơn là map Unicode -> ASCII, nhưng phức tạp hơn.
    out += '?';
  }
  return out;
#else
  return in;
#endif
}

// In văn bản theo dạng bọc (wrap) và chia thành nhiều dòng phù hợp với OLED
void OledDisplay::printWrapped(const String& text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const int lineHeight = 10;
  const int maxLines = OLED_H / lineHeight;  // số dòng tối đa hiển thị
  const int maxCharsPerLine = 21;            // ước lượng ký tự trên 1 dòng

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

    // Cố gắng ngắt ở khoảng trắng gần cuối để không cắt giữa từ
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

    // Bắt đầu đoạn tiếp theo, bỏ khoảng trắng dẫn đầu
    start = breakPos;
    while (start < (int)t.length() && t[start] == ' ') start++;
  }

  display.display();
}
