#include "WT2605C_Player.h"
#include <SoftwareSerial.h>

SoftwareSerial SSerial(0, 1); //use D2,D3 to simulate RX,TX
WT2605C<SoftwareSerial> Mp3Player;


#define COMSerial SSerial
#define ShowSerial Serial


void setup() {
  ShowSerial.print(1);
  ShowSerial.begin(9600);
  COMSerial.begin(115200);
  Mp3Player.init(COMSerial);
  ShowSerial.println("initial finished");

}

void loop() {
    
}
