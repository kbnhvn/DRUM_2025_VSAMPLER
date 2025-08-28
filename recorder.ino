// recorder.ino
// Enregistrement I2S minimal + édition WAV (trim, speed, pitch par resampling) + waveform peaks
// Hypothèses : WAV PCM 16-bit, 1 ou 2 canaux. Stockage sur SD. Fichier de travail: /samples/rec_tmp.wav

#include <Arduino.h>
#include <SD.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

// WAV metadata structure
struct WavMeta {
  uint16_t channels;
  uint32_t sampleRate;
  uint16_t bitsPerSample;
  uint32_t dataOffset;
  uint32_t dataBytes;
  
  // Constructor with default values
  WavMeta() : channels(1), sampleRate(44100), bitsPerSample(16), dataOffset(0), dataBytes(0) {}
};

static bool wav_read_header(File &f, WavMeta &m);
static void wav_write_header(File &out, const WavMeta &m);

// ==== externs ring buffer (wrappers) / writer ====
extern size_t audio_rb_write(const int16_t* data, size_t n);
extern size_t audio_rb_avail_to_read(void);
extern volatile bool g_sdwr_open;   // writer SD détient un fichier ouvert

// ---------- CONFIG ----------
static const char* kRecTmpPath = "/samples/rec_tmp.wav";
static const uint16_t WF_MAX_POINTS = 480;     // nb max de points de peak pour l'UI (≈ largeur écran)
static const size_t   IO_CHUNK_FRAMES = 1024;  // taille de bloc pour IO (frames)

// ---------- ÉTAT ENREGISTREMENT ----------
static bool     s_recActive = false;
static uint32_t s_recBytes  = 0;
static uint32_t s_recStartMs= 0;
static uint32_t s_recDurMs  = 0;
static File     s_recFile;

// ---------- ÉTAT ÉDITION ----------
static uint16_t s_wfPeaks[WF_MAX_POINTS];   // 0..255 (amplitude)
static uint16_t s_wfCount = 0;
static uint32_t s_totalFrames = 0;
static uint32_t s_sampleRate  = 44100;
static uint8_t  s_channels    = 2;

static uint32_t s_editStartFrame = 0;
static uint32_t s_editStopFrame  = 0;       // exclusif
static float    s_editSpeed      = 1.0f;    // 1.0 = normal
static float    s_editPitchSemi  = 0.0f;    // en demi-tons (±12, etc.)

// ===============================================================
// ======================  WAV helpers  ==========================
// ===============================================================

static bool wav_read_header(File &f, WavMeta &m){
  auto rd32=[&](uint32_t &v){ return f.read((uint8_t*)&v,4)==4; };
  auto rd16=[&](uint16_t &v){ return f.read((uint8_t*)&v,2)==2; };

  char tag[4];
  if (f.read((uint8_t*)tag,4)!=4 || strncmp(tag,"RIFF",4)) return false;
  uint32_t riffSize; if(!rd32(riffSize)) return false;
  if (f.read((uint8_t*)tag,4)!=4 || strncmp(tag,"WAVE",4)) return false;

  bool fmtOk=false, dataOk=false;
  while (f.available()){
    if (f.read((uint8_t*)tag,4)!=4) break;
    uint32_t chunkSize; if(!rd32(chunkSize)) break;
    if (!strncmp(tag,"fmt ",4)){
      uint16_t audioFmt; if(!rd16(audioFmt)) return false;
      if(!rd16(m.channels)) return false;
      if(!rd32(m.sampleRate)) return false;
      uint32_t byteRate; if(!rd32(byteRate)) return false;
      uint16_t align; if(!rd16(align)) return false;
      if(!rd16(m.bitsPerSample)) return false;
      // skip rest if any
      if (chunkSize>16) f.seek(f.position() + (chunkSize-16));
      fmtOk=true;
    } else if (!strncmp(tag,"data",4)){
      m.dataOffset = f.position();
      m.dataBytes  = chunkSize;
      dataOk=true;
      // ne pas avancer, on lira après
      break;
    } else {
      f.seek(f.position()+chunkSize);
    }
  }
  return fmtOk && dataOk && m.bitsPerSample==16 && (m.channels==1 || m.channels==2);
}

static void wav_write_header(File &out, const WavMeta &m){
  // RIFF
  out.write((const uint8_t*)"RIFF",4);
  uint32_t riffSize = 36 + m.dataBytes;
  out.write((uint8_t*)&riffSize,4);
  out.write((const uint8_t*)"WAVE",4);
  // fmt
  out.write((const uint8_t*)"fmt ",4);
  uint32_t sc1=16; out.write((uint8_t*)&sc1,4);
  uint16_t fmt=1;  out.write((uint8_t*)&fmt,2);
  out.write((uint8_t*)&m.channels,2);
  out.write((uint8_t*)&m.sampleRate,4);
  uint16_t bps = m.bitsPerSample;
  uint16_t align = (m.channels * bps)/8;
  uint32_t byteRate = m.sampleRate * align;
  out.write((uint8_t*)&byteRate,4);
  out.write((uint8_t*)&align,2);
  out.write((uint8_t*)&bps,2);
  // data
  out.write((const uint8_t*)"data",4);
  out.write((uint8_t*)&m.dataBytes,4);
}

// ===============================================================
// ==================  API ENREGISTREMENT  =======================
// ===============================================================

extern "C" {
  void     rec_start();
  void     rec_stop();
  bool     rec_is_active();
  uint32_t rec_duration_ms();
  void     rec_get_vu(uint16_t* L, uint16_t* R);
  const char* rec_tmp_path();

  // Édition / waveform
  bool     rec_build_waveform(const char* path, uint16_t target_points /*<=WF_MAX_POINTS*/);
  uint16_t rec_wave_copy(uint8_t* out, uint16_t max);
  uint32_t rec_total_ms();

  void     rec_edit_set_range_ms(uint32_t startMs, uint32_t stopMs);
  void     rec_edit_get_range_ms(uint32_t* startMs, uint32_t* stopMs);
  void     rec_edit_set_speed(float speed1x);
  void     rec_edit_set_pitch_semitones(float semi);
  bool     rec_render_to(const char* dstPath, bool applyTrim, bool applySpeedPitch);

  // --- Wrappers compat UI (anciens noms) ---
  void     rec_set_speed_percent(int pct);
  void     rec_set_pitch_semitones(int semis);
  void     rec_set_region(uint32_t startMs, uint32_t stopMs);
}

// NB : l’acquisition I2S n’est pas ré-implémentée ici (tu as déjà audio_hal/i2s côté projet).
// On se contente d’écrire dans s_recFile et de simuler VU via accumulation RMS.
// Branche tes callbacks RX vers rec_on_rx_samples() si tu veux la VU live.

static uint16_t s_vuL=0, s_vuR=0;

void rec_start(){
  if (s_recActive) return;
  // (ré)écrit header placeholder puis ferme (writer rouvre en append)
  SD.remove(kRecTmpPath);
  File f = SD.open(kRecTmpPath, FILE_WRITE);
  if (!f) return;

  // placeholder header, on finalise à la fin
  WavMeta m; m.channels=2; m.sampleRate=44100; m.bitsPerSample=16; m.dataBytes=0;
  wav_write_header(f, m);
  f.close();

  s_recBytes=0; s_recStartMs=millis(); s_recDurMs=0;
  s_vuL=s_vuR=0;
  s_recActive=true;
}

void rec_stop(){
  if (!s_recActive) return;
  s_recActive=false;

  // Attendre drain du RB + fermeture du writer
  uint32_t t0 = millis();
  while (audio_rb_avail_to_read() > 0) {
    vTaskDelay(pdMS_TO_TICKS(5));
    if (millis() - t0 > 2000) break;    // garde-fou 2s
  }
  t0 = millis();
  while (g_sdwr_open) {
    vTaskDelay(pdMS_TO_TICKS(5));
    if (millis() - t0 > 500) break;     // garde-fou 0.5s
  }

  // Finaliser header
  File f = SD.open(kRecTmpPath, FILE_WRITE);
  if (f) {
    f.seek(0);
    WavMeta m; m.channels=2; m.sampleRate=44100; m.bitsPerSample=16; m.dataBytes=s_recBytes;
    wav_write_header(f, m);
    f.close();
  }

  // (Re)construit la waveform pour l’UI
  rec_build_waveform(kRecTmpPath, WF_MAX_POINTS);
}

bool rec_is_active(){ return s_recActive; }

uint32_t rec_duration_ms(){ return s_recDurMs; }

void rec_get_vu(uint16_t* L, uint16_t* R){ if(L)*L=s_vuL; if(R)*R=s_vuR; }

const char* rec_tmp_path(){ return kRecTmpPath; }

// A appeler par ton driver RX si tu veux alimenter le fichier pendant rec
// samples interleaved LR, frames = nb d’échantillons stéréo
void rec_on_rx_samples(const int16_t* interleavedLR, size_t frames){
  if (!s_recActive) return;
  // push vers ring buffer (stéréo → 2 échantillons par frame)
  size_t want   = frames * 2;
  size_t pushed = audio_rb_write(interleavedLR, want);
  s_recBytes   += pushed * sizeof(int16_t); // octets réellement acceptés

  // VU RMS simple
  uint64_t accL=0, accR=0;
  for (size_t i=0;i<frames;i++){
    int16_t L=interleavedLR[2*i+0], R=interleavedLR[2*i+1];
    accL += (int32_t)L*L; accR += (int32_t)R*R;
  }
  accL /= frames; accR /= frames;
  s_vuL = (uint16_t)(sqrtf((float)accL)) >> 8; // 0..255 approx
  s_vuR = (uint16_t)(sqrtf((float)accR)) >> 8;
  s_recDurMs = millis() - s_recStartMs;
}

// ===============================================================
// ===================  WAVEFORM / ÉDITION  ======================
// ===============================================================

uint32_t rec_total_ms(){
  if (s_sampleRate==0) return 0;
  return (uint32_t)((1000.0 * s_totalFrames) / (double)s_sampleRate);
}

uint16_t rec_wave_copy(uint8_t* out, uint16_t max){
  uint16_t n = min(s_wfCount, max);
  for (uint16_t i=0;i<n;i++) out[i] = (uint8_t) s_wfPeaks[i];
  return n;
}

void rec_edit_set_range_ms(uint32_t startMs, uint32_t stopMs){
  uint64_t startF = (uint64_t)startMs * s_sampleRate / 1000ULL;
  uint64_t stopF  = (uint64_t)stopMs  * s_sampleRate / 1000ULL;
  if (stopF > s_totalFrames) stopF = s_totalFrames;
  if (startF >= stopF) startF = (stopF>0)?(stopF-1):0;
  s_editStartFrame = (uint32_t) startF;
  s_editStopFrame  = (uint32_t) stopF;
}

void rec_edit_get_range_ms(uint32_t* startMs, uint32_t* stopMs){
  if (startMs) *startMs = (uint32_t)(1000.0 * s_editStartFrame / (double)s_sampleRate);
  if (stopMs)  *stopMs  = (uint32_t)(1000.0 * s_editStopFrame  / (double)s_sampleRate);
}

void rec_edit_set_speed(float speed1x){
  if (speed1x < 0.25f) speed1x = 0.25f;
  if (speed1x > 4.0f)  speed1x = 4.0f;
  s_editSpeed = speed1x;
}

void rec_edit_set_pitch_semitones(float semi){
  if (semi < -24.0f) semi = -24.0f;
  if (semi > +24.0f) semi = +24.0f;
  s_editPitchSemi = semi;
}

// === Wrappers compat ===
void rec_set_speed_percent(int pct){
  if (pct < 25)  pct = 25;
  if (pct > 400) pct = 400;
  rec_edit_set_speed(pct / 100.0f);
}
void rec_set_pitch_semitones(int semis){
  rec_edit_set_pitch_semitones((float)semis);
}
void rec_set_region(uint32_t startMs, uint32_t stopMs){
  rec_edit_set_range_ms(startMs, stopMs);
}

// Construit peaks (0..255) uniformément répartis sur la largeur demandée
bool rec_build_waveform(const char* path, uint16_t target_points){
  s_wfCount = 0;
  File f = SD.open(path, FILE_READ); if(!f) return false;
  WavMeta m; if (!wav_read_header(f,m)) { f.close(); return false; }
  s_sampleRate = m.sampleRate; s_channels = m.channels;
  s_totalFrames = (m.dataBytes/2) / m.channels;
  if (target_points==0 || target_points>WF_MAX_POINTS) target_points = WF_MAX_POINTS;

  // init edit range par défaut
  s_editStartFrame = 0;
  s_editStopFrame  = s_totalFrames;

  // setup
  f.seek(m.dataOffset);
  const uint32_t framesPerBucket = max<uint32_t>(1, s_totalFrames / target_points);
  const uint32_t bytesPerFrame = 2 * m.channels;

  uint8_t peakMax=0;
  for (uint16_t p=0;p<target_points;p++){
    uint32_t framesToRead = framesPerBucket;
    uint64_t absmax = 0;
    for (uint32_t i=0;i<framesToRead && f.available(); i++){
      int16_t L; f.read((uint8_t*)&L,2);
      absmax = max<uint64_t>(absmax, (uint64_t)abs(L));
      if (m.channels==2){
        int16_t R; f.read((uint8_t*)&R,2);
        absmax = max<uint64_t>(absmax, (uint64_t)abs(R));
      }
    }
    // skip if bucket shorter
    if (!f.available()) {
      s_wfPeaks[p] = (uint16_t) (min<uint64_t>(absmax, 32767ULL) * 255 / 32767ULL);
      s_wfCount = p+1;
      break;
    }
    s_wfPeaks[p] = (uint16_t) (min<uint64_t>(absmax, 32767ULL) * 255 / 32767ULL);
    peakMax = max<uint8_t>(peakMax, (uint8_t)s_wfPeaks[p]);
  }
  if (s_wfCount==0) s_wfCount = target_points;
  f.close();
  return true;
}

// Rendu (export) : écrit un WAV selon les réglages actuels.
// Si applyTrim=true → n’exporte que [start..stop).
// Si applySpeedPitch=true → facteur = speed * 2^(pitch/12) via resampling linéaire.
bool rec_render_to(const char* dstPath, bool applyTrim, bool applySpeedPitch){
  File src = SD.open(kRecTmpPath, FILE_READ); if(!src) return false;
  WavMeta m; if(!wav_read_header(src,m)){ src.close(); return false; }
  src.seek(m.dataOffset);

  uint32_t startF = applyTrim ? s_editStartFrame : 0;
  uint32_t stopF  = applyTrim ? s_editStopFrame  : (m.dataBytes/2)/m.channels;
  if (stopF<=startF) { src.close(); return false; }

  // facteur resampling
  double factor = 1.0;
  if (applySpeedPitch){
    factor = (double)s_editSpeed * pow(2.0, s_editPitchSemi/12.0);
  }

  // ouvre out
  SD.remove(dstPath);
  File out = SD.open(dstPath, FILE_WRITE); if(!out){ src.close(); return false; }

  // On sort toujours en stéréo pour simplifier (mono dupliqué sur R)
  const uint16_t OUT_CH = 2;
  WavMeta outm; outm.channels=OUT_CH; outm.sampleRate=m.sampleRate; outm.bitsPerSample=16; outm.dataBytes=0;
  wav_write_header(out, outm);

  // Position source au frame startF
  uint32_t bytesPerFrame = 2 * m.channels;
  src.seek(m.dataOffset + (uint32_t)((uint64_t)startF * bytesPerFrame));

  // Resampling linéaire (mono ou stereo)
  const size_t SRC_CH = m.channels;
  const size_t CHUNK = IO_CHUNK_FRAMES;
  int16_t* srcBuf = (int16_t*)malloc(CHUNK * SRC_CH * sizeof(int16_t));
  if(!srcBuf){ src.close(); out.close(); SD.remove(dstPath); return false; }

  double srcIndex = 0.0; // index dans [startF..stopF)
  double srcEnd   = (double)(stopF - startF);

  auto fetchFrame = [&](uint32_t frameOffset, int16_t* L, int16_t* R){
    // lit 1 frame à position absolue (depuis startF déjà appliqué à seek initial)
    src.seek(m.dataOffset + (uint32_t)((startF + frameOffset) * bytesPerFrame));
    if (SRC_CH==1){
      int16_t s=0; if(src.read((uint8_t*)&s,2)==2){ *L=s; *R=s; return true; }
      return false;
    }else{
      int16_t l=0,r=0; if(src.read((uint8_t*)&l,2)!=2) return false;
      if(src.read((uint8_t*)&r,2)!=2) return false;
      *L=l; *R=r; return true;
    }
  };

  // boucle de rendu
  while (srcIndex < srcEnd){
    // indices pour interp
    uint32_t i0 = (uint32_t)srcIndex;
    uint32_t i1 = min<uint32_t>(i0+1, (uint32_t)srcEnd);
    double   t  = srcIndex - (double)i0;

    int16_t L0=0,R0=0,L1=0,R1=0;
    if (!fetchFrame(i0,&L0,&R0)) break;
    if (!fetchFrame(i1,&L1,&R1)) { L1=L0; R1=R0; }

    int32_t Li = (int32_t)((1.0-t)*L0 + t*L1);
    int32_t Ri = (int32_t)((1.0-t)*R0 + t*R1);

    // écrit stéréo: si source mono -> duplique L sur R
    out.write((uint8_t*)&Li,2);
    if (SRC_CH==2) out.write((uint8_t*)&Ri,2);
    else           out.write((uint8_t*)&Li,2);
    outm.dataBytes += 4; // 2 canaux * 16 bits
    srcIndex += factor;
  }

  // finalise header
  out.seek(0);
  wav_write_header(out, outm);
  out.close();
  src.close();
  free(srcBuf);
  return true;
}

void recorder_poll(void) {
  // Si tu pushes l'audio via rec_on_rx_samples() (callbacks I2S), rien à faire ici.
  // Laisse vide pour satisfaire le linker. (Tu pourras y mettre un flush SD si besoin)
}
