// #define BLACK RGB565_BLACK
// #define NAVY RGB565_NAVY
// #define DARKGREEN RGB565_DARKGREEN
// #define DARKCYAN RGB565_DARKCYAN
// #define MAROON RGB565_MAROON
// #define PURPLE RGB565_PURPLE
// #define OLIVE RGB565_OLIVE
// #define LIGHTGREY RGB565_LIGHTGREY
// #define DARKGREY RGB565_DARKGREY
// #define BLUE RGB565_BLUE
// #define GREEN RGB565_GREEN
// #define CYAN RGB565_CYAN
// #define RED RGB565_RED
// #define MAGENTA RGB565_MAGENTA
// #define YELLOW RGB565_YELLOW
// #define WHITE RGB565_WHITE
// #define ORANGE RGB565_ORANGE
// #define GREENYELLOW RGB565_GREENYELLOW
// #define PALERED RGB565_PALERED


#define OSCURO RGB565(20, 25, 20)
#define ZGREEN RGB565(50, 90, 50)

#define ZRED RGB565(222, 43, 27)
#define ZYELLOW RGB565(201, 191, 40)
#define ZMAGENTA RGB565(128, 92, 156)
#define ZCYAN RGB565(126, 225, 247)
#define ZBLUE RGB565(80, 110, 240)
#define ZGREENCLEAR RGB565(197, 240, 221)
#define ZGREENALTER RGB565(90, 232, 170)

#define DARKGREY RGB565(50, 50, 50)

//#define PADCOLOR RGB565(45, 30, 45)
#define PADCOLOR RGB565(64, 64, 75)
#define ZBLACK RGB565(1, 1, 1)

const uint8_t icon_on[] = {
    0b00000000, 
    0b11111111, 
    0b11111111, 
    0b11111111, 
    0b11111111, 
    0b11111111,
    0b11111111,
    0b11111111, 
};
const uint8_t icon_off[] = {
    0b00000000, 
    0b11111111, 
    0b00000000, 
    0b11111111, 
    0b00000000, 
    0b11111111,
    0b00000000,
    0b11111111, 
};
void showLastTouched() {
  if (!show_last_touched) {
    return;
  }
  if (last_touched>31) return; // exclude bars

  gfx->drawRect(BPOS[last_touched][0] + 0, BPOS[last_touched][1] + 0, BPOS[last_touched][2] - 1, BPOS[last_touched][3] - 1, BCOLOR[last_touched]);
  gfx->drawRect(BPOS[last_touched][0] + 1, BPOS[last_touched][1] + 1, BPOS[last_touched][2] - 3, BPOS[last_touched][3] - 3, BCOLOR[last_touched]);

}

void clearLastTouched() {
  if (!show_last_touched) {
    return;
  }
  if (start_showlt + (debounce_time * 2) > millis()) {
    return;
  }
  for (byte f = 0; f < 32; f++) {
    gfx->drawRect(BPOS[f][0] + 0, BPOS[f][1] + 0, BPOS[f][2] - 1, BPOS[f][3] - 1, BLACK);
    gfx->drawRect(BPOS[f][0] + 1, BPOS[f][1] + 1, BPOS[f][2] - 3, BPOS[f][3] - 3, BLACK);
  }
  show_last_touched = false;

}

void show_dark_selectors_ONLY1(){  // only once
  int pos_x;
  int pos_y;

  // borrar todos los marcadores de selected_sound
  for (int f=0;f<16;f++){      
    pos_x=BPOS[f][0]+9;
    pos_y=BPOS[f][1]+BPOS[f][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_off, 8, 8, OSCURO, BLACK);      
  }

  // borrar todos los marcadores de selected_pattern 
  for (int f=0;f<16;f++){      
    pos_x=BPOS[f][0]+20;
    pos_y=BPOS[f][1]+BPOS[f][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_off, 8, 8, OSCURO, BLACK);      
  }

  // borrar todos los marcadores de selected_sndSet
  for (int f=0;f<16;f++){      
    pos_x=BPOS[f][0]+31;
    pos_y=BPOS[f][1]+BPOS[f][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_off, 8, 8, OSCURO, BLACK);          
  }  

  // borrar todos los marcadores de selected_memory
  for (int f=0;f<16;f++){      
    pos_x=BPOS[f][0]+42;
    pos_y=BPOS[f][1]+BPOS[f][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_off, 8, 8, OSCURO, BLACK);       
  }
  
}

void show_all_bars_ONLY1(){
  int pos_x;
  int pos_y;
  int bar_max_height;
  for (int f=32;f<48;f++){ 
    gfx->drawRect(BPOS[f][0], BPOS[f][1], BPOS[f][2]-1, BPOS[f][3]-1, DARKGREY);
    pos_x=BPOS[f][0]+8;  
    pos_y=BPOS[f][1]+2;
    bar_max_height=BPOS[f][3]-24;
    gfx->setCursor(pos_x-6,pos_y+bar_max_height+18);
    gfx->setTextColor(DARKGREY, BLACK);
    gfx->print(trot[f-32]);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

void drawPADsound(byte f, int color){
  gfx->fillRect(BPOS[f][0] + 10, BPOS[f][1] + 9, 15, 7, color);
}

void drawPADstep(byte f, int color){
    gfx->fillRect(BPOS[f][0] + 34, BPOS[f][1] + 9, 15, 7, color);
}

void drawBTPAD(byte bt, int color, String texto = "") {

  BCOLOR[bt] = color;
  //gfx->fillRect(BPOS[bt][0] + 3, BPOS[bt][1] + 3, BPOS[bt][2] - 7, BPOS[bt][3] - 7, color);
  gfx->drawRect(BPOS[bt][0] + 3, BPOS[bt][1] + 3, BPOS[bt][2] - 7, BPOS[bt][3] - 7, color);  
}

void drawBT(byte bt, int color, String texto = "") {

  BCOLOR[bt] = color;

  gfx->drawRect(BPOS[bt][0] + 3, BPOS[bt][1] + 3, BPOS[bt][2] - 7, BPOS[bt][3] - 7, color); 
  if (color!=DARKGREY) {
    gfx->drawRect(BPOS[bt][0] + 4, BPOS[bt][1] + 4, BPOS[bt][2] - 9, BPOS[bt][3] - 9, color);
    gfx->drawRect(BPOS[bt][0] + 5, BPOS[bt][1] + 5, BPOS[bt][2] - 11, BPOS[bt][3] - 11, color);  
  } else {
    gfx->drawRect(BPOS[bt][0] + 4, BPOS[bt][1] + 4, BPOS[bt][2] - 9, BPOS[bt][3] - 9, BLACK);
    gfx->drawRect(BPOS[bt][0] + 5, BPOS[bt][1] + 5, BPOS[bt][2] - 11, BPOS[bt][3] - 11, BLACK);      
  } 


  /////////////////////// Button texts
  gfx->setTextColor(color, BLACK);
  gfx->setCursor(BPOS[bt][0] + 8, BPOS[bt][1] + 27);
  if (bt < 16) {
    gfx->setTextColor(LIGHTGREY, BLACK);
    // if write modeZ show note number else print pad number
    if (modeZ == tWrite) {

      if (bitRead(pattern[selected_sound], bt)) {  // note on
        gfx->print(F(melodic[selected_sound][bt]));
        gfx->print("   ");
      }
    } else {      
      gfx->print(F(bt));
      gfx->print("   ");      
    }
  } else if (bt<24){   
    if (texto == "") {
      gfx->print(F(tbuttons1[bt - 16]));
    } else {
      gfx->print(F(texto));
    }
  } else { 
    gfx->setCursor(BPOS[bt][0] + 8, BPOS[bt][1] + 23);
    if (texto == "") {
      gfx->print(F(tbuttons2[bt - 24]));
    } else {
      gfx->print(F(texto));
    }
  }
}

void REFRESH_KEYS() {

  if (refreshMODES) {
    refreshMODES = false;

    // Borrar modos
    for (byte f = 16; f < 24; f++) {
      drawBT(f, DARKGREY);
    }
    switch (modeZ) {
      case tPad:
        drawBT(16, DARKGREEN);
        break;
      case tSel:
        drawBT(16, ZBLUE, " SELECT  ");
        break;
      case tWrite:
        drawBT(16, ZRED,  "  WRITE  ");
        break;
      case tMute:
        drawBT(20, ZBLUE);
        break;
      case tSolo:
        drawBT(20, ZRED,  "  SOLO   ");
        break;
      case tMelod:
        drawBT(20, ZYELLOW, "  MELOD  ");
        break;
      case tClear:
        drawBT(20, ZGREENALTER, "  CLEAR  ");
        break;
      case tFirst:
        drawBT(20, ZCYAN, "  FIRST  ");
        break;
      case tLast:
        drawBT(20, GREENYELLOW, "  LAST   ");
        break;
      case tLoaPS:
        drawBT(18, ZBLUE);
        break;
      case tLoadS:
        drawBT(18, ZCYAN, " LOAD S  ");
        break;
      case tLoadP:
        drawBT(18, ZGREENCLEAR, " LOAD P  ");
        break;
      case tSavPS:
        drawBT(19, ZGREENCLEAR);
        break;
      case tSaveS:
        drawBT(19, ZYELLOW, " SAVE S  ");
        break;
      case tSaveP:
        drawBT(19, ZRED, " SAVE P  ");
        break;
      case tRndS:
        drawBT(17, MAGENTA, "  RND S  ");
        break;
      case tRndP:
        drawBT(17, ZYELLOW, "  RND P  ");
        break;
      case tRndNo:
        drawBT(17, ZCYAN,   "  RND N  ");
        break;
      case tPiano:
        drawBT(21, MAGENTA);
        break;
      case tMemo:
        drawBT(23, ZGREENALTER," MEMORY  ");
        break;
      case tMfirs:
        drawBT(23, ZCYAN, "  FIRST  ");
        break;
      case tMlast:
        drawBT(23, GREENYELLOW, "  LAST   ");
        break;
    }
    // play/stop
    if (playing) {
      drawBT(22, DARKGREEN);
    }
    if (recording) {
      drawBT(22, ZRED, "   REC   ");
    }
    // song
    if (songing) {
      drawBT(23, ZBLUE);
    }
  }

  if (clearPATTERNPADS) {
    clearPATTERNPADS = false;
    clearPATTERN();
  }

  if (refreshPATTERN) {
    refreshPATTERN = false;

    switch (modeZ) {
      case tWrite:

        for (byte f = 0; f < 16; f++) {
          if bitRead (pattern[selected_sound], f) {
            drawBT(f, ZRED);
          } else {
            drawBT(f, PADCOLOR);
          }
        }
        break;
      case tMute:

        for (byte f = 0; f < 16; f++) {
          if bitRead (mutes, f) {
            drawBT(f, ZGREENALTER);
          } else {
            drawBT(f, PADCOLOR);
          }
        }
        break;
      case tSolo:

        for (byte f = 0; f < 16; f++) {
          if bitRead (solos, f) {
            drawBT(f, GREEN);
          } else {
            drawBT(f, PADCOLOR);
          }
        }
        break;
      case tMelod:
        for (byte f = 0; f < 16; f++) {
          if bitRead (isMelodic, f) {
            drawBT(f, ZRED);
          } else {
            drawBT(f, PADCOLOR);
          }
        }
        break;
      case tClear:
        for (byte f = 0; f < 16; f++) {
          drawBT(f, PADCOLOR);
        }
        break;        
      case tFirst:
        for (byte f = 0; f < 16; f++) {
          if (firstStep == f) {
            drawBT(f, GREEN);
          } else {
            drawBT(f, PADCOLOR);
          }
        }
        break;
      case tLast:
        for (byte f = 0; f < 16; f++) {
          if (newLastStep == f) {
            drawBT(f, ZRED);
          } else {
            drawBT(f, PADCOLOR);
          }
        }
        break;
      case tMfirs:
        for (byte f = 0; f < 16; f++) {
          if (firstMemory == f) {
            drawBT(f, GREEN);
          } else {
            drawBT(f, PADCOLOR);
          }
        }
        break;
      case tMlast:
        for (byte f = 0; f < 16; f++) {
          if (newLastMemory == f) {
            drawBT(f, ZRED);
          } else {
            drawBT(f, PADCOLOR);
          }
        }
        break;
      default:
        clearPATTERNPADS=true;        
    }
  }

  if (refreshPADSTEP) {
    refreshPADSTEP = false;
    // Si SEQ activo muestro los pads que suenan
    if (playing) {
      for (byte f = 0; f < 16; f++) {
        if bitRead (pattern[f], sstep) {
          drawPADsound(f, BLUE);
        } else {
          drawPADsound(f, OSCURO);
        }
      }
    }
    // Mostrar step actual
    if (sstep!=s_old_sstep && s_old_sstep!=-1){
      drawPADstep(s_old_sstep, OSCURO);
    } 
    drawPADstep(sstep, YELLOW);
    s_old_sstep=sstep;  
    
  }


  if (clearPADSTEP) {
    clearPADSTEP = false;
    if (modeZ != 2) {
      for (byte f = 0; f < 16; f++) {
        drawPADsound(f, OSCURO);
        drawPADstep(f, OSCURO);
      }
    }

    // refresco lo que hay en el step en el siguiente bucle
    //refreshPATTERN = true;
  }
}



void drawShift() {
  // Actualizar texto botones 0-15
  for (byte f = 0; f < 16; f++) {
    drawBT(f, DARKGREY);
  }
  refreshPATTERN=true;
}

void drawScreen1_ONLY1() {
  for (byte f = 0; f < 32; f++) {
    //gfx->fillRect(BPOS[f][0], BPOS[f][1], BPOS[f][2], BPOS[f][3], BLACK);
    if (f < 16) {
      drawBTPAD(f, PADCOLOR);
      drawPADsound(f,OSCURO);
      drawPADstep(f,OSCURO);
    } else if (f < 24) {
      drawBT(f, DARKGREY);
    } else {
      drawBT(f, DARKGREY);
    }
  }
  show_dark_selectors_ONLY1();
  show_all_bars_ONLY1();

}

void clearPATTERN() {
  for (byte f = 0; f < 16; f++) {
    drawBT(f, PADCOLOR);
  }
}


void drawBar(byte bar){

  // Color
  int qcolor=ZCYAN;
  if (bar>7) qcolor=ZGREENALTER;

  byte qbar=bar+32;
  int pos_x=BPOS[qbar][0]+8;  
  int pos_y=BPOS[qbar][1]+2;
  int ancho=BPOS[qbar][2];

  // valor
  int value=0;
  if (bar==11){
    value=master_filter; 
  } else if (bar==12){
    value=octave;      
  } else if (bar==8){
    value=bpm;
  } else if (bar==9){
    value=master_vol;
  } else if (bar==10){
    ztranspose=0;
    value=ztranspose;
  } else if (bar==11){
    value=octave;     
  } else if (bar==13){
    zmpitch=0;
    value=zmpitch;              
   } else if (bar==14){
    value=sync_state; 
   } else if (bar==15){
    value=selected_scale; 
  } else {
    value=ROTvalue[selected_sound][bar];
  }



    // Tamaño disponible para mostrar la barra: alto - ocupado por el borde- espacio arriba y abajo - dos filas de texto 20 
    int bar_max_height=BPOS[qbar][3]-24;

    int nvalue=map(value,min_values[bar],max_values[bar],0,bar_max_height);
    if (nvalue==0) nvalue=1;
    if (bar>2){
      // refresh bar 
      gfx->fillRect(pos_x, pos_y, 5,bar_max_height-nvalue, OSCURO);
      gfx->fillRect(pos_x, pos_y+bar_max_height-nvalue, 5,nvalue, qcolor);
    }
    // mostrar valor
    if (bar==0) {
      pos_x=pos_x+18;
      gfx->fillRect(pos_x, pos_y+bar_max_height+8, ancho-30,10, BLACK);
      gfx->setCursor(pos_x,pos_y+bar_max_height+18);
    } else {
      pos_x=pos_x-6;
      gfx->fillRect(pos_x, pos_y+bar_max_height+1, ancho-4,10, BLACK);
      gfx->setCursor(pos_x,pos_y+bar_max_height+9);
    }
    gfx->setTextColor(qcolor, BLACK);
    gfx->print(F(value));
    if (bar==0) {
      gfx->setCursor(pos_x+20,pos_y+bar_max_height+18);
      gfx->print(F(sound_names[ROTvalue[selected_sound][0]]));
    }

  if (bar<3) drawWaveform();

}



void draw8aBar(){
  for (byte f = 0; f < 8; f++) {
    drawBar(f);
  }
}
void draw8bBar(){
  for (byte f = 8; f < 16; f++) { 
    drawBar(f);
  }
       
}


void REFRESH_STATUS(){
  
  int pos_x;
  int pos_y;

  // ticks: selected sound, pattern, soundset, memory
  if (selected_sound!=s_old_selected_sound) {    
    pos_x=BPOS[s_old_selected_sound][0]+9;
    pos_y=BPOS[s_old_selected_sound][1]+BPOS[s_old_selected_sound][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_off, 8, 8, OSCURO, BLACK);   
    pos_x=BPOS[selected_sound][0]+9;
    pos_y=BPOS[selected_sound][1]+BPOS[selected_sound][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_on, 8, 8, ZYELLOW, BLACK);  
    s_old_selected_sound=selected_sound; 
  }

  if (selected_pattern!=s_old_selected_pattern) {
    pos_x=BPOS[s_old_selected_pattern][0]+20;
    pos_y=BPOS[s_old_selected_pattern][1]+BPOS[s_old_selected_pattern][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_off, 8, 8, OSCURO, BLACK);
    pos_x=BPOS[selected_pattern][0]+20;
    pos_y=BPOS[selected_pattern][1]+BPOS[selected_pattern][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_on, 8, 8, GREEN, BLACK); 
    s_old_selected_pattern=selected_pattern; 
  }

  if (selected_sndSet!=s_old_selected_sndSet) {    
    pos_x=BPOS[s_old_selected_sndSet][0]+31;
    pos_y=BPOS[s_old_selected_sndSet][1]+BPOS[s_old_selected_sndSet][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_off, 8, 8, OSCURO, BLACK);
    pos_x=BPOS[selected_sndSet][0]+31;
    pos_y=BPOS[selected_sndSet][1]+BPOS[selected_sndSet][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_on, 8, 8, ZBLUE, BLACK);
    s_old_selected_sndSet=selected_sndSet; 
  }

  if (selected_memory!=s_old_selected_memory) {  
    pos_x=BPOS[s_old_selected_memory][0]+42;
    pos_y=BPOS[s_old_selected_memory][1]+BPOS[s_old_selected_memory][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_off, 8, 8, OSCURO, BLACK);    
    pos_x=BPOS[selected_memory][0]+42; 
    pos_y=BPOS[selected_memory][1]+BPOS[selected_memory][3]-20;
    gfx->drawBitmap(pos_x, pos_y, icon_on, 8, 8, MAGENTA, BLACK);
    s_old_selected_memory=selected_memory;
  }

  // selected rotary
   if (selected_rot!=s_old_selected_rot) { 
    gfx->drawRect(BPOS[s_old_selected_rot+32][0], BPOS[s_old_selected_rot+32][1], BPOS[s_old_selected_rot+32][2]-1, BPOS[s_old_selected_rot+32][3]-1, DARKGREY); 
    gfx->drawRect(BPOS[selected_rot+32][0], BPOS[selected_rot+32][1], BPOS[selected_rot+32][2]-1, BPOS[selected_rot+32][3]-1, ZYELLOW); 
    s_old_selected_rot=selected_rot;
   }

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void fillBPOS() {
  int pos_x;
  int pos_y;
 
  byte counter = 0;

   // 16 pad buttons
  int y_accumulated = Btop;
  for (byte r = 0; r < 2; r++) {
    int x_accumulated = Bleft;
    for (byte c = 0; c < 8; c++) {
      // x - left
      BPOS[counter][0] = x_accumulated;
      x_accumulated = x_accumulated+ BwidthPad;
      BPOS[counter][1] = y_accumulated;
      BPOS[counter][2] = BwidthPad;
      BPOS[counter][3] = BheightPad;
      counter++;
    }
    x_accumulated = 0;
    y_accumulated = y_accumulated + BheightPad;

  }
  
  // 8 func buttons
  pos_y=(BheightPad*2)+BheightBar; // 2 rows of pads + 1 row of bar 
  pos_x=0;
  counter=16;
  for (int bb=0;bb<8;bb++){
    pos_x=bb*BwidthFunc;
    BPOS[counter][0] = pos_x;
    BPOS[counter][1] = pos_y;
    BPOS[counter][2] = BwidthFunc;
    BPOS[counter][3] = BheightFunc;
    counter++;      
  }

  // 8 norot buttons; 
  pos_y=(BheightPad*2) + BheightFunc + BheightBar; // 3 rows of pads + 1 row of bar 
  pos_x=0;
  counter=24;
  for (int bb=0;bb<8;bb++){
    pos_x=bb*BwidthFunc;
    BPOS[counter][0] = pos_x;
    BPOS[counter][1] = pos_y;
    BPOS[counter][2] = BwidthNorot;
    BPOS[counter][3] = BheightNorot;
    counter++;      
  }
 
  // 16 bars/columns
  pos_y=BheightPad*2; //two rows of pads
  pos_x=0;
  counter=32;
    BPOS[counter][0] = pos_x;
    BPOS[counter][1] = pos_y;
    BPOS[counter][2] = rot0_width;
    BPOS[counter][3] = rot0_height;
  counter++;
    BPOS[counter][0] = pos_x + rot0_width;
    BPOS[counter][1] = pos_y;
    BPOS[counter][2] = rot1_width;
    BPOS[counter][3] = rot1_height;
  counter++; 
    BPOS[counter][0] = pos_x + rot0_width;
    BPOS[counter][1] = pos_y + rot1_height;
    BPOS[counter][2] = rot2_width;
    BPOS[counter][3] = rot2_height;
  counter++;

  for (int bar=3;bar<16;bar++){
    pos_x=(bar*BwidthBar)+(120-BwidthBar);
    BPOS[counter][0] = pos_x;
    BPOS[counter][1] = pos_y;
    BPOS[counter][2] = BwidthBar;
    BPOS[counter][3] = BheightBar;
    counter++;      
  }

}

void refresh_shift_key(){
  int qcolor=DARKGREY;
  if (shifting) qcolor=ORANGE;
  
  drawBT(24, qcolor);
  drawBT(31, qcolor);
}

void drawWaveform() {
  
  gfx->fillRect(WAVE_ORIGIN_X, WAVE_ORIGIN_Y, WAVE_WIDTH, WAVE_HEIGHT, BLACK);
  gfx->drawRect(WAVE_ORIGIN_X, WAVE_ORIGIN_Y, WAVE_WIDTH, WAVE_HEIGHT, OSCURO);
  int16_t minVal = INT16_MAX;
  int16_t maxVal = INT16_MIN;

  // Encuentra el mínimo y máximo para normalizar
  for (size_t i = 0; i < ENDS[ROTvalue[selected_sound][0]]; i++) {
    if (SAMPLES[ROTvalue[selected_sound][0]][i] < minVal) minVal = SAMPLES[ROTvalue[selected_sound][0]][i];
    if (SAMPLES[ROTvalue[selected_sound][0]][i] > maxVal) maxVal = SAMPLES[ROTvalue[selected_sound][0]][i];
  }

  // Dibuja la forma de onda
  for (int x = 0; x < WAVE_WIDTH - 1; x++) {
    size_t index1 = map(x, 0, WAVE_WIDTH - 1, 0, ENDS[ROTvalue[selected_sound][0]] - 1);
    size_t index2 = map(x + 1, 0, WAVE_WIDTH - 1, 0, ENDS[ROTvalue[selected_sound][0]] - 1);

    int y1 = map(SAMPLES[ROTvalue[selected_sound][0]][index1], minVal, maxVal, WAVE_HEIGHT - 1, 0);
    int y2 = map(SAMPLES[ROTvalue[selected_sound][0]][index2], minVal, maxVal, WAVE_HEIGHT - 1, 0);

    gfx->drawLine(WAVE_ORIGIN_X + x, WAVE_ORIGIN_Y + y1, WAVE_ORIGIN_X + x + 1, WAVE_ORIGIN_Y + y2, ZCYAN);
  }
  // mostrar marca de inicio y de fin
  int xini = map(ROTvalue[selected_sound][1], min_values[1], max_values[1], 0, WAVE_WIDTH-1);
  int xfin = map(ROTvalue[selected_sound][2], min_values[2], max_values[2], 0, WAVE_WIDTH)-1;
  gfx->drawLine(WAVE_ORIGIN_X+xini, WAVE_ORIGIN_Y, WAVE_ORIGIN_X+xini, WAVE_ORIGIN_Y+WAVE_HEIGHT-1, ZGREEN);
  gfx->drawLine(WAVE_ORIGIN_X+xfin, WAVE_ORIGIN_Y, WAVE_ORIGIN_X+xfin, WAVE_ORIGIN_Y+WAVE_HEIGHT-1, ZRED);

  //setSound(selected_sound);

}

//////////////////// New UI helpers ////////////////////
void drawTopBar(const char* title, bool showBack) {
  gfx->fillRect(0, 0, 640, 24, DARKGREY);
  gfx->setCursor(8, 16);
  gfx->setTextColor(WHITE, DARKGREY);
  gfx->print(title);
  if (showBack) {
    gfx->drawRect(600, 4, 32, 16, WHITE);
    gfx->setCursor(608, 16);
    gfx->setTextColor(WHITE, DARKGREY);
    gfx->print("<");
  }
}

void drawButtonBox(int x,int y,int w,int h, int color, const char* texto){
  gfx->drawRect(x + 3, y + 3, w - 7, h - 7, color);
  if (color != DARKGREY) {
    gfx->drawRect(x + 4, y + 4, w - 9, h - 9, color);
    gfx->drawRect(x + 5, y + 5, w - 11, h - 11, color);
  } else {
    gfx->drawRect(x + 4, y + 4, w - 9, h - 9, BLACK);
    gfx->drawRect(x + 5, y + 5, w - 11, h - 11, BLACK);
  }
  gfx->setTextColor(color, BLACK);
  gfx->setCursor(x + 8, y + (h/2) + 3);
  gfx->print(texto);
}
