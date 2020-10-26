#include <ESP32CAN.h>
#include <CAN_config.h>
#include "Classes.cpp"
#include "S60_02.cpp"
#include "BluetoothSerial.h"

CAN_device_t CAN_cfg;
const int rx_queue_size = 50;

BluetoothSerial SerialBT;

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
    for (int i = 0; i < 8; i++) {
        printf(",0x%02X", tx_frame.data.u8[i]);
      }
      printf("\n");
}

Profile *profiles[] = {
   new S60_02(&can_tx, printBT)
};
int PROFILE_COUNT = 1;
int currentProfile = 0;

void setup() {
  Serial.begin(115200);
  SerialBT.enableSSP();
  SerialBT.begin("Volve");
  SerialBT.setTimeout(100);

  CAN_cfg.speed = CAN_SPEED_125KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_5;
  CAN_cfg.rx_pin_id = GPIO_NUM_4;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  ESP32Can.CANInit();
}

void loop(){
  if (Serial.available()) {
    char test = Serial.read();
    if(test == '@'){
    SerialBT.println("notify-SKIP_SONG");
    }
    if(test == '!'){
    SerialBT.println("notify-PREVIOUS_SONG");
    }
  }
  String command;
  if (SerialBT.available()) {
    command = SerialBT.readString();
  }

  if(currentProfile == -1){
    if(command.startsWith("getProfiles")){
       for(int x = 0; x < PROFILE_COUNT; x++){
         SerialBT.print("getProfiles-"+profiles[x]->getName()+'\n');
       }
     }else{
        if(command.startsWith("profile")){
          currentProfile = command.substring(8,9).toInt();
          SerialBT.print("profile-OK"+'\n');
        }else{
          if(command != 0){
            SerialBT.print("NO_PROFILE"+'\n');
          }
        }
     }
  }
  else{
   if(command.startsWith("getCommands"))
      SerialBT.print("getCommands-"+profiles[0]->getCommands()+'\n');
      
   if(command.startsWith("getSettings"))
      SerialBT.print("getSettings-"+profiles[0]->getSettings()+'\n');

   if(command.startsWith("getData"))
      SerialBT.print("getData-"+profiles[0]->getData()+'\n');
      
   if(command.startsWith("command"))
      SerialBT.print("command-"+profiles[0]->command(command.substring(command.indexOf(' ')+1,command.indexOf('\n')))+'\n');

   if(command.startsWith("data"))
      SerialBT.print("data-"+profiles[0]->data(command.substring(command.indexOf(' ')+1,command.indexOf('\n')))+'\n');

   if(command.startsWith("print"))
      SerialBT.print("print-"+profiles[0]->print(command.substring(command.indexOf(' ')+1,command.indexOf('\n')))+'\n');
  }
can_RX();
}

void can_RX(){
  CAN_frame_t rx_frame;
  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    profiles[0]->can_rx(rx_frame.MsgID, rx_frame.data.u8);
  }
}

void printBT(String s){
  SerialBT.println(s);
}

void firstBoot(){

   //Vehicle *p = &vehicles[0];
  ///vehicles[0] = 0;
  
  String yee = profiles[0]->getCommands();
  
  Serial.println(profiles[0]->getName());
}
