const int led1 = 10, 
          led2 = 11, 
          led3 = 12, 
          led4 = 13;

#define potentiometer A1

const int MAXPOTENTIOMETERVAL = 1023;

int leds[] = {led1, led2, led3, led4};

//The difficulies are represented by the interval between 2 LED lighting and the delay the player can push the button
struct Difficulty {
    int lightingInterval[2];
    unsigned long pushingDelay;
};

// Define difficulties with both values
const Difficulty EASY   = {{6000, 6000}, 5000};
const Difficulty MEDIUM = {{4000, 4000}, 3000};
const Difficulty HARD   = {{1000, 1000}, 1000};

Difficulty difficulties[3] = {EASY, MEDIUM, HARD};

// the setup routine runs once when you press reset:
void setup() {
  initLED();
  pinMode(A0, INPUT);
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  //Get the value of the pentotiometer
  int diffValue = analogRead(potentiometer);
  Difficulty diff = getDifficulty(diffValue);
  Serial.println(diff.pushingDelay);
  Difficulty currentDiff = diff;
  waitLighting(currentDiff.lightingInterval);
  //Select randomly one of the LED to light it up
  int unitIdx = getRdmUnit();
  lightUpLED(unitIdx);
  delay(currentDiff.pushingDelay);
  lightDownLED(unitIdx);
}


void initLED(){
  for(int led : leds){
      pinMode(led, OUTPUT);
  }
}

int getGameModeDelay(int modeValue, int nbMode, int minDelay){
  int interval = MAXPOTENTIOMETERVAL/nbMode;
  for(int i = 0; i < nbMode; i++){
    if(interval*i < modeValue && modeValue <= (i+1)*interval){
      return minDelay*(i+1)*(i+1);
    }
  }
}

Difficulty getDifficulty(int diffValue){
  int nbMode = 3;
  int interval = MAXPOTENTIOMETERVAL/nbMode;
  for(int i = 0; i < nbMode; i++){
    if(interval*i < diffValue && diffValue <= (i+1)*interval){
      return difficulties[i];
    }
  }
}

/*
void difficultyAction(char difficulty){
  randomLighting();
  for(int i = 0; i < sizeof(difficulties); i++){
    if(difficulty == difficulties[i]){
        delay(speeds[i]);
    }
  }
  
}
*/

/*
  Select randomly one of the 4 unit (button + LED) and return its index (1 to 4)
*/
int getRdmUnit(){
  //sizeof return the size of the array
  return random(0,sizeof(leds) / sizeof(leds[0]));

}

/*
  Wait a certain random delay influenced by the difficulty
*/
void waitLighting(int lightingInterval[2]){
  delay(random(lightingInterval[0],lightingInterval[1])); 
}


/*
  Light up the LED with the corresponding given pin 
*/
void lightUpLED(int ledIdx){
  digitalWrite(leds[ledIdx], HIGH);
}

void lightDownLED(int ledIdx){
    digitalWrite(leds[ledIdx], LOW);
}


