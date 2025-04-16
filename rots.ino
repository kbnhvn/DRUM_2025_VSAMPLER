
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



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Función para iniciar la lectura ADC sin bloquear
void startADC() {
    ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, false); // Canal 0, no bloqueante
    ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_1, false); // Canal 1, no bloqueante
    ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_2, false); // Canal 2, no bloqueante
    ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_3, false); // Canal 3, no bloqueante
}

// Función para leer el resultado si está listo
void checkADC() {
  if (ads.conversionComplete()) {
    adcValue0 = ads.getLastConversionResults();
    startADC(); // Iniciar nueva lectura inmediatamente
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void READ_POTS(){
  // Leer del ADS1015
  // adc0 = map(ads.readADC_SingleEnded(0),0,1090,127,0); // MVOL
  // tmp_adc1 = map(ads.readADC_SingleEnded(1),0,1090,12,0); // FX1

  adc0 = map(ads.readADC_SingleEnded(0),0,1090,127,0); // 
  adc1 = map(ads.readADC_SingleEnded(1),0,1090,12,0); // FX1
  adc2 = map(ads.readADC_SingleEnded(2), 0, 1090, 127, 0); // mvol
  adc3 = map(ads.readADC_SingleEnded(3), 0, 1090, 127, 0); // mfil  

  if (adc0!=old_adc0){
    old_adc0=adc0;
    //Serial.print("0 ");
    //Serial.println(adc0);

  }
  if (adc1!=old_adc1){
    old_adc1=adc1;
    //Serial.print("1 ");
    //Serial.println(adc1);
    if (adc1>6) {
      fx1=3;
    } else if (adc1>3) {
      fx1=2;
    } else if (adc1>1) {
      fx1=1;
    } else {
      fx1=0;
    }
  }

  if (adc2!=old_adc2){
    old_adc2=adc2;
    //Serial.print("2 ");
    //Serial.println(adc2);
    master_vol=adc2;
    synthESP32_setMVol(master_vol);
    drawBar(9);    
  }
  
  if (adc3!=old_adc3){
    old_adc3=adc3;
    //Serial.print("3 ");
    //Serial.println(adc3);
    master_filter=adc3;
    synthESP32_setMFilter(master_filter);
    drawBar(11);
  }

}