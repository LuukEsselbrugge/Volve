#include "Classes.cpp"

class S60_02 : public Profile {
  const String name = "Volvo S60 2.4 '02";
  const String availableCommands = "UNLOCK,LOCK,HIGHBEAM_ON,HIGHBEAM_OFF,LOWBEAM_ON,LOWBEAM_OFF,POP_TRUNK";
  const String availableData = "MILEAGE,BATTERY_VOLTAGE,FUEL_LEVEL,CABIN_TEMP";
  const String availableSettings = "LAZYINDICATOR,UNLOCK_LIGHT";

  private: int BATTERY_VOLTAGE = 12;
  private: int MILEAGE = 130000;
  private: int FUEL_LEVEL = 30;
  private: int CABIN_TEMP = 21;

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

  private:
      void (*can_tx)(int id, uint8_t d[8]);
      void (*printBT)(String s);
      
      void enablelcd(){
        //Enables lcd but also disabled media buttons needs to be done once
        uint8_t d[] = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0b00000101};
        can_tx(0x0220200E, d);
        //LCD stays on but media buttons work again
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
      
      void diag2_tx(int byte7){
        //Enable power command
        uint8_t d[] = {0xCE,0x43,0xB0,0x09,0x01,0x01,0x01,0x00};
        can_tx(0x0FFFFE, d);
        uint8_t d1[] = {0xCD,0x43,0xB0,0x10,0x01,0x03,0x00,byte7};
        can_tx(0x0FFFFE, d1);
      }
      
      void requestVoltage(){
        //byte 4 might be the requested value byte 5 might be how many u want //0x40 = CEM //A6 get data
        int id = 0x02;//Bat voltage
        uint8_t d[] = {0xCD, 0x40, 0xA6, 0x1A, id, 0x01, 0x00, 0x00};
        can_tx(0x0FFFFE, d);
      }
      
      void sweep(){
        uint8_t d[] = {0xCB, 0x51, 0xB2, 0x02, 0x00, 0x00, 0x00, 0x00};
        can_tx(0x0FFFFE, d);
      }
      
      void disablelcd(){
        uint8_t d[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0b000100};
        this->can_tx(0x0220200E, d);
      }

  
  public:
    S60_02(void (*f)(int, uint8_t*), void (*e)(String)){
      this->can_tx = f;
      this->printBT = e;

      //this->enablelcd();
//      th/is->lcd("CANe~V1         luuk.cc",23);
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
      Serial.println(key);
      if(key=="UNLOCK"){
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

      if(key=="POP_TRUNK"){
        uint8_t d[] = {0xCF,0x46,0xB1,0x6F,0x51,0x01,0x02,0x04};
        uint8_t d2[] = {0xCD,0x46,0xB1,0x6F,0x51,0x00,0x00,0x00};
        can_tx(0x0FFFFE, d);
        can_tx(0x0FFFFE, d2);
        return "OK";
      }
      return "NOT_FOUND";
    }

    String data(String key) {
      Serial.println(key);
      if(key=="BATTERY_VOLTAGE"){
        return String(BATTERY_VOLTAGE, DEC);
      }
      if(key=="MILEAGE"){
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
      if(id == 0x0FFFFE){
      printf("0x%08X",id);
      for (int i = 0; i < 8; i++) { 
        printf(",0x%02X", data[i]);
      }
      printf(",%lu",millis());
      printf("\n");     
     }
    }

    String print(String value) {
      Serial.println(value);
      return "OK";
    }
};
