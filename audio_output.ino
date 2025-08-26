#include <Arduino.h>
#include "driver/i2s.h"

static AudioOut g_audioOut = AUDIO_JACK;
static uint8_t  g_volJack  = 80;  // 0..100 (WM8731)
static uint8_t  g_volSpk   = 80;  // 0..100 (gain logiciel)

extern void wm8731Write(uint8_t reg, uint16_t val);

#ifdef SPEAKER_ENABLE_PIN
static void speaker_enable(bool on){ pinMode(SPEAKER_ENABLE_PIN,OUTPUT); digitalWrite(SPEAKER_ENABLE_PIN,on?HIGH:LOW); }
#else
static void speaker_enable(bool){ }
#endif

static void wm8731_setHpVol(uint8_t percent){
  if (percent>100) percent=100;
  uint16_t v = map(percent, 0,100, 0x10,0x7F);
  wm8731Write(0x02, v);
  wm8731Write(0x03, v);
}

void AudioOut_begin(){
  wm8731_setHpVol(g_volJack);
  speaker_enable(false);
}
void AudioOut_setRoute(AudioOut r){
  g_audioOut = r;
  if (r==AUDIO_JACK){ speaker_enable(false); }
  else { speaker_enable(true); wm8731_setHpVol(0); }
}
AudioOut AudioOut_getRoute(){ return g_audioOut; }

void AudioOut_setVolume(uint8_t percent){
  if (percent>100) percent=100;
  if (g_audioOut==AUDIO_JACK){ g_volJack=percent; wm8731_setHpVol(g_volJack); }
  else { g_volSpk=percent; }
}

extern "C" esp_err_t i2s_write(i2s_port_t, const void*, size_t, size_t*, TickType_t);
void AudioOut_writeStereo(int16_t* interleaved, size_t frames){
  if (g_audioOut==AUDIO_SPK){
    static int16_t work[1024*2];
    size_t left=frames, off=0;
    float gain = (float)g_volSpk/100.0f;
    while (left){
      size_t n = left>1024?1024:left;
      for (size_t i=0;i<n*2;i++){
        int32_t s = (int32_t)(interleaved[off*2+i]*gain);
        if (s<-32768) s=-32768; else if (s>32767) s=32767;
        work[i]=(int16_t)s;
      }
      size_t wb=0; i2s_write(I2S_NUM_0,(const char*)work,n*4,&wb,portMAX_DELAY);
      off+=n; left-=n;
    }
  } else {
    size_t wb=0; i2s_write(I2S_NUM_0,(const char*)interleaved,frames*4,&wb,portMAX_DELAY);
  }
}
