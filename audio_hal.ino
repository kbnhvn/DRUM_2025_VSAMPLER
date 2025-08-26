#include <Wire.h>
#include "driver/i2s.h"
#include <PINS_JC4827W543.h>

// Pins I2S (board macros) + entrée ADCDAT sur IO14
#define I2S_BCK_PIN       I2S_BCLK
#define I2S_WS_PIN        I2S_LRCK
#define I2S_DATA_OUT_PIN  I2S_DOUT
#ifndef I2S_DATA_IN_PIN
#define I2S_DATA_IN_PIN   14
#endif

#define WM8731_ADDR 0x1A  // MIKROE-506 (CSB=GND)

void wm8731Write(uint8_t reg, uint16_t val) {
  uint16_t data = ((reg & 0x7F) << 9) | (val & 0x1FF);
  Wire.beginTransmission(WM8731_ADDR);
  Wire.write((data >> 8) & 0xFF);
  Wire.write(data & 0xFF);
  Wire.endTransmission();
}

void wm8731Init_44100() {
  wm8731Write(0x0F, 0x00);   // Reset
  wm8731Write(0x0C, 0x00);   // Power up all
  wm8731Write(0x07, 0b000100000); // I2S, 16-bit, slave
  wm8731Write(0x08, 0b000000000); // 44.1kHz
  wm8731Write(0x04, 0b000100010); // DAC, line-in, mic mute
  wm8731Write(0x05, 0b000000000); // HPF on, no de-emph
  wm8731Write(0x02, 0x7F);        // HP vol L
  wm8731Write(0x03, 0x7F);        // HP vol R
  wm8731Write(0x00, 0x17);        // Line in L
  wm8731Write(0x01, 0x17);        // Line in R
  wm8731Write(0x09, 0x01);        // Activate
}

void i2sBeginFullDuplex() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 128,
    .use_apll = true,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_BCK_PIN,
    .ws_io_num    = I2S_WS_PIN,
    .data_out_num = I2S_DATA_OUT_PIN,
    .data_in_num  = I2S_DATA_IN_PIN
  };
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
}
