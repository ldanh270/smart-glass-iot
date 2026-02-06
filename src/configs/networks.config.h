// Socket (WebSocket)
// Format: ws://<ip>:<port>/<path>
static const char* WS_SERVER_URL = "ws://192.168.1.10:3000/ws";

// Thời gian reconnect WebSocket (milliseconds)
static const unsigned long WS_RECONNECT_INTERVAL_MS = 5000;

// WiFi - Thay "your_ssid" và "your_pass" bằng tên mạng và password của bạn
static const char* WIFI_SSID = "T618";     // Tên WiFi (SSID)
static const char* WIFI_PASS = "12345678";   // Mật khẩu WiFi

// Thời gian chờ kết nối WiFi (mili giây)
static const unsigned long WIFI_TIMEOUT_MS = 30000; 