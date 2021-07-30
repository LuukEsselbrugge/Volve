#include <ESP32CAN.h>
#include <CAN_config.h>
#include <HardwareSerial.h>
#include "Classes.cpp"
#include "S60_02.cpp"
#include "BluetoothSerial.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "driver/periph_ctrl.h"

HardwareSerial RTI(2);

CAN_device_t CAN_cfg;
const int rx_queue_size = 50;

//Volvo RTI Display;
const char brightness_levels[] = {0x20, 0x61, 0x62, 0x23, 0x64, 0x25, 0x26, 0x67, 0x68, 0x29, 0x2A, 0x2C, 0x6B, 0x6D, 0x6E, 0x2F};

int current_display_mode = 0x46;
char current_brightness_level = 15;

//BluetoothSerial SerialBT;

void can_tx(int id, uint8_t d[8]){
   CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_ext;
    tx_frame.MsgID = id;
    tx_frame.FIR.B.DLC = 8;
    tx_frame.data.u8[0] = d[0];
    tx_frame.data.u8[1] = d[1];
    tx_frame.data.u8[2] = d[2];
    tx_frame.data.u8[3] = d[3];
    tx_frame.data.u8[4] = d[4];
    tx_frame.data.u8[5] = d[5];
    tx_frame.data.u8[6] = d[6];
    tx_frame.data.u8[7] = d[7];
    ESP32Can.CANWriteFrame(&tx_frame);
    //Needs delay between because DIM no hablo fast
    delay(50);
   // for (int i = 0; i < 8; i++) {
     //   printf(",0x%02X", tx_frame.data.u8[i]);
      //}
      //printf("\n");
}

Profile *profiles[] = {
   new S60_02(&can_tx, &printAll, &updateDisplay)
};
int PROFILE_COUNT = 1;
int currentProfile = 0;

void setup() {
  if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0){
    esp_sleep_enable_timer_wakeup(100);
    esp_deep_sleep_start();
  }
  //Disable brownout detection because PI boot draw randomly crashes the ESP32
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  pinMode(GPIO_NUM_2, OUTPUT);
  digitalWrite(GPIO_NUM_2,HIGH);
  //delay(2000);
  Serial.begin(4800);
  RTI.begin(2400);
  //SerialBT.enableSSP();
  //SerialBT.begin("Volve");
  //SerialBT.setTimeout(100);

  CAN_cfg.speed = CAN_SPEED_125KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_5;
  CAN_cfg.rx_pin_id = GPIO_NUM_4;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  ESP32Can.CANInit();
  
  //Turn on the PI
  pinMode(GPIO_NUM_23, OUTPUT);
  digitalWrite(GPIO_NUM_23,HIGH);

  //Gpio for video switch
  pinMode(GPIO_NUM_27, OUTPUT);

  
//  uint8_t d[] = {0xCE, 0x/51, 0xB0, 0x0B, 0x01, 0xFF, 0x20, 0x00};
//  can_tx(0x0FFFFE, d);/
  
  profiles[0]->setupDone();
}

void loop(){
  String command;
  if (Serial.available()) {
    command = Serial.readString();
  }
  //if (SerialBT.available()) {
   // command = SerialBT.readString();
  //}

  if(currentProfile == -1){
    if(command.startsWith("getProfiles")){
       for(int x = 0; x < PROFILE_COUNT; x++){
         printAll("getProfiles-"+profiles[x]->getName()+'\n');
       }
     }else{
        if(command.startsWith("profile")){
          currentProfile = command.substring(8,9).toInt();
          printAll("profile-OK"+'\n');
        }else{
          if(command != 0){
            printAll("NO_PROFILE"+'\n');
          }
        }
     }
  }
  else{
   if(command.startsWith("getCommands"))
      printAll("getCommands-"+profiles[0]->getCommands()+'\n');
      
   if(command.startsWith("getSettings"))
      printAll("getSettings-"+profiles[0]->getSettings()+'\n');

   if(command.startsWith("getData"))
      printAll("getData-"+profiles[0]->getData()+'\n');
      
   if(command.startsWith("command"))
      printAll("command-"+profiles[0]->command(command.substring(command.indexOf(' ')+1,command.indexOf('\n')))+'\n');

   if(command.startsWith("data"))
      printAll("data-"+profiles[0]->data(command.substring(command.indexOf(' ')+1,command.indexOf('\n')))+'\n');

   if(command.startsWith("print"))
      printAll("print-"+profiles[0]->print(command.substring(command.indexOf(' ')+1,command.indexOf('\n')))+'\n');
   
   if(command.startsWith("PI_ON")){
      digitalWrite(GPIO_NUM_23, HIGH);
      printAll("PI_ON");
   }
   if(command.startsWith("PI_OFF")){
      digitalWrite(GPIO_NUM_23, LOW);
      printAll("PI_OFF");
   }
  }
  can_RX();

 //Send data to RTI Volvo Display

 rtiWrite();
 
}

unsigned long lastCanRX = millis();
void can_RX(){
  CAN_frame_t rx_frame;
  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    profiles[0]->can_rx(rx_frame.MsgID, rx_frame.data.u8);
    lastCanRX = millis();
  }
  
 //Go into deep sleep if no CAN frame recieved for 1 minute
 //Also turns off PI
 //lastCanRX = millis();
 if ((millis()-lastCanRX) > 30000){
    //Lower display
    current_display_mode = 0x46;
    //Notify the Raspberry PI we are going into deep sleep
    printAll("EVENT_SHUTDOWN\n");
    //Wait for PI to shutdown
    delay(30000);
    //Disable PI power
    digitalWrite(GPIO_NUM_23, LOW);
    //Sleep tight esp
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4,0);
    esp_deep_sleep_start();
 }
}

void printAll(String s){
  //SerialBT.print(s);
  Serial.print(s);
}

void updateDisplay(int m, int b){
  if(m != 0){
    current_display_mode = m;
  }
  current_brightness_level = b;
}

unsigned long lastRtiMsg = millis();
int part = 0;
void rtiWrite() {
   if ((millis()-lastRtiMsg) > 250){
    if(part==0){
     RTI.write(current_display_mode);
     //RTI.write(0x40);
    }
    if(part==1){
     RTI.write(brightness_levels[current_brightness_level]);
     //RTI.write(0x2F);
    }
    if(part==2){ 
     RTI.write(0x83);
     part=-1;
    }
     part++;
     lastRtiMsg = millis();
   }
}

void firstBoot(){

   //Vehicle *p = &vehicles[0];
  ///vehicles[0] = 0;
  
  String yee = profiles[0]->getCommands();
  
  Serial.println(profiles[0]->getName());
}
