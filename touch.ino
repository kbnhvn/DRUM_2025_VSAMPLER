// Reset GT911
void resetGT911() {
    pinMode(TOUCH_RST, OUTPUT);
    pinMode(TOUCH_INT, OUTPUT);
    digitalWrite(TOUCH_INT, LOW);
    delay(5);
    digitalWrite(TOUCH_RST, LOW);
    delay(10);
    digitalWrite(TOUCH_RST, HIGH);
    delay(50);
    pinMode(TOUCH_INT, INPUT);
    delay(50);
}

// Read register GT911
uint8_t readRegister8(uint16_t reg) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    Wire.endTransmission(false);  
    Wire.requestFrom(GT911_ADDR, 1);
    return Wire.available() ? Wire.read() : 0;
}

// Read registers GT911
void readRegisterMultiple(uint16_t reg, uint8_t *buffer, uint8_t length) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    Wire.endTransmission(false);  
    Wire.requestFrom((uint8_t)GT911_ADDR, (uint8_t)length);
    for (uint8_t i = 0; i < length && Wire.available(); i++) {
        buffer[i] = Wire.read();
    }
}

// Write register GT911
void writeRegister8(uint16_t reg, uint8_t value) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    Wire.write(value);
    Wire.endTransmission();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void read_touch(){
    
  uint8_t status = readRegister8(0x814E); // Estado de toque

  if (status & 0x80) { // Si hay un toque detectado
    if (!touchActivo){
      touchActivo = true; 
      uint8_t data[8] = {0};
      readRegisterMultiple(0x8150, data, 8); //read

      uint16_t x = (data[1] << 8) | data[0];
      uint16_t y = (data[3] << 8) | data[2];
      uint8_t size = data[4];
      writeRegister8(0x814E, 0x00); // clear
      cox=x;
      coy=y;

      for (byte f=0;f<48;f++){
        if ( (cox > BPOS[f][0]) && (cox < (BPOS[f][0]+BPOS[f][2])) && (coy > BPOS[f][1]) && (coy < (BPOS[f][1]+BPOS[f][3])) ) {
          if (f==last_touched ){
            if (start_debounce+debounce_time > millis() ){
              break;
            } 
          }
          trigger_on[f]=1;
          last_touched=f;
          start_debounce=millis();
          touchActivo = true;
          // Serial.print("t ");
          // Serial.println(f);
          break;
        }
      }
    }
  } else {
    touchActivo = false;
  }
  writeRegister8(0x814E, 0x00); // clear
}