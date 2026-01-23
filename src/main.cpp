#include <Arduino.h>

#include "display/OledDisplay.h"
#include "audio/AudioOutI2S.h"
#include "net/WiFiManager.h"
#include "net/WsClient.h"
#include "tts/TtsClient.h"

static OledDisplay oled;
static AudioOutI2S audio;
static WiFiManager wifi;
static WsClient wsClient;
static TtsClient tts;

void setup() {
  Serial.begin(115200);

  if (!oled.begin()) {
    while (true) { delay(1000); } // OLED lỗi cứng
  }

  if (!audio.begin()) {
    oled.printWrapped("I2S init FAIL");
    while (true) { delay(1000); } // I2S lỗi cứng
  }

  oled.printWrapped("WiFi connecting...");
  wifi.connectBlocking();
  oled.printWrapped("WiFi OK");

  wsClient.begin(oled, tts, audio);
}

void loop() {
  wsClient.poll();

  // nếu WS rớt, tự reconnect (có giới hạn)
  wsClient.ensureConnected();

  // WiFi rớt thì nối lại (đơn giản)
  if (!wifi.isConnected()) {
    oled.printWrapped("WiFi reconnect...");
    wifi.connectBlocking();
    oled.printWrapped("WiFi OK");
  }
}