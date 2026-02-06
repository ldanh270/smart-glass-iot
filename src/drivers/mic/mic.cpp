#include "mic.h"

#include <math.h>
#include <stdlib.h>
#include "driver/i2s.h"

static constexpr i2s_port_t MIC_PORT = I2S_NUM_0;
static constexpr int MIC_DMA_BUF_CNT = 8;

bool MicInmp441::init(uint32_t sampleRate, size_t frameSamples) {
    if (_inited) return true;

    _sampleRate = sampleRate;
    _frameSamples = frameSamples;

    // alloc buffers
    _raw32 = (int32_t*)malloc(sizeof(int32_t) * _frameSamples);
    _pcm16 = (int16_t*)malloc(sizeof(int16_t) * _frameSamples);
    if (!_raw32 || !_pcm16) {
        Serial.println("[MIC] malloc failed");
        if (_raw32) { free(_raw32); _raw32 = nullptr; }
        if (_pcm16) { free(_pcm16); _pcm16 = nullptr; }
        return false;
    }

    if (!installI2S()) {
        if (_raw32) { free(_raw32); _raw32 = nullptr; }
        if (_pcm16) { free(_pcm16); _pcm16 = nullptr; }
        return false;
    }

    _inited = true;
    _running = false;

    Serial.printf("[MIC] init OK (BCLK=%d WS=%d SD=%d, SR=%lu)\n",
                  MIC_I2S_BCLK, MIC_I2S_WS, MIC_I2S_SD, (unsigned long)_sampleRate);
    return true;
}

bool MicInmp441::start() {
    if (!_inited) {
        if (!init(_sampleRate, _frameSamples)) return false;
    }
    if (_running) return true;

    i2s_zero_dma_buffer(MIC_PORT);
    _running = true;
    Serial.println("[MIC] start OK");
    return true;
}

void MicInmp441::stop() {
    if (!_inited) return;

    _running = false;
    uninstallI2S();
    _inited = false;

    if (_raw32) { free(_raw32); _raw32 = nullptr; }
    if (_pcm16) { free(_pcm16); _pcm16 = nullptr; }

    Serial.println("[MIC] stopped");
}

void MicInmp441::process() {
    if (!_running) return;

    size_t bytesRead = 0;
    esp_err_t err = i2s_read(
        MIC_PORT,
        _raw32,
        sizeof(int32_t) * _frameSamples,
        &bytesRead,
        portMAX_DELAY
    );

    if (err != ESP_OK || bytesRead == 0) {
        Serial.printf("[MIC] i2s_read err=%d bytes=%u\n", (int)err, (unsigned)bytesRead);
        return;
    }

    const size_t n = bytesRead / sizeof(int32_t);

    int32_t peak = 0;
    int64_t sumSq = 0;

    for (size_t i = 0; i < n; i++) {
        int16_t s16 = convert32To16(_raw32[i]);
        _pcm16[i] = s16;

        int32_t a = abs((int32_t)s16);
        if (a > peak) peak = a;

        sumSq += (int64_t)s16 * (int64_t)s16;
    }

    _lastPeak = (int16_t)peak;
    _lastRms  = sqrtf((float)sumSq / (float)n);

    // "nếu có tín hiệu thì gửi"
    if (_cb && _lastRms >= _thresholdRms) {
        _cb(_pcm16, n);
    }
}

void MicInmp441::setThresholdRms(float thresholdRms) {
    _thresholdRms = thresholdRms;
}

float MicInmp441::getThresholdRms() const {
    return _thresholdRms;
}

void MicInmp441::setOnAudioFrame(OnAudioFrame cb) {
    _cb = cb;
}

int16_t MicInmp441::lastPeak() const {
    return _lastPeak;
}

float MicInmp441::lastRms() const {
    return _lastRms;
}

bool MicInmp441::isInited() const {
    return _inited;
}

bool MicInmp441::isRunning() const {
    return _running;
}

bool MicInmp441::installI2S() {
    // INMP441:
    // - L/R nối GND => LEFT
    // - dùng 32-bit slot (mic ~24-bit nằm trong 32-bit word trên ESP32)
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate = _sampleRate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = MIC_DMA_BUF_CNT;
    cfg.dma_buf_len = (int)_frameSamples;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = false;
    cfg.fixed_mclk = 0;

    i2s_pin_config_t pins = {};
    pins.bck_io_num = MIC_I2S_BCLK;
    pins.ws_io_num  = MIC_I2S_WS;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    pins.data_in_num  = MIC_I2S_SD;

    esp_err_t err = i2s_driver_install(MIC_PORT, &cfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("[MIC] i2s_driver_install failed: %d\n", (int)err);
        return false;
    }

    err = i2s_set_pin(MIC_PORT, &pins);
    if (err != ESP_OK) {
        Serial.printf("[MIC] i2s_set_pin failed: %d\n", (int)err);
        i2s_driver_uninstall(MIC_PORT);
        return false;
    }

    // reinforce mono clock settings
    err = i2s_set_clk(MIC_PORT, _sampleRate, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_MONO);
    if (err != ESP_OK) {
        Serial.printf("[MIC] i2s_set_clk failed: %d\n", (int)err);
        i2s_driver_uninstall(MIC_PORT);
        return false;
    }

    return true;
}

void MicInmp441::uninstallI2S() {
    i2s_driver_uninstall(MIC_PORT);
}

// Shift để đưa 32-bit word về 16-bit usable.
// Nếu bạn thấy level nhỏ quá -> đổi >>14 thành >>13
// Nếu bão hòa to quá -> >>15 hoặc >>16
int16_t MicInmp441::convert32To16(int32_t s32) {
    return (int16_t)(s32 >> 14);
}
