#ifndef PROFILE
#define PROFILE

#include <Arduino.h>
class Profile {
  public:
     virtual String getCommands() {}
     virtual String getData() {}
     virtual String getName() {}
     virtual String getScreens() {}
     virtual String setScreens() {}
     virtual String getSettings() {}

     virtual String command(String key) {}
     virtual String data(String key) {}
     virtual String setting(String key, String value) {}
     virtual String print(String value) {}

     virtual void can_rx(int id, uint8_t* data) {}

     virtual void setupDone() {}
     
     ~Profile() = default;
};
#endif
