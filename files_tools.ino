////////////////////////////////////////////////////////////////////////////////////////////

void load_pattern(byte pat){

  // Leer pattren
  String nombreArchivoP = "/PAT" + String(pat)+"_"+String(selected_memory);
  File archivoP = SPIFFS.open(nombreArchivoP, FILE_READ);   
  if (!archivoP) {
    Serial.println("Error al abrir el archivo para leer");
    return;
  }
  int fila = 0;
  while (archivoP.available()) {
    pattern[fila] = (archivoP.readStringUntil('\n')).toInt();
    //Serial.println(pattern[fila]);
    fila++;
  }
  archivoP.close();

  // Leer melodic
  String nombreArchivoM = "/MED" + String(pat)+"_"+String(selected_memory);
  File archivoM = SPIFFS.open(nombreArchivoM, FILE_READ);
  if (!archivoM) {
    Serial.println("Error al abrir el archivo para leer");
    return;
  }
  fila = 0;
  int columna = 0;  
  while (archivoM.available()) {
    melodic[fila][columna] = (archivoM.readStringUntil('\n')).toInt();
    columna++;
    if (columna == 16) {
      fila++;
      columna = 0;
    }
  }
  archivoM.close();  
}

void load_sound(byte pat){
  
  // Leer sound
  String nombreArchivoS = "/SND" + String(pat)+"_"+String(selected_memory);
  File archivoS = SPIFFS.open(nombreArchivoS, FILE_READ);
  if (!archivoS) {
    Serial.println("Error al abrir el archivo para leer");
    return;
  }
  int fila = 0;
  int columna = 0;  
  while (archivoS.available()) {
    ROTvalue[fila][columna] = (archivoS.readStringUntil('\n')).toInt();
    columna++;
    if (columna == 8) {
      // Set voices
      setSound(fila);
      fila++;
      columna = 0;
    }
  }
  archivoS.close();
  flag_ss=true; 
}

void save_pattern(byte pat){
  
  // Guardar pattern
  String nombreArchivoP = "/PAT" + String(pat)+"_"+String(selected_memory);
  File archivoP = SPIFFS.open(nombreArchivoP, FILE_WRITE);   
  if (!archivoP) {
    Serial.println("Error al abrir el archivo para escribir");
    return;
  }
  for (int i = 0; i < 16; i++) {
    archivoP.println(pattern[i]);
    //Serial.println(pattern[i]);
  }
  archivoP.close();
  //Serial.println("-");  

  // Guardar melodic
  String nombreArchivoM = "/MED" + String(pat)+"_"+String(selected_memory);
  File archivoM = SPIFFS.open(nombreArchivoM, FILE_WRITE); 
  if (!archivoM) {
    Serial.println("Error al abrir el archivo para escribir");
    return;
  }
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      archivoM.println(melodic[i][j]);
    }
  }
  archivoM.close();  
}

void save_sound(byte pat){
  
  // Guardar sound
  String nombreArchivoS = "/SND" + String(pat)+"_"+String(selected_memory);
  File archivoS = SPIFFS.open(nombreArchivoS, FILE_WRITE); 
  if (!archivoS) {
    Serial.println("Error al abrir el archivo para escribir");
    return;
  }
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 8; j++) {
      archivoS.println(ROTvalue[i][j]);
    }
  }
  archivoS.close();
}

