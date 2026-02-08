#ifndef SOCKET_H
#define SOCKET_H

#include <Arduino.h>
#include <ArduinoWebsockets.h>

/**
 * `Socket`: wrapper quản lý kết nối WebSocket cho mạch IoT.
 * `connect()` kết nối tới server WebSocket (dùng config).
 * `sendBinary(data, length)` gửi dữ liệu nhị phân tới server.
 * `onText(payload)` xử lý tin nhắn văn bản nhận từ server.
 * `disconnect()` ngắt kết nối khỏi server.
 * `loop()` cập nhật trạng thái kết nối (gọi trong main loop) và tự reconnect nếu cần.
 */

class Socket {
private:
    websockets::WebsocketsClient client;
    bool connected;
    unsigned long lastReconnectAttempt;

    // Thực hiện kết nối WebSocket nội bộ
    bool connectInternal();

public:
    // Khởi tạo Socket
    Socket();

    // Kết nối tới server WebSocket (dùng WS_SERVER_URL)
    bool connect();

    // Gửi dữ liệu nhị phân tới server
    void sendBinary(const uint8_t* data, size_t length);

    // Xử lý tin nhắn văn bản nhận từ server (override nếu cần)
    virtual void onText(const char* payload);

    // Ngắt kết nối khỏi server
    void disconnect();

    // Cập nhật trạng thái WebSocket (gọi trong main loop)
    void loop();

    // Kiểm tra trạng thái kết nối
    bool isConnected();
};

#endif