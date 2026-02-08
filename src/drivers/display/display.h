#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include "../../configs/drivers.config.h"

class OledDisplay {
public:
    // Khởi tạo OLED (I2C + driver). Trả về true nếu thành công
    bool init();

    // Xóa tất cả text đang hiển thị
    void clear();

    // Hiển thị text tại tọa độ (x, y) trên màn hình
    void print(uint16_t x, uint16_t y, const String& text);

    // In text với auto-wrap từ tọa độ (x, y), chia thành nhiều dòng nếu cần
    void printWrapped(uint16_t x, uint16_t y, const String& text);

    // Cập nhật màn hình để hiển thị các thay đổi
    void show();

private:
    // Cắt chuỗi an toàn nếu quá dài để tránh xử lý nặng
    String safeTruncate(const String& s, size_t maxLen);

    // Nếu cấu hình yêu cầu, thay các ký tự Việt bằng phiên bản không dấu
    String stripVietnamese(const String& in);

    // Chuyển Unicode ký tự Việt thành ASCII không dấu
    char vietneseToAscii(uint16_t unicode);
};

#endif