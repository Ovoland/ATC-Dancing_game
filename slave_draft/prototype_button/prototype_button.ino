/*
  Button file (adapted to test our own button)

  Turns on and off a light emitting diode(LED) connected to digital pin 13,
  when pressing a pushbutton attached to pin 2.

  The circuit:
  - LED attached from pin 13 to ground through 220 ohm resistor
  - Button LED attached from pin 2 to ground through 220 ohm resistor
  - pushbutton attached to pin 2 from +5V
  - 10K resistor attached to pin 2 from ground

  - Note: on most Arduinos there is already an LED on the board
    attached to pin 13.

  created 2005
  by DojoDave <http://www.0j0.org>
  modified 30 Aug 2011
  by Tom Igoe

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Button
*/

// constants won't change. They're used here to set pin numbers:
const int buttonPin = 7;  // the number of the pushbutton pin
const int ledPin = 1;    // the number of the LED pin
const int ledButton = 6;    // the number of the LED pin connected to the red zone (black zone to ground)

// variables will change:
int buttonState = 0;  // variable for reading the pushbutton status

void setup() {
  initPin();
}

void loop() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == LOW) {
    // turn LED on:
    digitalWrite(ledPin, HIGH);
    digitalWrite(ledButton, LOW);
  } else {
    // turn LED off:
    digitalWrite(ledPin, LOW);
    digitalWrite(ledButton, HIGH);
  }
}




