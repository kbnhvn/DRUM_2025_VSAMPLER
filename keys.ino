// Router de vues (défini dans views.h / menu_view.ino)
#include "views.h"
extern View currentView;
void openPatternView();
void openMenuView();
void openSongView();

// Prototypes synth/util utilisés ici
void synthESP32_TRIGGER_P(unsigned char voice, int note);
void synthESP32_TRIGGER(unsigned char voice);
void setSound(byte voice);
void select_rot();
void do_rot();
void setRandomVoice(byte v);
void setRandomVoice2(byte v);
void setRandomPattern(byte v);
void setRandomNotes(byte v);

void DO_KEYPAD(){

  // Si on est dans une vue secondaire → aucune action sur la grille principale
  if (currentView != VIEW_MAIN) {
    return;
  }


  for (int f=0;f<48;f++) {
    if (trigger_on[f]==1){
      trigger_on[f]=0;

      nkey=f;

      show_last_touched=true;
      start_showlt=start_debounce;
 
      if (nkey<16){
         
        switch (modeZ) {

          case tPiano: // 16 keys=16 notes
            synthESP32_TRIGGER_P(selected_sound,nkey+(12*octave));
            if (recording){
              bitWrite(pattern[selected_sound],sstep,1);
              melodic[selected_sound][sstep]=nkey+(12*octave);
            }
            refreshPATTERN=true;
            break;          
          case tPad: // play pads

            synthESP32_TRIGGER(nkey);


            if (recording){
              bitWrite(pattern[nkey],sstep,1); //!bitRead(pattern[nkey],sstep));
              melodic[selected_sound][nkey]=ROTvalue[selected_sound][3];
            } else {

              synthESP32_TRIGGER(nkey);
            
            }
            //if (!shiftR1 && !shifting){
              selected_sound=nkey;
              if (selected_sound!=oldselected_sound){          
                oldselected_sound=selected_sound;
                setSound(nkey);
                select_rot();
                draw8aBar();
              }
            //}
            refreshPATTERN=true;
            break;          
          case tSel:  // Select sound
            selected_sound=nkey;
            if (selected_sound!=oldselected_sound){          
              oldselected_sound=selected_sound;
              setSound(nkey);
              select_rot();
            }
            modeZ=tWrite;
            refreshPATTERN=true;
            draw8aBar();
            refreshMODES=true;
            break;
          case tWrite:
            bitWrite(pattern[selected_sound],nkey,!bitRead(pattern[selected_sound],nkey));
            melodic[selected_sound][nkey]=ROTvalue[selected_sound][3];
            refreshPATTERN=true;
            break;
          case tMelod:
            bitWrite(isMelodic,nkey,!bitRead(isMelodic,nkey));
            refreshPATTERN=true;            
            //selected_sound=nkey;
            break;

          case tMute:
            bitWrite(mutes,nkey,!bitRead(mutes,nkey));
            refreshPATTERN=true;            
            break;
          case tSolo:
            bitWrite(solos,nkey,!bitRead(solos,nkey));
            refreshPATTERN=true;            
            break;
          case tClear: // clear
            pattern[nkey]=0;
            break;
          case tLoadP:
            load_pattern(nkey);
            pattern_song_counter=nkey;
            selected_pattern=nkey;
            // select_rot();
            // draw8aBar();
            break;
          case tSaveP:
            save_pattern(nkey);
            selected_pattern=nkey;
            modeZ=tPad; // pongo el modeZ en 0 para que no se quede en grabar
            refreshMODES=true;
            break;
          case tLoadS:
            load_sound(nkey);
            selected_sndSet=pattern_song_counter;
            selected_sndSet=nkey;
            select_rot();
            draw8aBar();
            break;
          case tSaveS:
            save_sound(nkey);
            selected_sndSet=nkey;
            modeZ=tPad; // pongo el modeZ en 0 para que no se quede en grabar
            refreshMODES=true;
            break;
          case tLoaPS:
            load_pattern(nkey);
            pattern_song_counter=nkey;
            selected_pattern=nkey;
            load_sound(nkey);
            selected_sndSet=pattern_song_counter;
            selected_sndSet=nkey;
            select_rot();
            draw8aBar();
            break;
          case tSavPS:
            save_pattern(nkey);
            selected_pattern=nkey;
            save_sound(nkey);
            selected_sndSet=nkey;                        
            modeZ=tPad; // pongo el modeZ en 0 para que no se quede en grabar
            refreshMODES=true;
            break;
          case tRndS:
            selected_sound=nkey;
            setRandomVoice(nkey);
            setSound(nkey);
            select_rot();
            draw8aBar();
            break;
          case tRndS2:
            selected_sound=nkey;
            setRandomVoice2(nkey);
            setSound(nkey);
            select_rot();
            draw8aBar();
            break;  
          case tRndP:
            selected_sound=nkey;
            setRandomPattern(nkey);
            setSound(nkey);
            // select_rot();
            // draw8aBar();         
            break;
          case tRndNo:
            selected_sound=nkey;
            setRandomNotes(nkey);
            setSound(nkey);
            // select_rot();
            // draw8aBar();        
            break;
          case tFirst:
            refreshPATTERN=true;
            firstStep=nkey;
            break;          
          case tLast:
            refreshPATTERN=true;
            newLastStep=nkey;
            break;  
          case tMemo: // memory
            selected_memory=nkey;
            break;         
          case tMfirs:
            refreshPATTERN=true;
            firstMemory=nkey;
            break;          
          case tMlast:
            refreshPATTERN=true;
            newLastMemory=nkey;
            break; 

        }
        //refreshPADTOUCHED=true;

      //// 8 keys
      } else if (nkey<24){

        refreshMODES=true;
        
        if (!shiftR1 && !shifting){
          switch (nkey) {

            // playpad/select/write
            case 16:
              // 0 - play pad sound
              // 1 - select sound
              // 2 - edit track, write
              if (modeZ>tWrite){
                modeZ=tPad;
              } else {
                modeZ++;
              }

              if (modeZ>tWrite) modeZ=tPad;

              if (modeZ==tWrite) {
                refreshPATTERN=true;
              } else {
                clearPATTERNPADS=true; 
              }
              break;
            // random sound/pattern/Notes      
            case 17:
              if (modeZ==tRndP){
                modeZ=tRndS;
              } else if (modeZ==tRndS){
                modeZ=tRndNo;
              } else {
                modeZ=tRndP;
              }
              clearPATTERNPADS=true;
              break;
            // load       
            case 18:
              if (modeZ==tLoaPS) {
                modeZ=tLoadP;
              } else if (modeZ==tLoadP ) {
                modeZ=tLoadS;
              } else {
                modeZ=tLoaPS;
              }
              clearPATTERNPADS=true;
              break;              
            // save      
            case 19:
              if (modeZ==tSavPS) {
                modeZ=tSaveP;
              } else if (modeZ==tSaveP ) {
                modeZ=tSaveS;
              } else {
                modeZ=tSavPS;
              }
              clearPATTERNPADS=true;       
              break; 
            // solos/mutes/melodic
            case 20:
              if (modeZ==tMute) {
                modeZ=tSolo;
              } else if (modeZ==tSolo){
                modeZ=tMelod;
              } else if (modeZ==tMelod){
                modeZ=tClear;
              } else if (modeZ==tClear){
                modeZ=tFirst;               
              } else if (modeZ==tFirst){
                modeZ=tLast; 
              } else {
                modeZ=tMute;
              } 
              refreshPATTERN=true;                  
              break; 
            // piano
            case 21:
              modeZ=tPiano;
              clearPATTERNPADS=true;
              break;
            // Play
            case 22:
              if (playing){
                uClock.stop();
                //stopTimer();
                sstep=firstStep;
                recording=false;
                clearPADSTEP=true;
                pattern_song_counter=0; 
              } else {
                if (sync_state==2){ // if this machine is slave dont start playing now
                  pre_playing=true;
                } else {
                if (songing) pattern_song_counter=selected_pattern;
                  uClock.start();
                  //startTimer();
                  sstep=firstStep;
                  refreshPADSTEP=true;  
                }
              }
              playing=!playing; 
              break;
            // Song
            case 23:
              openSongView();
              break;
          }  

        // shift 
                                  
        } else {
          switch (nkey){
            case 16:
              modeZ=tSel;
              break;
            case 17:
              modeZ=tRndS2;
              break;  
            // reset mutes & solos and firstStep=0 and lastStep=15       
            case 20:
              mutes=0;
              solos=0; 
              firstStep=0;
              lastStep=15;
              clearPATTERNPADS=true;
              break;
            // Play
            case 22:
              if (playing){
                recording=!recording;
              } else {
                uClock.start();
                //startTimer();
                recording=true;
                playing=true;
                sstep=firstStep;
                refreshPADSTEP=true;  
              }
              if (songing) recording=false; // in song mode cant save modified patterns. I would need a new flag. so much cpu time?
              //refreshMODES=true;  
              break;
            case 23:
               modeZ=tMemo;
               break;                    
          }         
        }
      } else if (nkey<32){
        switch (nkey){
          case 24:
            shifting=!shifting;
            refresh_shift_key();
            break;
          case 25:
            old_counter1=counter1;
            if (shifting || shiftR1) {
              counter1=counter1-1;
            } else {
              counter1=counter1-1;
            }
            do_rot();
            break;
          case 26:
            old_counter1=counter1;
            if (shifting || shiftR1) {
              counter1=counter1-100;
            } else {
              counter1=counter1-10;
            }
            do_rot();
            break;
          case 27:
            old_counter1=counter1;
            if (shifting || shiftR1) {
              counter1=counter1+100;
            } else {
              counter1=counter1+10;
            }
            do_rot();
            break;
          case 28:
            old_counter1=counter1;
            if (shifting || shiftR1) {
              counter1=counter1+1;
            } else {
              counter1=counter1+1;
            }
            do_rot();
            break;
          case 29:
            openPatternView();
            break;
          case 30:
            openMenuView();
            break;
          case 31:
            shifting=!shifting;
            refresh_shift_key();
            break;  
        }   
        //refreshPADTOUCHED=true;
      } else { // bars
        selected_rot=nkey-32;

        if (shifting || shiftR1) {
          if (selected_rot==1){
            ROTvalue[selected_sound][1]=min_values[1];
          }
          if (selected_rot==2){
            ROTvalue[selected_sound][2]=max_values[2];
          }
        }
        select_rot();
        drawBar(selected_rot);

      }
    }
  } 
}
