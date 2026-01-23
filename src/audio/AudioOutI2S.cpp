#include "audio/AudioOutI2S.h"
#include "config.h"

#include "driver/i2s.h"

// Khởi tạo driver I2S cho output (ví dụ MAX98357A)
bool AudioOutI2S::begin() {
  // MAX98357A dùng chuẩn I2S: cần BCLK + LRCLK + DIN
  //BCLK: bit clock 
  //LRCLK: left-right clock (word select)
  //DIN: data in
  i2s_config_t i2s_config = {};
  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2s_config.sample_rate = AUDIO_SAMPLE_RATE;
  i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT; // mono -> dùng channel left để gửi dữ liệu 
  i2s_config.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S); 
  i2s_config.intr_alloc_flags = 0;
  i2s_config.dma_buf_count = 8;   // Tăng nếu bị giật
  i2s_config.dma_buf_len = 256;   // Kích thước buffer DMA
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = true;
  i2s_config.fixed_mclk = 0;

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) return false;

  // Cấu hình chân I2S theo `config.h`
  i2s_pin_config_t pin_config = {};
  pin_config.bck_io_num = I2S_BCLK;
  pin_config.ws_io_num  = I2S_LRCLK;
  pin_config.data_out_num = I2S_DOUT;
  pin_config.data_in_num  = I2S_PIN_NO_CHANGE;

  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) return false;

  // Thiết lập clock mẫu, bits, và số channel (mono)
  err = i2s_set_clk(I2S_NUM_0, AUDIO_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  return (err == ESP_OK);
}

// Dừng phát hiện tại: xóa buffer DMA để không còn dữ liệu treo
void AudioOutI2S::stop() {
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// Ghi buffer PCM tới I2S; blocking cho đến khi dữ liệu được push vào DMA
bool AudioOutI2S::write(const uint8_t* data, size_t len) {
  size_t bytesWritten = 0;
  esp_err_t err = i2s_write(I2S_NUM_0, data, len, &bytesWritten, portMAX_DELAY);
  return (err == ESP_OK);
}
