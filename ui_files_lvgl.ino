// ui_files_lvgl.ino
#include <lvgl.h>
#include <SD.h>

extern bool loadWavToSlot(const char* path, uint8_t slot);
extern uint8_t selected_sound;

// Clavier LVGL (tab ui_kb_lvgl.ino)
extern void kb_prompt_text(const char* title, bool passwordMode, const char* initial,
                           void (*on_ok)(const char*), void (*on_cancel)());

static lv_obj_t* scr_files;
static lv_obj_t* list_files;

// Fichier sélectionné (mis à jour quand on clique un item)
static char g_selected_name[64] = {0};     // Nom (ex: "kick.wav")
static char g_selected_full[96] = {0};     // Chemin (ex: "/samples/kick.wav")

// Helpers
static inline void set_selected(const char* name) {
  if (!name) return;
  strncpy(g_selected_name, name, sizeof(g_selected_name)-1);
  snprintf(g_selected_full, sizeof(g_selected_full), "/samples/%s", g_selected_name);
}

static void cb_back_files(lv_event_t*){
  extern void build_main_menu();
  build_main_menu();
}

static void refresh_files(){
  lv_obj_clean(list_files);

  File dir = SD.open("/samples");
  if (!dir || !dir.isDirectory()) {
    SD.mkdir("/samples");
    dir = SD.open("/samples");
  }
  // Reset selection
  g_selected_name[0] = '\0';
  g_selected_full[0] = '\0';

  for (File e = dir.openNextFile(); e; e = dir.openNextFile()){
    if (!e.isDirectory()){
      lv_obj_t* btn = lv_list_add_button(list_files, LV_SYMBOL_FILE, e.name());

      // 1) Click : charge aussi dans le slot courant ET sélectionne l’item
      lv_obj_add_event_cb(btn, [](lv_event_t* ev){
        lv_obj_t* btn = lv_event_get_target_obj(ev);
        const char* name = lv_list_get_button_text(list_files, btn);
        if (!name) return;
        set_selected(name);
        loadWavToSlot(g_selected_full, selected_sound);
      }, LV_EVENT_CLICKED, NULL);

      // 2) Long press : ne charge pas, ne fait que sélectionner
      lv_obj_add_event_cb(btn, [](lv_event_t* ev){
        lv_obj_t* btn = lv_event_get_target_obj(ev);
        const char* name = lv_list_get_button_text(list_files, btn);
        if (!name) return;
        set_selected(name);
      }, LV_EVENT_LONG_PRESSED, NULL);
    }
  }
}

// ===== RENAME =====
static void rename_ok(const char* txt){
  if (!g_selected_name[0]) return;       // rien sélectionné
  if (!txt || !*txt) return;             // nom vide
  // Construire destination
  char dst[96];
  snprintf(dst, sizeof(dst), "/samples/%s", txt);

  // Si destination existe déjà, on la remplace
  if (SD.exists(dst)) SD.remove(dst);
  SD.rename(g_selected_full, dst);

  // Mettre à jour la sélection et l’affichage
  set_selected(txt);
  refresh_files();
}
static void rename_cancel(){ /* rien */ }

static void cb_rename(lv_event_t*){
  if (!g_selected_name[0]) {
    // pas de sélection : proposer de saisir directement un nom
    kb_prompt_text("Rename (select file first or type target)", false, "", rename_ok, rename_cancel);
    return;
  }
  // Proposer le nom actuel par défaut
  kb_prompt_text("Rename", false, g_selected_name, rename_ok, rename_cancel);
}

// ===== DELETE =====
// Par consigne : on utilise aussi le clavier LVGL (taper DELETE pour confirmer)
static void delete_ok(const char* txt){
  if (!g_selected_name[0]) return;
  if (!txt) return;

  // Accepte "DELETE" ou "YES" (insensible à la casse)
  bool confirm = false;
  // Uppercase copy
  char up[16]; int i=0; for (; txt[i] && i<15; ++i){ up[i] = (char)toupper((unsigned char)txt[i]); } up[i]=0;
  if (!strcmp(up, "DELETE") || !strcmp(up, "YES")) confirm = true;
  if (!confirm) return;

  // Supprime
  SD.remove(g_selected_full);
  g_selected_name[0] = '\0';
  g_selected_full[0] = '\0';
  refresh_files();
}
static void delete_cancel(){ /* rien */ }

static void cb_delete(lv_event_t*){
  if (!g_selected_name[0]) {
    // Rien de sélectionné → pas de suppression
    kb_prompt_text("Type DELETE to confirm (no selection)", false, "", delete_ok, delete_cancel);
    return;
  }
  char title[96];
  snprintf(title, sizeof(title), "Delete %s (type DELETE)", g_selected_name);
  kb_prompt_text(title, false, "", delete_ok, delete_cancel);
}

void build_files_view(){
  scr_files = lv_obj_create(NULL);
  lv_scr_load(scr_files);

  // Titre
  lv_obj_t* ttl = lv_label_create(scr_files);
  lv_label_set_text(ttl, "Files (/samples)");
  lv_obj_align(ttl, LV_ALIGN_TOP_LEFT, 6, 6);

  // BACK (haut droite)
  lv_obj_t* back = lv_button_create(scr_files);
  lv_obj_set_size(back, 70, 32);
  lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -6, 6);
  lv_obj_add_event_cb(back, cb_back_files, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(back); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  // Liste
  list_files = lv_list_create(scr_files);
  lv_obj_set_size(list_files, 460, 200);
  lv_obj_align(list_files, LV_ALIGN_TOP_MID, 0, 36);

  // Boutons bas
  auto mk = [&](const char* t, lv_event_cb_t cb, int x){
    lv_obj_t* b = lv_button_create(scr_files);
    lv_obj_set_size(b, 110, 36);
    lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, x, -8);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l = lv_label_create(b); lv_label_set_text(l, t); lv_obj_center(l);
    return b;
  };
  mk("REFRESH", [](lv_event_t*){ refresh_files(); }, 10);
  mk("RENAME",  cb_rename,                           130);
  mk("DELETE",  cb_delete,                           250);

  refresh_files();
}
