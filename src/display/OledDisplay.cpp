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

// Cắt chuỗi an toàn (tránh text dài quá gây tràn hiển thị hoặc tốn RAM.)
String OledDisplay::safeTruncate(const String& s, size_t maxLen) {
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen);
}

// Chuyển Unicode ký tự Việt thành ASCII không dấu (chuẩn tiếng Việt, đủ hoa/thường)
char OledDisplay::vietneseToAscii(uint16_t u) {
  // ASCII thì giữ nguyên
  if (u < 128) return (char)u;

  switch (u) {
    // ===== d / D =====
    case 0x0111: return 'd'; // đ
    case 0x0110: return 'D'; // Đ

    // ===== a / A =====
    // a + dấu thanh
    case 0x00E1: case 0x00E0: case 0x1EA3: case 0x00E3: case 0x1EA1: return 'a'; // á à ả ã ạ
    case 0x00C1: case 0x00C0: case 0x1EA2: case 0x00C3: case 0x1EA0: return 'A'; // Á À Ả Ã Ạ

    // ă + dấu thanh
    case 0x0103: case 0x1EAF: case 0x1EB1: case 0x1EB3: case 0x1EB5: case 0x1EB7: return 'a'; // ă ắ ằ ẳ ẵ ặ
    case 0x0102: case 0x1EAE: case 0x1EB0: case 0x1EB2: case 0x1EB4: case 0x1EB6: return 'A'; // Ă Ắ Ằ Ẳ Ẵ Ặ

    // â + dấu thanh
    case 0x00E2: case 0x1EA5: case 0x1EA7: case 0x1EA9: case 0x1EAB: case 0x1EAD: return 'a'; // â ấ ầ ẩ ẫ ậ
    case 0x00C2: case 0x1EA4: case 0x1EA6: case 0x1EA8: case 0x1EAA: case 0x1EAC: return 'A'; // Â Ấ Ầ Ẩ Ẫ Ậ

    // ===== e / E =====
    // e + dấu thanh
    case 0x00E9: case 0x00E8: case 0x1EBB: case 0x1EBD: case 0x1EB9: return 'e'; // é è ẻ ẽ ẹ
    case 0x00C9: case 0x00C8: case 0x1EBA: case 0x1EBC: case 0x1EB8: return 'E'; // É È Ẻ Ẽ Ẹ

    // ê + dấu thanh
    case 0x00EA: case 0x1EBF: case 0x1EC1: case 0x1EC3: case 0x1EC5: case 0x1EC7: return 'e'; // ê ế ề ể ễ ệ
    case 0x00CA: case 0x1EBE: case 0x1EC0: case 0x1EC2: case 0x1EC4: case 0x1EC6: return 'E'; // Ê Ế Ề Ể Ễ Ệ

    // ===== i / I =====
    case 0x00ED: case 0x00EC: case 0x1EC9: case 0x0129: case 0x1ECB: return 'i'; // í ì ỉ ĩ ị
    case 0x00CD: case 0x00CC: case 0x1EC8: case 0x0128: case 0x1ECA: return 'I'; // Í Ì Ỉ Ĩ Ị

    // ===== o / O =====
    // o + dấu thanh
    case 0x00F3: case 0x00F2: case 0x1ECF: case 0x00F5: case 0x1ECD: return 'o'; // ó ò ỏ õ ọ
    case 0x00D3: case 0x00D2: case 0x1ECE: case 0x00D5: case 0x1ECC: return 'O'; // Ó Ò Ỏ Õ Ọ

    // ô + dấu thanh
    case 0x00F4: case 0x1ED1: case 0x1ED3: case 0x1ED5: case 0x1ED7: case 0x1ED9: return 'o'; // ô ố ồ ổ ỗ ộ
    case 0x00D4: case 0x1ED0: case 0x1ED2: case 0x1ED4: case 0x1ED6: case 0x1ED8: return 'O'; // Ô Ố Ồ Ổ Ỗ Ộ

    // ơ + dấu thanh
    case 0x01A1: case 0x1EDB: case 0x1EDD: case 0x1EDF: case 0x1EE1: case 0x1EE3: return 'o'; // ơ ớ ờ ở ỡ ợ
    case 0x01A0: case 0x1EDA: case 0x1EDC: case 0x1EDE: case 0x1EE0: case 0x1EE2: return 'O'; // Ơ Ớ Ờ Ở Ỡ Ợ

    // ===== u / U =====
    // u + dấu thanh
    case 0x00FA: case 0x00F9: case 0x1EE7: case 0x0169: case 0x1EE5: return 'u'; // ú ù ủ ũ ụ
    case 0x00DA: case 0x00D9: case 0x1EE6: case 0x0168: case 0x1EE4: return 'U'; // Ú Ù Ủ Ũ Ụ

    // ư + dấu thanh
    case 0x01B0: case 0x1EE9: case 0x1EEB: case 0x1EED: case 0x1EEF: case 0x1EF1: return 'u'; // ư ứ ừ ử ữ ự
    case 0x01AF: case 0x1EE8: case 0x1EEA: case 0x1EEC: case 0x1EEE: case 0x1EF0: return 'U'; // Ư Ứ Ừ Ử Ữ Ự

    // ===== y / Y =====
    case 0x00FD: case 0x1EF3: case 0x1EF7: case 0x1EF9: case 0x1EF5: return 'y'; // ý ỳ ỷ ỹ ỵ
    case 0x00DD: case 0x1EF2: case 0x1EF6: case 0x1EF8: case 0x1EF4: return 'Y'; // Ý Ỳ Ỷ Ỹ Ỵ

    // ===== Ký tự hay gặp trong đo đạc / hiển thị =====
    case 0x00B0: return 'o'; // ° degree sign -> 'o' (VD: 30oC)

    case 0x00BA: return 'o'; // º masculine ordinal -> thường giống '°' (một số text copy/paste)
    case 0x2103: return 'C'; // ℃ (U+2103) -> 'C'
    case 0x2109: return 'F'; // ℉ (U+2109) -> 'F'

    case 0x00B5: return 'u'; // µ (micro) -> 'u' (VD: µA -> uA)
    case 0x03BC: return 'u'; // μ (Greek mu) -> 'u' (cũng hay gặp)

    case 0x00D7: return 'x'; // × multiply -> 'x'
    case 0x00F7: return '/'; // ÷ divide -> '/'

    case 0x2192: return '>'; // → -> '>'
    case 0x2190: return '<'; // ← -> '<'

    case 0x2013: // – en dash
    case 0x2014: // — em dash
      return '-';

    // “nháy kép / nháy đơn thông minh” -> ký tự ASCII
    case 0x201C: case 0x201D: return '"'; // “ ”
    case 0x2018: case 0x2019: return '\''; // ‘ ’

    // dấu ba chấm …
    case 0x2026: return '.'; //  "..."


    default:
      // Ký tự Unicode khác (emoji các thứ, v.v.) -> thay bằng '?'
      return '?';
  }
}


// Tùy chọn: chuyển ký tự Việt có dấu thành không dấu
// Nếu `STRIP_VIETNAMESE_FOR_OLED` = 1, sẽ thay các ký tự Việt bằng phiên bản không dấu.
String OledDisplay::stripVietnamese(const String& in) {
#if STRIP_VIETNAMESE_FOR_OLED
  String out; out.reserve(in.length());
  
  for (size_t i = 0; i < in.length(); i++) {
    unsigned char c = (unsigned char)in[i];
    
    // Nếu là ASCII, giữ nguyên
    if (c < 128) {
      out += (char)c;
      continue;
    }
    
    // Xử lý UTF-8 đa byte
    // UTF-8: byte đầu tiên có số bit 1 ở đầu chỉ số byte trong chuỗi
    // Ví dụ: 0xC0-0xDF (2 byte), 0xE0-0xEF (3 byte)
    uint32_t codepoint = 0;
    int byteCount = 0;
    
    if ((c & 0xE0) == 0xC0) {
      // 2 byte UTF-8
      byteCount = 1;
      codepoint = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
      // 3 byte UTF-8
      byteCount = 2;
      codepoint = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
      // 4 byte UTF-8
      byteCount = 3;
      codepoint = c & 0x07;
    } else {
      // Byte tiếp theo (continuation byte), bỏ qua
      continue;
    }
    
    // Đọc các byte tiếp theo
bool ok = true;
    for (int j = 0; j < byteCount; j++) {
      if (i + 1 >= in.length()) { ok = false; break; }
      i++;
      unsigned char next = (unsigned char)in[i];
      if ((next & 0xC0) != 0x80) { ok = false; break; }
      codepoint = (codepoint << 6) | (next & 0x3F);
    }

    if (!ok) {
      out += '?';
      continue;
    }
    
    // Chuyển đổi Unicode -> ASCII
    char ascii = vietneseToAscii(codepoint);
    out += ascii;
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
