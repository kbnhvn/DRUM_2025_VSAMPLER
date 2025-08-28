// system_tasks.ino
#include <Arduino.h>
#include <lvgl.h>
#include <SD.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

// ===== Externs venant de ton code =====
extern void midiUSB_poll();
extern void onSync24Callback(uint32_t tick); // sequencer.ino
extern int bpm;                              // BPM global (sequencer)

// (Optionnel) si ton audio crée sa propre tâche ailleurs, on n’y touche pas ici.

// ======= Helpers allocation =======
// DRAM (rapide) pour buffers LVGL / I2S
static inline void* dram_malloc(size_t n) {
  return heap_caps_malloc(n, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}
// PSRAM (large) pour gros buffers (peaks, caches, etc.)
static inline void* psram_malloc(size_t n) {
  return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

// ======= Ring Buffer générique (lock-free mono producteur/consommateur) =======
// Utilisable pour audio, MIDI, ou SD writer.
// Exemple: RB<int16_t> rb; rb.init(capacity_in_elements, /*psram=*/true);
template<typename T>
struct RB {
  T*      buf = nullptr;
  size_t  cap = 0;
  volatile size_t w = 0; // write idx
  volatile size_t r = 0; // read idx

  bool init(size_t capacity, bool use_psram=false) {
    cap = capacity;
    buf = (T*)(use_psram ? psram_malloc(sizeof(T)*cap) : dram_malloc(sizeof(T)*cap));
    w = r = 0;
    return buf != nullptr;
  }
  inline size_t avail_to_read() const {
    size_t ww = w, rr = r;
    return (ww >= rr) ? (ww - rr) : (cap - (rr - ww));
  }
  inline size_t avail_to_write() const {
    return cap - 1 - avail_to_read();
  }
  // push n éléments (retourne combien écrits)
  size_t write(const T* src, size_t n) {
    size_t can = avail_to_write();
    if (n > can) n = can;
    size_t first = min(n, cap - w);
    if (first) memcpy(&buf[w], src, first * sizeof(T));
    size_t left = n - first;
    if (left)  memcpy(&buf[0], src + first, left * sizeof(T));
    w = (w + n) % cap;
    return n;
  }
  // pop n éléments (retourne combien lus)
  size_t read(T* dst, size_t n) {
    size_t can = avail_to_read();
    if (n > can) n = can;
    size_t first = min(n, cap - r);
    if (first) memcpy(dst, &buf[r], first * sizeof(T));
    size_t left = n - first;
    if (left)  memcpy(dst + first, &buf[0], left * sizeof(T));
    r = (r + n) % cap;
    return n;
  }
};

// ======= Ring buffer audio + writer flags =======
RB<int16_t> g_audio_rb;            // interleaved L,R int16_t
volatile bool g_sdwr_open = false; // true quand le writer a le fichier ouvert

// Externs utilisés par le writer
extern "C" {
  bool        rec_is_active(void);
  const char* rec_tmp_path(void);
}

// ======= Tâches =======
static TaskHandle_t hTaskGUI  = nullptr;
static TaskHandle_t hTaskMIDI = nullptr;
static TaskHandle_t hTaskSEQ  = nullptr;
static TaskHandle_t hTaskSDW  = nullptr;

// GUI: unique propriétaire de LVGL (thread-safe)
static void task_gui(void*) {
  for (;;) {
    lv_timer_handler();                  // LVGL v9
    vTaskDelay(pdMS_TO_TICKS(5));        // ~200 Hz
  }
}

// MIDI: poll USB (et/ou DIN) régulièrement, non bloquant
static void task_midi(void*) {
  for (;;) {
    midiUSB_poll();
    vTaskDelay(pdMS_TO_TICKS(1));        // ~1 kHz
  }
}

// PPQN 24: clock logicielle stable via vTaskDelayUntil (suffisant pour 24 ppqn).
// Si tu veux une précision microseconde, passe sur esp_timer + Queue.
static void task_ppqn24(void*) {
  uint32_t tick = 0; // [0..23]
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    const float period_ms = (bpm > 0) ? (1000.0f * 60.0f / (bpm * 24.0f)) : 1000.0f;
    const TickType_t period = pdMS_TO_TICKS((uint32_t)period_ms);
    vTaskDelayUntil(&last, period);
    onSync24Callback(tick);
    tick = (tick + 1) % 24;
  }
}

// SD writer: draine g_audio_rb et append dans /samples/rec_tmp.wav
static void task_sd_writer(void*) {
  File out;
  static int16_t chunk[1024 * 2]; // 1024 frames stéréo

  for (;;) {
    // Ouvrir si enregistrement actif et pas encore ouvert
    if (rec_is_active() && !out) {
      out = SD.open(rec_tmp_path(), FILE_APPEND);
      g_sdwr_open = out ? true : false;
    }

    // Draine le RB
    size_t n = g_audio_rb.read(chunk, sizeof(chunk)/sizeof(chunk[0]));
    if (n > 0 && out) {
      out.write((const uint8_t*)chunk, n * sizeof(int16_t));
    }

    // Si stop & RB vide -> fermer
    if (!rec_is_active() && out && g_audio_rb.avail_to_read() == 0) {
      out.flush();
      out.close();
      g_sdwr_open = false;
    }

    if (n == 0) vTaskDelay(pdMS_TO_TICKS(2));
  }
}


// ======= API =======
extern void system_tasks_init(void) {
  // GUI → core 1 (APP), priorité moyenne
  if (!hTaskGUI)  xTaskCreatePinnedToCore(task_gui,  "GUI",  6144, nullptr, 2, &hTaskGUI,  1);
  // MIDI → core 0 (PRO), priorité basse
  if (!hTaskMIDI) xTaskCreatePinnedToCore(task_midi, "MIDI", 2048, nullptr, 1, &hTaskMIDI, 0);
  // PPQN24 → core 0 (PRO), priorité moyenne
  if (!hTaskSEQ)  xTaskCreatePinnedToCore(task_ppqn24,"SEQ",  2048, nullptr, 2, &hTaskSEQ,  0);

  // Ring buffer en PSRAM (~128 KB): 65 536 éléments int16_t (≈ 32 768 samples stéréo)
  if (!g_audio_rb.buf) g_audio_rb.init(1<<16 /*elements*/, true /*psram*/);

  // Writer SD → core 0, prio 2
  if (!hTaskSDW) xTaskCreatePinnedToCore(task_sd_writer, "SD_WR", 4096, nullptr, 2, &hTaskSDW, 0);
}
