#include <FS.h>
#include <SD.h>

// --- helper: toutes les données patterns/sons sur la carte SD ---
static fs::FS &DATAFS = SD;

extern byte selected_memory;
extern uint16_t pattern[16];
extern uint8_t  melodic[16][16];
extern int32_t  ROTvalue[16][8];
extern void setSound(byte);
extern bool flag_ss;

static String makePath(const char* prefix, byte pat, byte mem) {
  // mêmes noms que ton code d’origine, à la racine de la SD
  return String("/") + prefix + String(pat) + "_" + String(mem);
}

void load_pattern(byte pat){
  // PATTERN
  String nombreArchivoP = makePath("PAT", pat, selected_memory);
  File archivoP = DATAFS.open(nombreArchivoP, FILE_READ);
  if (!archivoP) {
    Serial.println("Error al abrir el archivo para leer PAT");
    return;
  }
  int fila = 0;
  while (archivoP.available() && fila < 16) {
    pattern[fila] = (archivoP.readStringUntil('\n')).toInt();
    fila++;
  }
  archivoP.close();

  // MELODIC
  String nombreArchivoM = makePath("MED", pat, selected_memory);
  File archivoM = DATAFS.open(nombreArchivoM, FILE_READ);
  if (!archivoM) {
    Serial.println("Error al abrir el archivo para leer MED");
    return;
  }
  fila = 0;
  int columna = 0;  
  while (archivoM.available() && fila < 16) {
    melodic[fila][columna] = (archivoM.readStringUntil('\n')).toInt();
    columna++;
    if (columna == 16) { fila++; columna = 0; }
  }
  archivoM.close();  
}

void load_sound(byte pat){
  String nombreArchivoS = makePath("SND", pat, selected_memory);
  File archivoS = DATAFS.open(nombreArchivoS, FILE_READ);
  if (!archivoS) {
    Serial.println("Error al abrir el archivo para leer SND");
    return;
  }
  int fila = 0, columna = 0;  
  while (archivoS.available() && fila < 16) {
    ROTvalue[fila][columna] = (archivoS.readStringUntil('\n')).toInt();
    columna++;
    if (columna == 8) {
      setSound(fila);    // garder ton comportement
      fila++;
      columna = 0;
    }
  }
  archivoS.close();
  flag_ss = true; 
}

void save_pattern(byte pat){
  // PATTERN
  String nombreArchivoP = makePath("PAT", pat, selected_memory);
  File archivoP = DATAFS.open(nombreArchivoP, FILE_WRITE);
  if (!archivoP) {
    Serial.println("Error al abrir el archivo para escribir PAT");
    return;
  }
  for (int i = 0; i < 16; i++) archivoP.println(pattern[i]);
  archivoP.close();

  // MELODIC
  String nombreArchivoM = makePath("MED", pat, selected_memory);
  File archivoM = DATAFS.open(nombreArchivoM, FILE_WRITE); 
  if (!archivoM) {
    Serial.println("Error al abrir el archivo para escribir MED");
    return;
  }
  for (int i = 0; i < 16; i++)
    for (int j = 0; j < 16; j++)
      archivoM.println(melodic[i][j]);
  archivoM.close();  
}

void save_sound(byte pat){
  String nombreArchivoS = makePath("SND", pat, selected_memory);
  File archivoS = DATAFS.open(nombreArchivoS, FILE_WRITE); 
  if (!archivoS) {
    Serial.println("Error al abrir el archivo para escribir SND");
    return;
  }
  for (int i = 0; i < 16; i++)
    for (int j = 0; j < 8; j++)
      archivoS.println(ROTvalue[i][j]);
  archivoS.close();
}
