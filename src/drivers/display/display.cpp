#include "display.h"

// Đối tượng OLED global
static Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

// Khởi tạo OLED: cấu hình I2C và gọi begin() của thư viện
bool OledDisplay::init() {
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    return false;
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  
  return true;
}

// Xóa toàn bộ hiển thị
void OledDisplay::clear() {
  display.clearDisplay();
  display.display();
}

// In text tại tọa độ (x, y)
void OledDisplay::print(uint16_t x, uint16_t y, const String& text) {
  String t = stripVietnamese(text);
  t = safeTruncate(t, 128); // Giới hạn tối đa 128 ký tự cho an toàn
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.println(t);
  display.display();
}

// In text với auto-wrap từ (x, y), chia thành nhiều dòng
void OledDisplay::printWrapped(uint16_t x, uint16_t y, const String& text) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const int lineHeight = 10;  // Cao độ mỗi dòng (pixel)
  const int maxLines = (OLED_H - y) / lineHeight;  // Số dòng tối đa từ vị trí y
  const int maxCharsPerLine = 21;  // Ước lượng ký tự trên 1 dòng (font size 1)

  String t = stripVietnamese(text);
  t = safeTruncate(t, 256);  // Giới hạn 256 ký tự
  t.trim();

  int currentY = y;
  int lineCount = 0;
  int start = 0;

  while (start < (int)t.length() && lineCount < maxLines) {
    int remaining = (int)t.length() - start;
    int take = min(remaining, maxCharsPerLine);
    int end = start + take;
    int breakPos = -1;

    // Cố gắng ngắt ở khoảng trắng gần cuối để không cắt giữa từ
    for (int i = end; i > start; i--) {
      if (t[i - 1] == ' ') {
        breakPos = i - 1;
        break;
      }
    }

    if (breakPos == -1 || remaining <= maxCharsPerLine) {
      breakPos = min(start + maxCharsPerLine, (int)t.length());
    }

    String line = t.substring(start, breakPos);
    line.trim();

    // Hiển thị dòng
    if (line.length() > 0) {
      display.setCursor(x, currentY);
      display.println(line);
    }

    currentY += lineHeight;
    lineCount++;

    // Bắt đầu đoạn tiếp theo, bỏ khoảng trắng dẫn đầu
    start = breakPos;
    while (start < (int)t.length() && t[start] == ' ') start++;
  }

  display.display();
}

// Cắt chuỗi text an toàn (tránh text dài quá gây tràn hiển thị hoặc tốn RAM)
String OledDisplay::safeTruncate(const String& s, size_t maxLen) {
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen);
}

// Chuyển Unicode ký tự Việt thành ASCII không dấu
char OledDisplay::vietneseToAscii(uint16_t u) {
  // ASCII thì giữ nguyên
  if (u < 128) return (char)u;

  switch (u) {
    // ===== d / D =====
    case 0x0111: return 'd'; // đ
    case 0x0110: return 'D'; // Đ

    // ===== a / A =====
    case 0x00E1: case 0x00E0: case 0x1EA3: case 0x00E3: case 0x1EA1: return 'a';
    case 0x00C1: case 0x00C0: case 0x1EA2: case 0x00C3: case 0x1EA0: return 'A';

    case 0x0103: case 0x1EAF: case 0x1EB1: case 0x1EB3: case 0x1EB5: case 0x1EB7: return 'a';
    case 0x0102: case 0x1EAE: case 0x1EB0: case 0x1EB2: case 0x1EB4: case 0x1EB6: return 'A';

    case 0x00E2: case 0x1EA5: case 0x1EA7: case 0x1EA9: case 0x1EAB: case 0x1EAD: return 'a';
    case 0x00C2: case 0x1EA4: case 0x1EA6: case 0x1EA8: case 0x1EAA: case 0x1EAC: return 'A';

    // ===== e / E =====
    case 0x00E9: case 0x00E8: case 0x1EBB: case 0x1EBD: case 0x1EB9: return 'e';
    case 0x00C9: case 0x00C8: case 0x1EBA: case 0x1EBC: case 0x1EB8: return 'E';

    case 0x00EA: case 0x1EBF: case 0x1EC1: case 0x1EC3: case 0x1EC5: case 0x1EC7: return 'e';
    case 0x00CA: case 0x1EBE: case 0x1EC0: case 0x1EC2: case 0x1EC4: case 0x1EC6: return 'E';

    // ===== i / I =====
    case 0x00ED: case 0x00EC: case 0x1EC9: case 0x0129: case 0x1ECB: return 'i';
    case 0x00CD: case 0x00CC: case 0x1EC8: case 0x0128: case 0x1ECA: return 'I';

    // ===== o / O =====
    case 0x00F3: case 0x00F2: case 0x1ECF: case 0x00F5: case 0x1ECD: return 'o';
    case 0x00D3: case 0x00D2: case 0x1ECE: case 0x00D5: case 0x1ECC: return 'O';

    case 0x00F4: case 0x1ED1: case 0x1ED3: case 0x1ED5: case 0x1ED7: case 0x1ED9: return 'o';
    case 0x00D4: case 0x1ED0: case 0x1ED2: case 0x1ED4: case 0x1ED6: case 0x1ED8: return 'O';

    case 0x01A1: case 0x1EDB: case 0x1EDD: case 0x1EDF: case 0x1EE1: case 0x1EE3: return 'o';
    case 0x01A0: case 0x1EDA: case 0x1EDC: case 0x1EDE: case 0x1EE0: case 0x1EE2: return 'O';

    // ===== u / U =====
    case 0x00FA: case 0x00F9: case 0x1EE7: case 0x0169: case 0x1EE5: return 'u';
    case 0x00DA: case 0x00D9: case 0x1EE6: case 0x0168: case 0x1EE4: return 'U';

    case 0x01B0: case 0x1EE9: case 0x1EEB: case 0x1EED: case 0x1EEF: case 0x1EF1: return 'u';
    case 0x01AF: case 0x1EE8: case 0x1EEA: case 0x1EEC: case 0x1EEE: case 0x1EF0: return 'U';

    // ===== y / Y =====
    case 0x00FD: case 0x1EF3: case 0x1EF7: case 0x1EF9: case 0x1EF5: return 'y';
    case 0x00DD: case 0x1EF2: case 0x1EF6: case 0x1EF8: case 0x1EF4: return 'Y';

    // ===== Ký tự đặc biệt =====
    case 0x00B0: return 'o'; // °
    case 0x00BA: return 'o'; // º
    case 0x2103: return 'C'; // ℃
    case 0x2109: return 'F'; // ℉
    case 0x00B5: return 'u'; // µ
    case 0x03BC: return 'u'; // μ
    case 0x00D7: return 'x'; // ×
    case 0x00F7: return '/'; // ÷
    case 0x2192: return '>'; // →
    case 0x2190: return '<'; // ←
    case 0x2013: case 0x2014: return '-'; // – —
    case 0x201C: case 0x201D: return '"'; // " "
    case 0x2018: case 0x2019: return '\''; // ' '
    case 0x2026: return '.'; // …

    default:
      return '?';
  }
}

// Loại bỏ dấu tiếng Việt nếu cấu hình yêu cầu
String OledDisplay::stripVietnamese(const String& in) {
#if STRIP_VIETNAMESE_FOR_OLED
  String out;
  out.reserve(in.length());
  
  for (size_t i = 0; i < in.length(); i++) {
    unsigned char c = (unsigned char)in[i];
    
    if (c < 128) {
      out += (char)c;
      continue;
    }
    
    uint32_t codepoint = 0;
    int byteCount = 0;
    
    if ((c & 0xE0) == 0xC0) {
      byteCount = 1;
      codepoint = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
      byteCount = 2;
      codepoint = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
      byteCount = 3;
      codepoint = c & 0x07;
    } else {
      continue;
    }
    
    bool ok = true;
    for (int j = 0; j < byteCount; j++) {
      if (i + 1 >= in.length()) {
        ok = false;
        break;
      }
      i++;
      unsigned char next = (unsigned char)in[i];
      if ((next & 0xC0) != 0x80) {
        ok = false;
        break;
      }
      codepoint = (codepoint << 6) | (next & 0x3F);
    }

    if (!ok) {
      out += '?';
      continue;
    }
    
    char ascii = vietneseToAscii(codepoint);
    out += ascii;
  }
  
  return out;
#else
  return in;
#endif
}