#ifndef MIC_H
#define MIC_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "../../configs/drivers.config.h"

class MicInmp441 {
public:
    // init(): khai báo/cấu hình chân kết nối + cài I2S driver
    // sampleRate: khuyến nghị 16000 cho voice
    // frameSamples: số sample mỗi frame (ảnh hưởng độ trễ & tần suất callback)
    bool init(uint32_t sampleRate = SAMPLE_RATE, size_t frameSamples = FRAME_SAMPLES);

    // start(): bắt đầu thu âm
    bool start();

    // stop(): dừng thu âm (khi chuyển mode khác)
    void stop();

    // Gọi trong loop(): đọc 1 frame -> tính RMS/Peak (luôn cập nhật lastRms/lastPeak)
    // Nếu RMS vượt ngưỡng và có callback -> gọi callback ("gửi")
    void process();

    // Cấu hình "nếu có tín hiệu thì gửi"
    // thresholdRms: ngưỡng RMS (tùy môi trường). Mặc định ~300 là mức vừa.
    void setThresholdRms(float thresholdRms);
    float getThresholdRms() const;

    // Callback khi có tín hiệu
    // data: PCM16 mono, n: số sample
    typedef void (*OnAudioFrame)(const int16_t* data, size_t n);
    void setOnAudioFrame(OnAudioFrame cb);

    // Debug info
    int16_t lastPeak() const;
    float lastRms() const;

    bool isInited() const;
    bool isRunning() const;

private:
    bool installI2S();
    void uninstallI2S();
    static int16_t convert32To16(int32_t s32);

private:
    bool initialized = false;
    bool _inited = false;
    bool _running = false;

    uint32_t _sampleRate = 16000;
    size_t   _frameSamples = 256;

    float   _thresholdRms = 300.0f;
    int16_t _lastPeak = 0;
    float   _lastRms = 0.0f;

    OnAudioFrame _cb = nullptr;

    int32_t* _raw32 = nullptr;
    int16_t* _pcm16 = nullptr;
};

#endif
