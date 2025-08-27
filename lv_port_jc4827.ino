// lv_port_jc4827.ino
// Port LVGL v9 pour JC4827W543, aligné sur ton exemple radio

#include <lvgl.h>
#include <PINS_JC4827W543.h> // fournit 'gfx' et GFX_BL (Arduino_GFX + Dev Device Pins)
#include "TAMC_GT911.h"

// ---- Touch GT911 : mêmes pins/rotation que l'exemple ----
#define TOUCH_SDA    8
#define TOUCH_SCL    4
#define TOUCH_INT    3
#define TOUCH_RST    38
#define TOUCH_WIDTH  480
#define TOUCH_HEIGHT 272
static TAMC_GT911 touchController(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

// ---- Display / LVGL globals ----
static uint32_t screenWidth;
static uint32_t screenHeight;
static uint32_t bufSize;
static lv_display_t *disp = nullptr;
static lv_color_t  *disp_draw_buf = nullptr;

// ---- Callbacks identiques à l’exemple ----
static uint32_t lvgl_millis_cb(void) { return millis(); }

static void lvgl_print(lv_log_level_t level, const char *buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

static void lvgl_disp_flush(lv_display_t *d, const lv_area_t *a, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(a);
  uint32_t h = lv_area_get_height(a);
  gfx->draw16bitRGBBitmap(a->x1, a->y1, (uint16_t *)px_map, w, h);
  lv_disp_flush_ready(d);
}

static void lvgl_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  LV_UNUSED(indev);
  touchController.read();
  if (touchController.isTouched && touchController.touches > 0) {
    data->point.x = touchController.points[0].x;
    data->point.y = touchController.points[0].y;
    data->state   = LV_INDEV_STATE_PRESSED;
  } else {
    data->state   = LV_INDEV_STATE_RELEASED;
  }
}

void lv_port_init() {
  // Init écran
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
    while (true) { delay(1000); }
  }
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  gfx->fillScreen(RGB565_BLACK);

  // Touch
  touchController.begin();
  touchController.setRotation(ROTATION_INVERTED);

  // LVGL
  lv_init();
  lv_tick_set_cb(lvgl_millis_cb);
#if LV_USE_LOG
  lv_log_register_print_cb(lvgl_print);
#endif

  screenWidth  = gfx->width();   // 480
  screenHeight = gfx->height();  // 272
  bufSize      = screenWidth * 40; // ~40 lignes

  disp_draw_buf = (lv_color_t*)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) disp_draw_buf = (lv_color_t*)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
    while (true) { delay(1000); }
  }

  disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_flush_cb(disp, lvgl_disp_flush);
  lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, lvgl_touchpad_read);
}
