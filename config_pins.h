#pragma once
#include <PINS_JC4827W543.h>

// I²S → PCM5102A (DAC externe)
#define I2S_PCM_BCLK  I2S_BCLK
#define I2S_PCM_LRCK  I2S_LRCK
#define I2S_PCM_DOUT  I2S_DOUT

// I²S → HP board (mêmes lignes, lib AudioI2S)
#define I2S_SPK_BCLK  I2S_BCLK
#define I2S_SPK_LRCK  I2S_LRCK
#define I2S_SPK_DOUT  I2S_DOUT
