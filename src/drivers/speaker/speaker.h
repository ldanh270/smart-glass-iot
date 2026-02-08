#ifndef SPEAKER_H
#define SPEAKER_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "../../configs/drivers.config.h"

// Fallback nếu config chưa có (nhưng bạn đã có đúng pin rồi)
#ifndef SPK_I2S_BCLK
#define SPK_I2S_BCLK 38
#endif

#ifndef SPK_I2S_WS
#define SPK_I2S_WS 39
#endif

#ifndef SPK_I2S_DIN
#define SPK_I2S_DIN 40
#endif

class SpeakerMax98357A {
public:
    // init(): cấu hình chân I2S + cài driver I2S TX
    bool init(uint32_t sampleRate = 16000);

    // start(): sẵn sàng phát (clear DMA)
    bool start();

    // stop(): dừng phát khi chuyển mode khác (uninstall I2S)
    void stop();

    bool isInited() const { return _inited; }
    bool isRunning() const { return _running; }

    // play(): phát PCM16 mono -> tự nhân đôi thành stereo cho MAX98357A
    // return: số bytes đã ghi
    size_t playPCM16Mono(const int16_t* pcm, size_t nSamples);

private:
    bool installI2S();
    void uninstallI2S();

private:
    bool _inited  = false;
    bool _running = false;

    uint32_t _sampleRate = 16000;
};

#endif
