#ifndef MYLIBRARY_H
#define MYLIBRARY_H

#include <Arduino.h>

enum Mode { OFF, ON };

struct Device {
  int pin;
  Mode mode;
};

class MyLibrary {
  public:
    MyLibrary(int pin);  // Constructor
    void setup();        // Setup function
    void loop();         // Loop function
    
  private:
    Device myDevice;
};

#endif // MYLIBRARY_H
