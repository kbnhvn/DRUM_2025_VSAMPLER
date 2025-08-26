int counter1 = 0;
int old_counter1 = 0;
bool shiftR1 = false;

void select_rot(){
if (selected_rot==11){
    counter1=master_filter; 
  } else if (selected_rot==12){
    counter1=octave;      
  } else if (selected_rot==8){
    counter1=bpm;
  } else if (selected_rot==9){
    counter1=master_vol;
  } else if (selected_rot==10){
    ztranspose=0;
    counter1=ztranspose;
  } else if (selected_rot==13){
    zmpitch=0;
    counter1=zmpitch;                
  } else if (selected_rot==14){
    counter1=sync_state; 
  } else if (selected_rot==15){    
    counter1=selected_scale;
  } else {
    counter1=ROTvalue[selected_sound][selected_rot];
  }
}

//////////////////////////////////////////////////////////////////////////////
void do_rot(){

  if (counter1>max_values[selected_rot]) counter1=max_values[selected_rot];
  if (counter1<min_values[selected_rot]) counter1=min_values[selected_rot];

  if (counter1!=old_counter1){
    
    if (selected_rot==11){
      master_filter=counter1;
      synthESP32_setMFilter(master_filter);
    } else if (selected_rot==12){
      octave=counter1;
    } else if (selected_rot==8){
      bpm=counter1;
      uClock.setTempo(bpm);
      //setBPM(bpm);       
    } else if (selected_rot==9){
      master_vol=counter1;
      synthESP32_setMVol(master_vol);
    } else if (selected_rot==10){
      ztranspose=counter1;
      // cambiar pitch en melodic
      for (byte f=0; f<16;f++){
        melodic[selected_sound][f]=melodic[selected_sound][f]+ztranspose;
      }
      if (modeZ==tWrite) refreshPATTERN=true;
      ztranspose=0;
      counter1=ztranspose;
    } else if (selected_rot==13){
      zmpitch=counter1;
      // cambiar pitch en melodic
      for (byte k=0; k<16;k++){  
        ROTvalue[k][3]=ROTvalue[k][3]+zmpitch;  
        for (byte f=0; f<16;f++){
          melodic[k][f]=melodic[k][f]+zmpitch;
        } 
      }
      if (modeZ==tWrite) refreshPATTERN=true;
      zmpitch=0;
      counter1=zmpitch;
    } else if (selected_rot==14){
      sync_state=counter1; 
    } else if (selected_rot==15){
      selected_scale=counter1;
    } else {       
      if (selected_rot==1) {
          if (counter1 > ROTvalue[selected_sound][2]) counter1=ROTvalue[selected_sound][2];
      }
      if (selected_rot==2) {
          if (counter1 < ROTvalue[selected_sound][1]) counter1=ROTvalue[selected_sound][1];
      }
      ROTvalue[selected_sound][selected_rot]=counter1;
      setSound(selected_sound);

      // play sound if
      if (selected_rot<8 && !playing){
        synthESP32_TRIGGER(selected_sound);
      }
    }

    drawBar(selected_rot);

  }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
void READ_ENCODERS(){

  bitWrite(ENC_PORT1,0,!digitalRead(DT));
  bitWrite(ENC_PORT1,1,!digitalRead(CLK));
  old_counter1=counter1;
  counter1=counter1+read_encoder1();
  do_rot();  
}

//////////////////////////////////////////////////////////////////////////////////////////////
int16_t read_encoder1() {
  
  // Select rotary reading mode
  //static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0}; // Si dejo esto cuenta 4 pasos cada click
  static int8_t enc_states[] =  {0,-1,0,0,1,0,0, 0, 0,0,0,0,0,0, 0,0};  // Si dejo esto cuenta 1 paso  cada click

  static uint8_t old_AB1;
  old_AB1 <<= 2; //remember previous state
  old_AB1 |= ( ENC_PORT1 & 0x03 ); //add current state
  int8_t result=enc_states[( old_AB1 & 0x0f )];
  if (result>0) return (result +(9*shiftR1) ); 
  if (result<0) return (result -(9*shiftR1) ); 
  return 0;
}