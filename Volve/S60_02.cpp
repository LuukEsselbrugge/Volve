#include "Classes.cpp"

class S60_02 : public Profile {
  const String name = "Volvo S60 2.4 '02";
  const String availableCommands = "UNLOCK,LOCK,HIGHBEAM_ON,HIGHBEAM_OFF,LOWBEAM_ON,LOWBEAM_OFF,POP_TRUNK,DISPLAY_UP,DISPLAY_DOWN,WINDOWS_DOWN,WINDOWS_UP";
  const String availableData = "MILEAGE,BATTERY_VOLTAGE,FUEL_LEVEL,CABIN_TEMP,BRIGHTNESS";
  const String availableSettings = "LAZYINDICATOR,UNLOCK_LIGHT";
  
  const int SHORT_PRESS_TIME = 500;

  private: int BATTERY_VOLTAGE = 0;
  private: int MILEAGE = 0;
  private: int FUEL_LEVEL = 23;
  private: int CABIN_TEMP = 3;
  private: int SPEED = 0;
  private: int RPM = 0;
  private: int BRIGHTNESS = 0;
  private: int REVERSE = 0;
  private: int REVERSE_TIMEOUT = 0;

  private: int keep_display_up = 0;
  private: int old_ring_value = 0;
  private: int current_ring_value = 0;
  private: int LAST_DIM_UPDATE = 0;

  private: int display_info = 0;

  private: int mediapressed = 0;
  private: int controlmedia = 0;
  private: unsigned long pressedTime = 0;
  private: unsigned long releasedTime = 0;
  private: int lastmediacontrols = 0;

  private: unsigned long lockpressedtime = 0;
  private: int lockpressedamount = 0;
  private: unsigned long ulockpressedtime = 0;
  private: int ulockpressedamount = 0;

  //Lazy indicators
  private: int lazylefttime = 0;
  private: int lazyrighttime = 0;
  
  //byte 7 diag1 commands
  const int WINDOW_REAR_R_OPEN = 0b00000001;
  const int LOW_BEAM_ON =  0b00001000;
  const int HIGH_BEAM_ON=  0b00000100;
  const int TAIL_LIGHTS_ON=0b00100000;
  const int FOG_LIGHTS_ON = 0b01000000;
  const int WIPER_ON = 0b10000000;
  //byte 6 diag1 commands
  const int WINDOW_REAR_L_CLOSE = 0b00100000;
  const int WINDOW_REAR_L_OPEN =  0b01000000;
  const int WINDOW_REAR_R_CLOSE = 0b10000000;
  
  const char deg = (char)0x41;
  //diag2 commands
  const int WINDOW_L_CLOSE = 0b00000001;
  const int WINDOW_L_OPEN = 0b00000010;
  const int WINDOW_R_CLOSE = 0b00001000;
  const int WINDOW_R_OPEN = 0b00010000;

  //Adresses modules
  const int SWM = 0x00400066;
  const int SWM_2 = 0x0261300A;
  const int CCM = 0x04000002;
  const int DIAG = 0x0FFFFE;
  const int REM = 0x0280142A;

  private:
      void (*can_tx)(int id, uint8_t d[8]);
      void (*printBT)(String s);
      void (*updateDisplay)(int m, int b);
      
      void enablelcd(){
        //Enables lcd but also disabled media buttons needs to be done once
        uint8_t d[] = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0b00000101};
        can_tx(0x0220200E, d);
        //LCD stays on but media buttons work again magic
        uint8_t d2[] = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0b00000000};
        can_tx(0x0220200E, d2);
      }

      void clearlcd(){
        uint8_t d[] = {0xE1, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        can_tx(0x00C00008, d);
      }
      
      void updatelcd(char input[], int len, int pos){
        char str[8] = {0};
        memcpy(str, input, len);
        uint8_t d1[] = {0xE0+len, pos, str[0], str[1], str[2], str[3], str[4], str[5]};
        can_tx(0x00C00008, d1);
      }
      
      void lcd(char input[], int len){
        char str[34] = {0};
        std::fill_n(str, 34, 0x20);
        memcpy(str, input, len);
        int block = 0;
        for(int i = 0; i < 32; i+=6)
         {
            if(block == 0){
                uint8_t d1[] = {0xA7, 0x00, str[i], str[i+1], str[i+2], str[i+3], str[i+4], str[i+5]};
                can_tx(0x00C00008, d1);
              }else{
                if(block == 4){
                  uint8_t d4[] = {0x67, str[i], str[i+1], str[i+2], str[i+3], str[i+4], str[i+5], str[i+6]};
                  can_tx(0x00C00008, d4);
                }else{
                  if(block != 5){
                    uint8_t d3[] = {0x20+block, str[i], str[i+1], str[i+2], str[i+3], str[i+4], str[i+5], str[i+6]};
                    can_tx(0x00C00008, d3);
                    i++;
                  }
                }
              }
              block++;
         }
      }
      //Diag activations for CEM
      void diag1_tx(int byte6, int byte7){
        uint8_t d[] = {0x8F, 0x40, 0xB1, 0x1A, 0x21, 0x01, byte6, byte7};
        can_tx(0x0FFFFE, d);
        uint8_t d1[] = {0x4E, 0x00, 0x00, byte6, byte7, 0x00, 0x00, 0x00};
        can_tx(0x0FFFFE, d1);
      
        if(byte6 == 0 && byte7 == 0){
          uint8_t d[] = {0xCD, 0x40, 0xB1, 0x1A, 0x21, 0x00, 0x00, 0x00};
          can_tx(0x0FFFFE, d);
        }
      }
      //Diag activations for Driver Door Module
      void diag2_tx(int byte7){
        //Enable power command
        uint8_t d[] = {0xCE,0x43,0xB0,0x09,0x01,0x01,0x01,0x00};
        can_tx(0x0FFFFE, d);
        uint8_t d1[] = {0xCD,0x43,0xB0,0x10,0x01,0x03,0x00,byte7};
        can_tx(0x0FFFFE, d1);
      }
      
      void sweep(){
        uint8_t d[] = {0xCB, 0x51, 0xB2, 0x02, 0x00, 0x00, 0x00, 0x00};
        can_tx(0x0FFFFE, d);
      }

      void gong_on(){
        uint8_t d[] = {0xCE, 0x51, 0xB0, 0x0B, 0x01, 0xFF, 0x20, 0x00};
        can_tx(0x0FFFFE, d);
      }

      void gong_off(){
        uint8_t d[] = {0xCE, 0x51, 0xB0, 0x0B, 0x00, 0x00, 0x00, 0x00};
        can_tx(0x0FFFFE, d);
      }

      void indicate_left(){
        uint8_t d[] = {0xCE, 0x48, 0xB0, 0x05, 0x01, 0x03, 0x02, 0x00};
        can_tx(0x0FFFFE, d);
      }

      void indicate_right(){
        uint8_t d[] = {0xCE, 0x48, 0xB0, 0x05, 0x01, 0x03, 0x01, 0x00};
        can_tx(0x0FFFFE, d);
      }

      void indicate_stop(){
        uint8_t d[] = {0xCE, 0x48, 0xB0, 0x05, 0x00, 0x00, 0x00, 0x00};
        can_tx(0x0FFFFE, d);
      }
      
      void disablelcd(){
        uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0b000100};
        this->can_tx(0x0220200E, d);
      }

  
  public:
    S60_02(void (*f)(int, uint8_t*), void (*e)(String), void (*d)(int, int)){
      this->can_tx = f;
      this->printBT = e;
      this->updateDisplay = d;
      
    }
    
    String getCommands() {
      return availableCommands;
    }
    String getSettings() {
      return availableSettings;
    }
    String getData() {
      return availableData;
    }
    String getName()  {
      return name;
    }
    void unlock() {  }

    String command(String key) {
      if(key=="UNLOCK"){
        uint8_t d[] = {0x80,0x13,0x00,0xA0,0x37,0x00,0x1D,0x00};
        can_tx(0x0280142A, d);
        return "OK";
      }
      if(key=="LOCK"){
        uint8_t d[] = {0x80,0x13,0x00,0xB0,0x37,0x00,0x1D,0x00};
        can_tx(0x0280142A, d);
        return "OK";
      }
      if(key=="LOWBEAM_ON"){
        this->diag1_tx(0x00, LOW_BEAM_ON);
        return "OK";
      }
       if(key=="LOWBEAM_OFF"){
        this->diag1_tx(0x00, 0x00);
        return "OK";
      }
      if(key=="HIGHBEAM_ON"){
        this->diag1_tx(0x00, HIGH_BEAM_ON);
        return "OK";
      }
       if(key=="HIGHBEAM_OFF"){
        this->diag1_tx(0x00, 0x00);
        return "OK";
      }
      if(key=="WNDOWS_DOWN"){
        this->diag1_tx(WINDOW_REAR_L_OPEN,WINDOW_REAR_R_OPEN);
        this->diag2_tx(WINDOW_L_OPEN | WINDOW_R_OPEN);
        return "OK";
      }
      if(key=="WNDOWS_UP"){
        this->diag1_tx(WINDOW_REAR_L_CLOSE,0x00);
        this->diag1_tx(WINDOW_REAR_R_CLOSE,0x00);
        this->diag2_tx(WINDOW_L_CLOSE | WINDOW_R_CLOSE);
        return "OK";
      }

      if(key=="POP_TRUNK"){
        //0x46 = REM ID
        uint8_t d[] = {0xCF,0x46,0xB1,0x6F,0x51,0x01,0x02,0x04};
        uint8_t d2[] = {0xCD,0x46,0xB1,0x6F,0x51,0x00,0x00,0x00};
        can_tx(0x0FFFFE, d);
        can_tx(0x0FFFFE, d2);
        return "OK";
      }

      if(key.startsWith("DISPLAY_UP")){
        this->updateDisplay(0x40,15);
        keep_display_up = 1;
        return "OK";
      }
      if(key.startsWith("DISPLAY_DOWN")){
        this->updateDisplay(0x46,15);
        keep_display_up = 0;
        return "OK";
      }
      return "NOT_FOUND";
    }

    String data(String key) {
      Serial.println(key);
      if(key=="BATTERY_VOLTAGE"){
        //byte 4 requested value byte 5 might be how many u want //0x40 = CEM //A6 get data
        uint8_t d[] = {0xCD, 0x40, 0xA6, 0x1A, 0x02, 0x01, 0x00, 0x00};
        can_tx(0x0FFFFE, d);
        return String(BATTERY_VOLTAGE, DEC);
      }
      if(key=="MILEAGE"){
        uint8_t d[] = {0xCD,0x40,0xA6,0x1A,0x11,0x01,0x00,0x00};
        can_tx(0x0FFFFE, d);
        return String(MILEAGE, DEC);
      }
      if(key=="FUEL_LEVEL"){
        return String(FUEL_LEVEL, DEC);
      }
      if(key=="CABIN_TEMP"){
        return String(CABIN_TEMP, DEC);
      }
      return "NOT_FOUND";
    }

    //CAN data input
    void can_rx(int id, uint8_t* data){
     // if(id == SWM_2 ){
        // printf(",0x%02X", data[7]);
       //  printf("\n");
        //printf("0x%08X",id);
//        for (int i = 0; i < 8; i++) { 
//          printf(",0x%02X", data[i]);
//        }
       // printf(",%lu",millis());
        //printf("\n");  
    //  }   

     //Diag request responds
     if(id == 0x00800003){
         for (int i = 0; i < 8; i++) { 
            printf(",0x%02X", data[i]);
          }
          printf(",%lu",millis());
          printf("\n");   
        //Battery voltage request responds 0x40 = CEM, 0x02 = battery request
        if(data[1] == 0x40 && data[4] == 0x02){
          this->BATTERY_VOLTAGE = data[5] / 0.08;
        }
        //Milage request responds 0x40 = CEM, 0x11 = mileage request
        if(data[1] == 0x40 && data[4] == 0x11){
          this->MILEAGE = (data[5]<<24) | (data[6]<<16)| (data[7]<<8);
        }
     }

     if(id == SWM_2){
       int rst_btn = (data[4] >> 4) & 0x0F;
       current_ring_value = data[7];
       if(current_ring_value != old_ring_value){
         display_info = 0;
       }
       //reset pressed
       if(rst_btn == 0xC && display_info == 0){
          display_info = 1;
          printf("press");
          old_ring_value = data[7];
          this->print("--- KM/H        ---- RPM      ");
       }
     }
     
     if(id == REM){
        //Lock pressed on remote
        //TODO Add settings option in app to specify what windows to operate
        if(data[3] >> 4 == 0xB){          
          if((millis()-lockpressedtime) < 2000){
            lockpressedamount+=1;
          }else{
            lockpressedamount = 0;
          }
          lockpressedtime = millis();
          
          if(lockpressedamount == 3){
            diag1_tx(WINDOW_REAR_L_CLOSE,0x00);
            diag1_tx(WINDOW_REAR_R_CLOSE,0x00);
            diag2_tx(WINDOW_L_CLOSE | WINDOW_R_CLOSE);
            lockpressedamount=0;
          }
        }
        //Unlocked pressed on remote
        //TODO add settings for both options
        if(data[3] >> 4 == 0xA){
          diag1_tx(0x00,TAIL_LIGHTS_ON | LOW_BEAM_ON);
          
          if((millis()-ulockpressedtime) < 2000){
            ulockpressedamount+=1;
          }else{
            ulockpressedamount=0;
          }
          ulockpressedtime = millis();
          
          if(ulockpressedamount == 3){
            diag1_tx(WINDOW_REAR_L_OPEN,WINDOW_REAR_R_OPEN);
            diag2_tx(WINDOW_L_OPEN | WINDOW_R_OPEN);
            ulockpressedamount=0;
          }
        }
     }

     if(id == SWM){
      int mediacontrols = data[7];
      int cruisecontrols = data[5];
      //Frame to tell radio phone is on spoof radio into no buttons pressed
      uint8_t radio_overwrite[] = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0b000001};
      uint8_t radio_normal[] = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0b000000};

      //Vol up and cruise up pressed raise screen
      if(mediacontrols == 0x77 && cruisecontrols == 0x42){
         this->updateDisplay(0x40,15);
         keep_display_up = 1;
      }
      //Vol down and cruise down pressed raise screen
      if(mediacontrols == 0x7B && cruisecontrols == 0x44){
         this->updateDisplay(0x46,15);
         keep_display_up = 0;
      }
      
      //Volume up and skip pressed same time
      if(mediacontrols == 0x75){
        lastmediacontrols = mediacontrols;
        mediapressed = 1;
        controlmedia = !controlmedia;
        if(controlmedia){
          can_tx(0x0220200E, radio_overwrite);
        }else{
          can_tx(0x0220200E, radio_normal);
        }
      }
      //Media button pressed
      if(mediacontrols != 0x7F && mediapressed == 0 && controlmedia == 1){
        pressedTime = millis();
        mediapressed = 1;
        lastmediacontrols = mediacontrols;
      }
      //All media buttons released
      if(mediacontrols == 0x7F && mediapressed == 1 && controlmedia == 1){
        mediapressed = 0;
        releasedTime = millis();
        //Total button press time
        long pressDuration = releasedTime - pressedTime;

        if(pressDuration < SHORT_PRESS_TIME){
          //Prev pressed
          if(lastmediacontrols == 0x7E)
            this->printBT("EVENT_LEFT\n");
            
          //Forward pressed
          if(lastmediacontrols == 0x7D)
            this->printBT("EVENT_RIGHT\n");

          //Volume up pressed
          if(lastmediacontrols == 0x77)
            this->printBT("EVENT_ENTER\n");

          //Volume down pressed
          if(lastmediacontrols == 0x7B)
            this->printBT("EVENT_BACK\n"); 
        }else{
          //Long forward pressed
          if(lastmediacontrols == 0x7D)
            this->printBT("EVENT_UP\n");

          //Long prev pressed
          if(lastmediacontrols == 0x7E)
            this->printBT("EVENT_DOWN\n");
        }
      }
     }

     if(id == CCM){
      CABIN_TEMP = data[7] / 0.04;
     }

     if(id == 0x02e10df4){
      BRIGHTNESS = data[7] - 0x10;
      //Brightness level is between 0-13 add 2 to make it work with RTI
      this->updateDisplay(0,BRIGHTNESS+2);
     }

     if(id == 0x01213FFC){
      int A = data[6];
      A &= 0b11;
      SPEED = (A << 8 | data[7]) / 4;
      REVERSE = (data[2] >> 5) & 1;
      //Serial.println(REVERSE);
      if(REVERSE){
        this->updateDisplay(0x45,15);
        REVERSE_TIMEOUT = millis() + 10000;
      }else{
        //Make sure display was not enabled by someone else and timer has expired
        if(millis() > REVERSE_TIMEOUT){
          if(!keep_display_up){
            this->updateDisplay(0x46,15);
          }else{
            //Switch back to Raspberry PI (VGA input)s
            this->updateDisplay(0x40,15);
          }
        }
      }
     }

     if(id == 0x02C13428){
      int A = data[6];
      A &= 0b1111;
      RPM = A << 8 | data[7];
     }

     if(display_info && (millis() - LAST_DIM_UPDATE) > 250){
      char spd[3];
      if( ((SPEED/100)%10) == 0){
        spd[0] = ' ';
      }else{
         spd[0] = '0'+(SPEED/100)%10;
      }
      spd[1] = '0'+(SPEED/10)%10;
      spd[2] = '0'+(SPEED % 10);
      updatelcd(spd,3,0);

      char rpm[4];
      rpm[0] = '0'+(RPM/1000);
      rpm[1] = '0'+((RPM/100)%10);
      rpm[2] = '0'+((RPM/10)%10);
      rpm[3] = '0'+(RPM%10);
      updatelcd(rpm,4,16);

      LAST_DIM_UPDATE = millis();
     }
     
    }

    String print(String value) {
      Serial.println(value);
      char copy[31];
      value.toCharArray(copy, 31);
      this->enablelcd();
      this->lcd(copy,31);
      return "OK";
    }

    void setupDone(){
      this->print("Volve~V1        luuk.cc       ");
      //gong_on();
      //delay(500);
      //gong_off();
      
    }
};
