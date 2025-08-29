#include <ArduinoJson.h>
#include <SD.h>

extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt);

extern byte   pattern[16][16];
extern String sound_names[];

extern uint32_t SAMvalue[16], ENDvalue[16];
extern int16_t  PITvalue[16];
extern uint8_t  RVSvalue[16], VOLvalue[16];
extern int8_t   PANvalue[16];
extern uint16_t FILvalue[16], BPMvalue[16];
extern uint8_t  TRPvalue[16], MFIvalue[16], OCTvalue[16], HPIvalue[16], SYNvalue[16], SCAvalue[16];

static int nextPatternNum(){
  if (!SD.exists("/patterns")) SD.mkdir("/patterns");
  File dir=SD.open("/patterns"); if (!dir) return 1;
  int maxN=0; while(true){ File f=dir.openNextFile(); if(!f) break;
    String nm=f.name(); if (nm.startsWith("/patterns/pattern_") && nm.endsWith(".json")){
      int n=nm.substring(18,nm.length()-5).toInt(); if (n>maxN) maxN=n;
    } f.close();
  } return maxN+1;
}

static void jsonWriteParams(JsonObject obj, int s){
  obj["SAM"]=SAMvalue[s]; obj["END"]=ENDvalue[s]; obj["PIT"]=PITvalue[s]; obj["RVS"]=RVSvalue[s];
  obj["VOL"]=VOLvalue[s]; obj["PAN"]=PANvalue[s]; obj["FIL"]=FILvalue[s]; obj["BPM"]=BPMvalue[s];
  obj["TRP"]=TRPvalue[s]; obj["MFI"]=MFIvalue[s]; obj["OCT"]=OCTvalue[s]; obj["HPI"]=HPIvalue[s]; obj["SYN"]=SYNvalue[s]; obj["SCA"]=SCAvalue[s];
}
static void jsonReadParams(JsonObject obj, int s){
  if(obj.containsKey("SAM")) SAMvalue[s]=(uint32_t)obj["SAM"];
  if(obj.containsKey("END")) ENDvalue[s]=(uint32_t)obj["END"];
  if(obj.containsKey("PIT")) PITvalue[s]=(int16_t)obj["PIT"];
  if(obj.containsKey("RVS")) RVSvalue[s]=(uint8_t)obj["RVS"];
  if(obj.containsKey("VOL")) VOLvalue[s]=(uint8_t)obj["VOL"];
  if(obj.containsKey("PAN")) PANvalue[s]=(int8_t)obj["PAN"];
  if(obj.containsKey("FIL")) FILvalue[s]=(uint16_t)obj["FIL"];
  if(obj.containsKey("BPM")) BPMvalue[s]=(uint16_t)obj["BPM"];
  if(obj.containsKey("TRP")) TRPvalue[s]=(uint8_t)obj["TRP"];
  if(obj.containsKey("MFI")) MFIvalue[s]=(uint8_t)obj["MFI"];
  if(obj.containsKey("OCT")) OCTvalue[s]=(uint8_t)obj["OCT"];
  if(obj.containsKey("HPI")) HPIvalue[s]=(uint8_t)obj["HPI"];
  if(obj.containsKey("SYN")) SYNvalue[s]=(uint8_t)obj["SYN"];
  if(obj.containsKey("SCA")) SCAvalue[s]=(uint8_t)obj["SCA"];
}

static void pattern_save_json(){
  if (!SD.exists("/patterns")) SD.mkdir("/patterns");
  String tmp = "/patterns/.tmp.json";
  String fin = "/patterns/pattern_"+String(nextPatternNum())+".json";
  File f=SD.open(tmp, FILE_WRITE); if (!f) return;

  DynamicJsonDocument doc(16384);
  JsonArray pads = doc.createNestedArray("pads");
  for(int s=0;s<16;s++){
    JsonObject po = pads.createNestedObject();
    po["name"]=sound_names[s];
    JsonObject pa = po.createNestedObject("params");
    jsonWriteParams(pa, s);
    JsonArray steps=po.createNestedArray("pattern");
    for(int st=0;st<16;st++) steps.add(pattern[s][st]);
  }
  serializeJson(doc,f); f.close(); SD.rename(tmp, fin);
}

static void pattern_new(){ for(int s=0;s<16;s++) for(int st=0;st<16;st++) pattern[s][st]=0; }

static bool pattern_load_first(){
  File dir=SD.open("/patterns"); if (!dir) return false;
  File f=dir.openNextFile(); if (!f) return false;
  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc,f)) { f.close(); return false; }
  JsonArray pads = doc["pads"];
  for(int s=0;s<min(16,(int)pads.size());s++){
    JsonObject po=pads[s];
    sound_names[s]=po["name"].as<String>();
    JsonObject pa = po["params"]; jsonReadParams(pa, s);
    JsonArray steps=po["pattern"];
    for(int st=0;st<min(16,(int)steps.size());st++) pattern[s][st]=steps[st];
  }
  f.close(); return true;
}

void openPatternView(){
  gfx->fillScreen(BLACK);
  drawTopBar("PATTERN", true);
  drawButtonBox(40,70,180,80, DARKGREY, "Save");
  drawButtonBox(240,70,180,80, DARKGREY, "New");
  drawButtonBox(440,70,180,80, DARKGREY, "Load");
  // Branches via DO_KEYPAD (boutons dédiés) ou dispatcher tactile si désiré
}
