/* Master set up operation*/
#include <SPI.h>
#include <nRF24L01.h>
#include "printf.h"
#include <RF24.h>

#define NBR_SLAVES  2
#define MAX_PLAYERS 4 //Ne pas modifier pour utiliser toutes les couleurs
#define CE_PIN 9
#define CSN_PIN 10

// ============================================
// Pin Definitions and Global Variables
// ============================================


// Defining the Buttons
const int StartButton = 2; // Pin for Start Button
const int SetUpButton = 3; // Pin for SetUp Button
const int StartLed = 4; // Pin for Start Led
const int SetUpLed = 5; // Pin for SetUp Led
// Defining the Master ID
const int deviceID = 0;

// Defining the Potentiometers
const int potPlayerPin = A0; // Pin for Player Potentiometer
const int potDifficultyPin = A1; // Pin for Difficulty Potentiometer
const int potModePin = A2; // Pin for Game Mode Potentiometer

// Defining the buttons states
bool StartState = false; // State for Start Button
bool SetUpState = false; // State for SetUp Button 

// Defining the CSN and CE pins and the address pipes for wireless communication
RF24 radio(CE_PIN, CSN_PIN); // Create an RF24 object with CSN and CE pins
const uint64_t addresses[2] = { 0x5A36484130LL,
                                0x5448344430LL};  // Addresses for master and slaves



// ============================================
// Enums and Struct Definitions
// ============================================



// Enum for clarity in player color selection - Audeline called this Colors
enum PlayerColor : uint8_t { 
  NONE=0, 
  RED, 
  GREEN, 
  BLUE, 
  YELLOW 
};

// Structure representing a player
struct PlayerStruct {
  PlayerColor color; // Player color
  uint8_t boardIDs[NBR_SLAVES];  // Max 2 boards per player but will be changed to 4
  uint8_t nbOfModules; // Count of boards assigned to the player
  uint8_t score;
};
 // DO WE NEED MODULE STRUCT??

// Enum for clarity in master commands - different names from Audeline
enum CommandsFromMaster : uint8_t {
  NO_COMMAND = 0,
  ENTER_SETUP = 1,
  START_GAME = 2,
  BUTTONS = 3,
  SCORE = 4,
  MISSED_BUTTONS =5
};


// Defining the structure for messages between master and slave
/* Changing this with Aude-line version
struct Payload {
  uint8_t commandFlags;
  int Data;
};
Payload datatoSend;
Payload receivedData;*/

struct PayloadFromMasterStruct {
  CommandsFromMaster command;
  uint8_t buttonsToPress;
  uint16_t score;
};

struct PayloadFromSlaveStruct {
  PlayerStruct idPlayer;
  bool buttonsPressed;
};

// Message structure for Setup communication with slaves
struct SetUpMessage {
  uint8_t commandFlags; // Flags to communicate with the slaves
  PlayerColor color; // Selected color or none
};

// Struct for board assignments (used to track active boards)
struct BoardAssignment {
  bool active = false;
  PlayerColor color = NONE;
};

BoardAssignment boardAssignments[NBR_SLAVES]; // Will be up to 4, rigyht now 2


// ============================================
// Setup Function
// ============================================


void setup() {
  Serial.begin(9600); // Start serial communication
  while (!Serial) {
    // Ensures serial communication is ready
  }
  delay(1500); // Delay to stabilize communication

  // Initialize input pins
  pinMode(StartButton, INPUT_PULLUP);
  pinMode(SetUpButton, INPUT_PULLUP);
  pinMode(StartLed, OUTPUT);
  pinMode(SetUpLed, OUTPUT);
  
  // Initialize the wireless communication
  if(!radio.begin()){
    Serial.println(F("radio hardware is not responding!"));
    while(1){} // Halt the program if the radio isn't working
  }

 // Set wireless communication settings
  radio.setChannel(125); // Setting communication channel
  radio.setPALevel(RF24_PA_LOW); //Setting power level
  radio.enableDynamicPayloads(); //Allow dynamic payload sizes for flexible message lenghts
  
  // Set up the address pipes dynamically for slaves
  for (uint8_t i = 0; i < NBR_SLAVES; i++){
    Serial.print(F("Setting address for slave "));
    Serial.print(i);
    Serial.print(F(": 0x"));
    print64Hex(addresses[1] + i);  // Print slave address in hexadecimal format

    // Open reading pipes for each slave (1-4), as 0 is reserved for writing
    radio.openReadingPipe(i+1, addresses[1]+i); // Assign reading pipes
    radio.openWritingPipe(addresses[0]); // Address of transmitter
  }
  
  radio.stopListening(); // Set the radio to TX mode

  Serial.println("Ready. Press SETUP to begin");

}




// ============================================
// Main Loop Function
// ============================================



void loop() {
  unsigned long currentMillis = millis(); // Get current time
  // Continuously monitor potentiometer values (non-blocking)
  unsigned long lastTime = 0;
  const long interval = 200;  // Update every 200ms for responsiveness
  int selectedPlayers, selectedDifficulty, selectedGameMode = 1;
  PlayerStruct playersList[NBR_SLAVES]; // More likely 2
  int playerCount;

  // Check if it's time for periodic tasks
  if (currentMillis - lastTime >= interval) {
    lastTime = currentMillis;
    

    // Read button states without blocking
    SetUpState = digitalRead(SetUpButton) == LOW;
    StartState = digitalRead(StartButton) == LOW;
  

  
    //SetUpState = Serial.read();
    if (SetUpState) {
      // Enter Setupmode if SetUp button is pressed

      SetUpMode(playersList, playerCount, selectedPlayers, selectedDifficulty, selectedGameMode);

      
    } 
  // Start the game based on selected mode
    switch (selectedGameMode) {
      case 1:
        startAccuracyMode(playersList, playerCount, selectedDifficulty);
        break;
      // Add future modes here
    }
    //Additional periodic tasks to add here
  }
    

}




// ============================================
// Helper Functions
// ============================================




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

void read(){
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