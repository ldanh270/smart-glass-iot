#include "socket.h"
#include "../../configs/networks.config.h"

using namespace websockets;

Socket::Socket() : connected(false), lastReconnectAttempt(0) {
    // Khởi tạo trạng thái Socket
}

bool Socket::connect() {
    return connectInternal();
}

bool Socket::connectInternal() {
    // Thiết lập callback nhận message text
    client.onMessage([this](WebsocketsMessage message) {
        if (message.isText()) {
            onText(message.data().c_str());
        }
        });

    // Thiết lập callback sự kiện kết nối
    client.onEvent([this](WebsocketsEvent event, String data) {
        if (event == WebsocketsEvent::ConnectionOpened) {
            connected = true;
            Serial.println("[Socket] Connected to server");
        }

        if (event == WebsocketsEvent::ConnectionClosed) {
            connected = false;
            Serial.println("[Socket] Disconnected from server");
        }
        });

    // Bắt đầu kết nối tới server
    Serial.printf("[Socket] Connecting to %s\n", WS_SERVER_URL);
    return client.connect(WS_SERVER_URL);
}

void Socket::sendBinary(const uint8_t* data, size_t length) {
    // Chỉ gửi khi đang kết nối
    if (!connected) return;

    client.sendBinary((const char*)data, length);
}

void Socket::onText(const char* payload) {
    // Xử lý plain text nhận từ server
    Serial.printf("[Socket] Received text: %s\n", payload);
}

void Socket::disconnect() {
    // Ngắt kết nối WebSocket
    if (!connected) return;

    client.close();
    connected = false;
}

void Socket::loop() {
    // Cập nhật trạng thái WebSocket
    client.poll();

    // Tự reconnect nếu mất kết nối
    if (!connected) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= WS_RECONNECT_INTERVAL_MS) {
            lastReconnectAttempt = now;
            Serial.println("[Socket] Reconnecting...");
            connectInternal();
        }
    }
}

bool Socket::isConnected() {
    // Trả true nếu đang kết nối server
    return connected;
}
