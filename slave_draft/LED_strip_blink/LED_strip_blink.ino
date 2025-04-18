#include <Adafruit_AW9523.h>

Adafruit_AW9523 aw;

uint8_t LedPin = 1; 

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);  // wait for serial port to open
  
  Serial.println("Adafruit AW9523 GPIO Expander test!");

  
  if (! aw.begin(0x58)) {
    Serial.println("AW9523 not found? Check wiring!");
    while (1) delay(10);  // halt forever
  } 

  Serial.println("AW9523 found!");
  aw.pinMode(LedPin, OUTPUT);
}


void loop() {
  int delayBlink = 500;
  aw.digitalWrite(LedPin, HIGH);
  delay(delayBlink);
  aw.digitalWrite(LedPin, LOW);
  delay(delayBlink);
}