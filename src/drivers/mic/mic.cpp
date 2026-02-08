#include "mic.h"
#include <math.h>
#include <stdlib.h>
#include "driver/i2s.h"

// Cấu hình cổng I2S
static constexpr i2s_port_t MIC_PORT = I2S_NUM_0; // Nên dùng NUM_0 cho ổn định, NUM_1 cũng được
static constexpr int MIC_DMA_BUF_CNT = 8;

bool MicInmp441::init(uint32_t sampleRate, size_t frameSamples) {
    if (_inited) return true;

    _sampleRate = sampleRate;
    _frameSamples = frameSamples;

    // Cấp phát bộ nhớ
    _raw32 = (int32_t*)malloc(sizeof(int32_t) * _frameSamples);
    _pcm16 = (int16_t*)malloc(sizeof(int16_t) * _frameSamples);

    // Kiểm tra cấp phát thành công
    if (!_raw32 || !_pcm16) {
        Serial.println("[MIC] malloc failed");
        if (_raw32) { free(_raw32); _raw32 = nullptr; }
        if (_pcm16) { free(_pcm16); _pcm16 = nullptr; }
        return false;
    }

    if (!installI2S()) {
        // Nếu cài driver lỗi thì giải phóng bộ nhớ ngay
        if (_raw32) { free(_raw32); _raw32 = nullptr; }
        if (_pcm16) { free(_pcm16); _pcm16 = nullptr; }
        return false;
    }

    _inited = true;
    _running = false;

    Serial.printf("[MIC] init OK (SR=%lu, Frame=%u)\n",
        (unsigned long)_sampleRate, (unsigned)_frameSamples);
    return true;
}

bool MicInmp441::start() {
    if (!_inited) {
        if (!init(_sampleRate, _frameSamples)) return false;
    }
    if (_running) return true;

    // Xóa buffer cũ để tránh tiếng nổ "bụp" khi bắt đầu
    i2s_zero_dma_buffer(MIC_PORT);

    // Bắt đầu I2S
    i2s_start(MIC_PORT);

    _running = true;
    Serial.println("[MIC] start OK");
    return true;
}

void MicInmp441::stop() {
    if (!_inited) return;

    _running = false;

    // Dừng I2S trước khi gỡ
    i2s_stop(MIC_PORT);
    uninstallI2S();

    _inited = false;

    if (_raw32) { free(_raw32); _raw32 = nullptr; }
    if (_pcm16) { free(_pcm16); _pcm16 = nullptr; }

    Serial.println("[MIC] stopped");
}

void MicInmp441::process() {
    if (!_running) return;

    size_t bytesRead = 0;
    // Đọc dữ liệu từ I2S (Blocking mode với portMAX_DELAY)
    esp_err_t err = i2s_read(
        MIC_PORT,
        _raw32,
        sizeof(int32_t) * _frameSamples,
        &bytesRead,
        portMAX_DELAY // Chờ đến khi đủ dữ liệu
    );

    if (err != ESP_OK) {
        Serial.printf("[MIC] Read Error: %d\n", err);
        return;
    }

    // Nếu không đọc được byte nào
    if (bytesRead == 0) return;

    const size_t n = bytesRead / sizeof(int32_t);

    int32_t peak = 0;
    int64_t sumSq = 0;

    for (size_t i = 0; i < n; i++) {
        // Convert 32bit -> 16bit
        int16_t s16 = convert32To16(_raw32[i]);
        _pcm16[i] = s16;

        // Tính Peak
        int32_t a = abs((int32_t)s16);
        if (a > peak) peak = a;

        // Tính tổng bình phương cho RMS
        sumSq += (int64_t)s16 * (int64_t)s16;
    }

    _lastPeak = (int16_t)peak;
    // RMS = Sqrt(Trung bình cộng bình phương)
    _lastRms = sqrtf((float)sumSq / (float)n);

    // Callback nếu có hàm đăng ký và vượt ngưỡng
    if (_cb && _lastRms >= _thresholdRms) {
        _cb(_pcm16, n);
    }
}

// ... Các hàm getter/setter giữ nguyên ...
void MicInmp441::setThresholdRms(float thresholdRms) { _thresholdRms = thresholdRms; }
float MicInmp441::getThresholdRms() const { return _thresholdRms; }
void MicInmp441::setOnAudioFrame(OnAudioFrame cb) { _cb = cb; }
int16_t MicInmp441::lastPeak() const { return _lastPeak; }
float MicInmp441::lastRms() const { return _lastRms; }
bool MicInmp441::isInited() const { return _inited; }
bool MicInmp441::isRunning() const { return _running; }

bool MicInmp441::installI2S() {
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = _sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // Mic 24bit cần đọc ở mode 32bit
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // L/R nối GND -> Kênh trái
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = MIC_DMA_BUF_CNT,
        .dma_buf_len = (int)_frameSamples,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pins = {
        .bck_io_num = MIC_I2S_BCLK,
        .ws_io_num = MIC_I2S_WS,
        .data_out_num = -1, // Không dùng chân phát (Loa)
        .data_in_num = MIC_I2S_SD
    };

    esp_err_t err = i2s_driver_install(MIC_PORT, &cfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("[MIC] Install Failed: %d\n", err);
        return false;
    }

    err = i2s_set_pin(MIC_PORT, &pins);
    if (err != ESP_OK) {
        Serial.printf("[MIC] Pin Config Failed: %d\n", err);
        i2s_driver_uninstall(MIC_PORT);
        return false;
    }

    // ĐÃ XÓA: i2s_set_clk(...) -> Không cần thiết và dễ gây lỗi xung đột

    return true;
}

void MicInmp441::uninstallI2S() {
    i2s_driver_uninstall(MIC_PORT);
}

int16_t MicInmp441::convert32To16(int32_t s32) {
    // INMP441 output 24-bit data in a 32-bit slot.
    // Dữ liệu thực tế nằm ở 24 bit cao (MSB aligned).
    // >> 16 là chuẩn.
    // >> 14 là Gain x4 (Loudness boost). Cẩn thận clipping!
    return (int16_t)(s32 >> 14);
}