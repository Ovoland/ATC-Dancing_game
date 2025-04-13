// This file will contain the actual implementation of the functions or methods declared in the header

#include "MyLibrary.h"

// Pin definitions: to be set 
const int StartButton = 2;
const int SetUpButton = 3;
const int StartLed = 4;
const int SetUpLed = 5;
const int deviceID = 0; // to be changed to master id
const int potPlayerPin = A0;
const int potDifficultyPin = A1;
const int potModePin = A2;
const int buttonPins[4] = {1,2,3,4};
const int ledPins[4] = {5,6,7,8};
const int ledStrips[4] = {9,10,11,12};
const int buzzer = 13;

// Global variables
RF24 radio(CE_PIN, CSN_PIN);
const uint64_t addresses[2] = { 0x5A36484130LL, 0x5448344430LL };
BoardAssignment boardAssignments[NBR_SLAVES];
Adafruit_AW9523 aw;


void MyLibrary::setup() {
    Serial.begin(9600);
    while (!Serial) { }
    delay(1500);
  
    pinMode(StartButton, INPUT_PULLUP);
    pinMode(SetUpButton, INPUT_PULLUP);
    pinMode(StartLed, OUTPUT);
    pinMode(SetUpLed, OUTPUT);
  
    if (!radio.begin()) {
      Serial.println(F("Radio hardware is not responding!"));
      while (1) {}
    }
  
    radio.setChannel(125);
    radio.setPALevel(RF24_PA_LOW);
    radio.enableDynamicPayloads();
  
    for (uint8_t i = 0; i < NBR_SLAVES; i++) {
      Serial.print(F("Setting address for slave "));
      Serial.print(i);
      Serial.print(F(": 0x"));
      print64Hex(addresses[1] + i);
      radio.openReadingPipe(i + 1, addresses[1] + i);
      radio.openWritingPipe(addresses[0]);
    }
    
    radio.stopListening();
    Serial.println("Ready. Press SETUP to begin");
}

void MyLibrary::loop() {
    unsigned long currentMillis = millis();
    unsigned long lastTime = 0;
    const long interval = 200;
    int selectedPlayers, selectedDifficulty, selectedGameMode = 1;
    PlayerStruct playersList[NBR_SLAVES];
    int playerCount;
  
    if (currentMillis - lastTime >= interval) {
      lastTime = currentMillis;
      SetUpState = digitalRead(SetUpButton) == LOW;
      StartState = digitalRead(StartButton) == LOW;
  
      if (SetUpState) {
        SetUpMode(playersList, playerCount, selectedPlayers, selectedDifficulty, selectedGameMode);
      }
    }
}

// Function to enter setup mode and read inputs
void SetUpMode (PlayerStruct playersList[], int &playerCount, int &playerNumb, int &difficulty, int &gameMode) {
    Serial.println("=== ENTERING SETUP MODE ===");
  
  
    // Communicate with slaves to inform them we are in setup mode
    sendMessageToAllSlave(ENTER_SETUP,0);  // Inform all slaves about entering setup
  
    // Read potentiometers continuosly
    while (!StartState) {
        // Continuously read and update button states
      SetUpState = digitalRead(SetUpButton) == LOW;
      StartState = digitalRead(StartButton) == LOW;
      // Continuos reading from potentiometers without blocking
      playerNumb = map(analogRead(potPlayerPin), 0, 1023, 1, 4);
      difficulty = map(analogRead(potDifficultyPin), 0, 1023, 1, 10);  // How many difficulty possibilities?
      gameMode = map(analogRead(potModePin), 0, 1023, 1, 4);
  
      // Check if slave has sent an update
      if (radio.available()) {
        SetUpMessage msg;
        radio.read(&msg, sizeof(msg));
        bool shouldAct = msg.commandFlags & (1 << deviceID); // Is it for me?
  
        if (shouldAct) {
          handleBoardAssignment(msg);
        }
      }
  
      // Provide perodic feedback
      provideFeedback(playerNumb, difficulty, gameMode);
  
    }
  
    // Exit Setup Mode when start is pressed
    
    Serial.println("Press START to finalize");
    delay(300); // we will not have delays is only for the serial print
    // Check for Start button press and exit in case
    StartState = digitalRead(StartButton) == LOW;
    if (StartState) {
      Serial.println("=== SETUP COMPLETE ===");
      Serial.print("Selected Players: "); Serial.println(playerNumb);
      Serial.print("Selected Difficulty: "); Serial.println(difficulty);
      Serial.print("Selected Mode: "); Serial.println(gameMode);
      radio.stopListening();
  
  
  
      // Will the master have to send something to the slaves? (maybe break from set up)
      
  
      //After setup parameters defined let's define the players
  
      buildPlayerListFromAssignments(boardAssignments, playersList, playerCount);
      
    }
}
  
// Function to provide feedback to user regarding on seleted setup features
void provideFeedback(int playerNumb, int difficulty, int gameMode) {
Serial.print("Players: ");
Serial.print(playerNumb);
Serial.print(" | Difficulty: ");
Serial.print(difficulty);
Serial.print(" | Game Mode: ");
Serial.println(gameMode);
delay(100);  // Periodic feedback delay (could be reduced or adjusted)
}


// Handle board assignment based on the received setup message
void handleBoardAssignment(SetUpMessage msg) {
// Process the board assignment
BoardAssignment &assignment = boardAssignments[msg.commandFlags];
if (assignment.active && assignment.color == msg.color) {
    assignment.active = false;
    assignment.color = NONE;
    Serial.print("Board ");
    Serial.print(msg.commandFlags);
    Serial.println(" deselected");
} else {
    assignment.active = true;
    assignment.color = msg.color;
    Serial.print("Board ");
    Serial.print(msg.commandFlags);
    Serial.print(" set to ");
    printColorName(msg.color);
}
}


// Print the color name for clarity
void printColorName( PlayerColor color) {
switch (color) {
    case RED: Serial.println("RED"); break;
    case YELLOW: Serial.println("YELLOW"); break;
    case BLUE: Serial.println("BLUE"); break;
    case GREEN: Serial.println("GREEN"); break;
    default: Serial.println("NONE"); break;

}
}


// Build the player list based on active board assignments
void buildPlayerListFromAssignments(BoardAssignment boardAssignments[], PlayerStruct playersList[], int &playerCount) {
playerCount = 0;

for (int boardID = 0; boardID < 4; boardID++) {
    if (boardAssignments[boardID].active) {
    PlayerColor boardColor = boardAssignments[boardID].color;

    // Check if a player with that color already exists
    bool found = false;
    for (int i = 0; i < playerCount; i++) {
        if (playersList[i].color == boardColor) {
        playersList[i].boardIDs[playersList[i].nbOfModules++] = boardID;
        found = true;
        break;
        }
    }

    // New player
    if (!found && playerCount < 4) {
        playersList[playerCount].color = boardColor;
        playersList[playerCount].boardIDs[0] = boardID;
        playersList[playerCount].nbOfModules = 1;
        playerCount++;
    }
    }
}
}

void readMaster(){
uint8_t pipe;
PayloadFromSlaveStruct payloadFromSlave;
PayloadFromMasterStruct payloadFromMaster;
if (radio.available(&pipe)) {              // is there a payload? get the pipe number that received it
    uint8_t bytes = radio.getDynamicPayloadSize();  // get the size of the payload
    radio.read(&payloadFromSlave, bytes);             // fetch payload from FIFO

    Serial.println(F("\n==========NEW RECEPTION=========="));
    Serial.print(F("From slave "));
    print64Hex(pipe-1);
    Serial.print(F(" from pipe "));
    Serial.print(pipe);

    Serial.println(F("Payload content:"));
    Serial.print(F("  ID player: "));
    //Serial.println(payloadFromSlave.idPlayer);
    Serial.print(F("  ButtonsToPress: "));
    Serial.println(payloadFromMaster.buttonsToPress);
    Serial.print(F("  Score: "));
    Serial.println(payloadFromMaster.score);
}
}

// Start accuracy mode (game mode logic to be implemented)
void startAccuracyMode(PlayerStruct playersList[], int playerCount, int difficulty) {}

void print64Hex(uint64_t val) {
uint32_t high = (uint32_t)(val >> 32);  // Poids fort
uint32_t low = (uint32_t)(val & 0xFFFFFFFF); // Poids faible

Serial.print(high, HEX);
Serial.println(low, HEX);
}

void sendMessage(CommandsFromMaster command, uint8_t receivers) {
PayloadFromMasterStruct payloadFromMaster;
radio.stopListening();

//Loop thorugh each slave based on the bitmask and send the message
for (uint8_t slave = 0; slave < NBR_SLAVES; ++slave) {
    if (receivers & (1 << slave)) { // Check if slave is selected by bitmask
    // Create the payload
    payloadFromMaster.command = command;
    payloadFromMaster.buttonsToPress = random(0, 16);  // Random for testing
    payloadFromMaster.score = random(0, 100);  // Random for testing

    // Set the writing pipe to the selected slave
    radio.openWritingPipe(addresses[1]+slave);
    unsigned long start_timer = micros();
    bool report = radio.write(&payloadFromMaster, sizeof(payloadFromMaster));
    unsigned long end_time = micros();

    Serial.println(F("\n==========NEW TRANSMISSION=========="));
    Serial.print(F("Slave index: "));
    Serial.println(slave);
    Serial.print(F("Writing pipe address: 0x"));
    print64Hex(addresses[1]+slave);

    Serial.println(F("Payload content:"));
    Serial.print(F("  Command: "));
    Serial.println(payloadFromMaster.command);
    Serial.print(F("  ButtonsToPress: "));
    Serial.println(payloadFromMaster.buttonsToPress);
    Serial.print(F("  Score: "));
    Serial.println(payloadFromMaster.score);

    if (report) {
        Serial.print(F("✅ Transmission successful in "));
        Serial.print(end_time - start_timer);
        Serial.println(F(" µs"));
    } else {
        Serial.println(F("❌ Transmission failed"));
    }
    }
}
radio.startListening();  // put radio in RX mode
}


// Send a message to all slaves (broadcasting)
void sendMessageToAllSlave(uint8_t command, int data ) {
// Prepare the Payload to send
PayloadFromMasterStruct payloadFromMaster;
payloadFromMaster.command = command;  // Set the command to send to the slaves
payloadFromMaster.buttonsToPress = random(0, 16);  // Can be used to send additional data (e.g., difficulty, playerID)
payloadFromMaster.score = random(0, 16);  // This could be for any other data you need to send

radio.stopListening();  // Stop listening to send

// Create a bitmask where all slaves are included (i.e., broadcast to all)
uint8_t allSlavesMask = B11111111; // or 0xFF, since we have 8 slaves (NBR_SLAVES = 8)

// Use sendMessage to broadcast to all slaves
sendMessage(command, allSlavesMask); // Send the message to all slaves
}

void enterSetupMode() {
    Serial.println(" Entered SetUp Mode");
    Serial.println("Select your color");
  
  
  
      while (true) {
          int selectedColor = -1;
          for (int i = 0; i < 4; i++) {
              if (digitalRead(buttonPins[i]) == LOW) {
                  selectedColor = i + 1; // Map button to color index
                  break;
              }
          }
          if (selectedColor != -1) {
              assignColor(static_cast<PlayerColor>(selectedColor)); // Call function for color assignment
              break;
          }
      }
  }
  
void assignColor(PlayerColor color) { 
// Print out the selected color
Serial.print("Selected Color: ");
switch (color) {
    case YELLOW:
    Serial.println("Yellow");
    break;
    case GREEN:
    Serial.println("Green");
    break;
    case BLUE:
    Serial.println("Blue");
    break;
    case RED:
    Serial.println("Red");
    break;
    default:
    Serial.println("Unknown Color");
    break;
}

// Send the selected color to the master (for example, as an integer or enum value)
SetUpMessage colorData;
colorData.commandFlags = B0000001; 
colorData.color = color; // send the enum values per each color

radio.stopListening();
bool success = radio.write(&colorData, sizeof(colorData));  // Send color to master
radio.startListening();  // Start listening again

if (success) {
    Serial.println("Color sent to master successfully!");
} else {
    Serial.println("Failed to send color.");
}
}

PayloadFromMasterStruct  readSlave(){
PayloadFromMasterStruct payloadFromMaster;
if (radio.available()) {              // is there a payload? get the pipe number that received it
    uint8_t bytes = radio.getDynamicPayloadSize();  // get the size of the payload
    radio.read(&payloadFromMaster, bytes);             // fetch payload from FIFO

    Serial.println(F("\n==========NEW RECEPTION=========="));

    Serial.println(F("Payload content:"));
    Serial.print(F("  Command: "));
    Serial.println(payloadFromMaster.command);
    Serial.print(F("  ButtonsToPress: "));
    Serial.println(payloadFromMaster.buttonsToPress);
    Serial.print(F("  Score: "));
    Serial.println(payloadFromMaster.score);

    // Check the command received from the master and handle accordingly
    switch (payloadFromMaster.command) {
    case ENTER_SETUP:
        Serial.println("Master command: ENTER_SETUP received.");
        enterSetupMode();
        break;
        
    case START_GAME:
        Serial.println("Master command: START_GAME received.");
        // Add handling for START_GAME if needed
        break;

    case BUTTONS:
        Serial.println("Master command: BUTTONS received.");
        // Add handling for BUTTONS if needed
        break;

    case SCORE:
        Serial.println("Master command: SCORE received.");
        // Add handling for SCORE if needed
        break;

    case MISSED_BUTTONS:
        Serial.println("Master command: MISSED_BUTTONS received.");
        // Add handling for MISSED_BUTTONS if needed
        break;

    default:
        Serial.println("No valid command received.");
        break;
    }
}
return payloadFromMaster;  // Return the payload received from the master
}





void print64Hex(uint64_t val) {
uint32_t high = (uint32_t)(val >> 32);  // Poids fort
uint32_t low = (uint32_t)(val & 0xFFFFFFFF); // Poids faible

Serial.print("0x");
Serial.print(high, HEX);
Serial.println(low, HEX);
}


void sendMessageSlave(PayloadFromSlaveStruct payloadFromSlave) {
radio.stopListening();
unsigned long start_timer = micros();
bool report = radio.write(&payloadFromSlave, sizeof(payloadFromSlave));
unsigned long end_time = micros();

Serial.println(F("\n==========NEW TRANSMISSION=========="));
Serial.print(F("Slave index: "));
Serial.println(ID_SLAVE);
Serial.print(F("Writing pipe address: 0x"));
print64Hex(addresses[1]+ID_SLAVE);



if (report) {
    Serial.print(F("✅ Transmission successful in "));
    Serial.print(end_time - start_timer);
    Serial.println(F(" µs"));
} else {
    Serial.println(F("❌ Transmission failed"));
}
radio.startListening();  // put radio in RX mode
}