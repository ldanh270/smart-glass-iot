#pragma once
#include <Arduino.h>

// `OledDisplay`: lớp tiện ích để in chuỗi lên màn OLED với tính năng bọc dòng
// và (tuỳ chọn) loại bỏ dấu tiếng Việt để tránh lỗi hiển thị trên OLED nhỏ.
class OledDisplay {
public:
  // Khởi tạo OLED (I2C + driver). Trả về true nếu thành công.
  bool begin();

  // In chuỗi với wrap (chia nhiều dòng), dùng font mặc định của Adafruit
  void printWrapped(const String& text);

private:
  // Cắt chuỗi an toàn nếu quá dài để tránh xử lý nặng
  String safeTruncate(const String& s, size_t maxLen);

  // Nếu cấu hình yêu cầu, thay các ký tự UTF-8 (dấu) bằng '?' để tránh lỗi hiển thị
  String stripVietnamese(const String& in);

    // Chuyển Unicode ký tự Việt thành ASCII không dấu
  char vietneseToAscii(uint16_t unicode);
};
