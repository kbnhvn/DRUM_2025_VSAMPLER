#pragma once

// Enum fort pour éviter les collisions int/View
enum View : uint8_t {
  VIEW_MAIN   = 0,
  VIEW_MENU   = 1,
  VIEW_BROWSER= 2,
  VIEW_WIFI   = 3,
  VIEW_PICKER = 4,
  VIEW_PATTERN= 5,
  VIEW_SONG   = 6
};

// Une seule définition dans menu_view.ino, ailleurs : extern
extern View currentView;