/* lv_conf.h — minimal v9 config for ESP32 (JC4827W543 480x272, RGB565) */
#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   Color settings
 *===================*/
#define LV_COLOR_DEPTH          16
/* Swap the high and low bytes of RGB565 color. 
 * Set to 1 if colors look inverted on your display. Usually 0 for ESP32 RGB(I2S) or SPI.*/
#define LV_COLOR_16_SWAP        0

/*====================
   Feature usage
 *===================*/
#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN

/* Enable all widgets (roller, bar, arc, canvas, chart, button, label, slider, etc.) */
#define LV_USE_ALL_WIDGETS      1

/* Draw engine */
#define LV_USE_DRAW_SW          1
#define LV_DRAW_SW_COMPLEX      1

/* Timers */
#define LV_USE_TIMER            1

/* User data in objects (safe to keep enabled) */
#define LV_USE_USER_DATA        1

/*====================
   Display & input
 *===================*/
/* Not strictly required here; your code sets buffers and flush cb at runtime.
 * Keep LVGL generic; port (lv_port_init) will configure display & touch. */

/*====================
   Memory
 *===================*/
/* Use standard malloc/free (good on ESP32 with PSRAM or internal heap) */
#define LV_MEM_CUSTOM           0

/*====================
   Misc
 *===================*/
#define LV_BUILD_TEST           0
#define LV_USE_ASSERT_NULL      0
#define LV_USE_ASSERT_MALLOC    0
#define LV_USE_ASSERT_STYLE     0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ       0

#endif /* LV_CONF_H */
