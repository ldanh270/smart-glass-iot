#include "audio/AudioOutI2S.h"
#include "config.h"

#include "driver/i2s.h"

bool AudioOutI2S::begin() {
  // LƯU Ý: MAX98357A kỳ vọng chuẩn I2S, dữ liệu trên DIN, cần BCLK + LRCLK + DIN.
  i2s_config_t i2s_config = {};
  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2s_config.sample_rate = AUDIO_SAMPLE_RATE;
  i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT; // mono -> trái
  i2s_config.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S); // I2S chuẩn
  i2s_config.intr_alloc_flags = 0;
  i2s_config.dma_buf_count = 8;        // LƯU Ý: tăng nếu âm thanh bị giật
  i2s_config.dma_buf_len = 256;        // LƯU Ý: kích thước khối; điều chỉnh để ổn định
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = true;
  i2s_config.fixed_mclk = 0;

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) return false;

  i2s_pin_config_t pin_config = {};
  pin_config.bck_io_num = I2S_BCLK;
  pin_config.ws_io_num  = I2S_LRCLK;
  pin_config.data_out_num = I2S_DOUT;
  pin_config.data_in_num  = I2S_PIN_NO_CHANGE;

  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) return false;

  err = i2s_set_clk(I2S_NUM_0, AUDIO_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  return (err == ESP_OK);
}

void AudioOutI2S::stop() {
  // LƯU Ý: xả DMA để dừng phát hiện tại sạch sẽ
  i2s_zero_dma_buffer(I2S_NUM_0);
}

bool AudioOutI2S::write(const uint8_t* data, size_t len) {
  size_t bytesWritten = 0;
  esp_err_t err = i2s_write(I2S_NUM_0, data, len, &bytesWritten, portMAX_DELAY);
  return (err == ESP_OK);
}
