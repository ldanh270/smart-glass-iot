#include "speaker.h"

#include <stdlib.h>
#include "driver/i2s.h"

static constexpr i2s_port_t SPK_PORT = I2S_NUM_1;   // tránh đụng mic (thường dùng I2S_NUM_0)
static constexpr int SPK_DMA_BUF_CNT = 8;
static constexpr int SPK_DMA_BUF_LEN = 256;         // độ trễ ổn cho voice

bool SpeakerMax98357A::init(uint32_t sampleRate) {
    if (_inited) return true;

    _sampleRate = sampleRate;

    if (!installI2S()) {
        return false;
    }

    _inited = true;
    _running = false;
    return true;
}

bool SpeakerMax98357A::start() {
    if (!_inited) {
        if (!init(_sampleRate)) return false;
    }
    if (_running) return true;

    i2s_zero_dma_buffer(SPK_PORT);
    _running = true;
    return true;
}

void SpeakerMax98357A::stop() {
    if (!_inited) return;

    _running = false;
    uninstallI2S();
    _inited = false;
}

size_t SpeakerMax98357A::playPCM16Mono(const int16_t* pcm, size_t nSamples) {
    if (!_running || !pcm || nSamples == 0) return 0;

    // MAX98357A nhận I2S stereo rất ổn định.
    // Ta nhân đôi mono -> stereo interleaved: L,R,L,R,...
    // Buffer tạm theo chunk để tránh malloc quá lớn.
    const size_t CHUNK_SAMPLES = 256; // mono samples/chunk
    int16_t stereo[CHUNK_SAMPLES * 2];

    size_t totalBytesWritten = 0;

    size_t offset = 0;
    while (offset < nSamples) {
        size_t take = nSamples - offset;
        if (take > CHUNK_SAMPLES) take = CHUNK_SAMPLES;

        // mono -> stereo
        for (size_t i = 0; i < take; i++) {
            int16_t s = pcm[offset + i];
            stereo[i * 2 + 0] = s; // L
            stereo[i * 2 + 1] = s; // R
        }

        size_t bytesToWrite = take * 2 * sizeof(int16_t); // stereo
        size_t bytesWritten = 0;

        esp_err_t err = i2s_write(
            SPK_PORT,
            (const char*)stereo,
            bytesToWrite,
            &bytesWritten,
            portMAX_DELAY
        );

        if (err != ESP_OK) {
            break; // lỗi I2S -> dừng luôn
        }

        totalBytesWritten += bytesWritten;
        offset += take;
    }

    return totalBytesWritten;
}

bool SpeakerMax98357A::installI2S() {
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate = _sampleRate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;

    // Stereo (L/R) để MAX98357A ổn định, dù nguồn là mono ta đã nhân đôi.
    cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;

    // Tránh warning deprecated (đúng như bạn gặp ở mic)
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;

    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = SPK_DMA_BUF_CNT;
    cfg.dma_buf_len = SPK_DMA_BUF_LEN;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = true;
    cfg.fixed_mclk = 0;

    i2s_pin_config_t pins = {};
    pins.bck_io_num   = SPK_I2S_BCLK;
    pins.ws_io_num    = SPK_I2S_WS;
    pins.data_out_num = SPK_I2S_DIN;
    pins.data_in_num  = I2S_PIN_NO_CHANGE;

    esp_err_t err = i2s_driver_install(SPK_PORT, &cfg, 0, nullptr);
    if (err != ESP_OK) return false;

    err = i2s_set_pin(SPK_PORT, &pins);
    if (err != ESP_OK) {
        i2s_driver_uninstall(SPK_PORT);
        return false;
    }

    // set clock stereo 16-bit
    err = i2s_set_clk(SPK_PORT, _sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    if (err != ESP_OK) {
        i2s_driver_uninstall(SPK_PORT);
        return false;
    }

    return true;
}

void SpeakerMax98357A::uninstallI2S() {
    i2s_driver_uninstall(SPK_PORT);
}
