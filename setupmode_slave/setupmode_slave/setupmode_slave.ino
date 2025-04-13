/* Slave set up operation*/
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_AW9523.h>


#define ID_SLAVE 1 //Unique and in [0, NBR_SLAVES[ (defined in the master code)
#define NBR_SLAVES  2
#define MAX_PLAYERS 4 //Ne pas modifier pour utiliser toutes les couleurs
#define CE_PIN 9
#define CSN_PIN 10


// ============================================
// Pin Definitions and Global Variables
// ============================================

// Defining the Buttons and LEDS
Adafruit_AW9523 aw;
const int buttonPins[] = {4, 5, 6, 7};  // Array of button pin numbers (Yellow, Green, Blue, Red)
const int ledPins[] = {8,9,10,11}; // The Leds on the module

// Defining the buzzer
const int buzzer = 12;

 


RF24 radio(CE_PIN, CSN_PIN); // Set accordingly to branching
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

BoardAssignment boardAssignments[NBR_SLAVES]; 

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
 for (int i = 0; i < 4; i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
    aw.pinMode(ledPins[i], OUTPUT);
  }
  pinMode(buzzer, OUTPUT);

  // Initialize the wireless communication
  if(!radio.begin()){
    Serial.println(F("radio hardware is not responding!"));
    while(1){} // Halt the program if the radio isn't working
  }

  Serial.print("address to send: ");
  print64Hex(addresses[1] + ID_SLAVE);
  Serial.print("address to receive: ");
  print64Hex(addresses[0] + ID_SLAVE);
  

 // Set wireless communication settings
  radio.setChannel(125); // Setting communication channel
  radio.setPALevel(RF24_PA_LOW); //Setting power level
  radio.enableDynamicPayloads(); //Allow dynamic payload sizes for flexible message lenghts
  radio.openWritingPipe(addresses[1]+ID_SLAVE);
  radio.openReadingPipe(1, addresses[0]+ID_SLAVE);
  radio.startListening();
  Serial.println("Ready. Press SETUP to begin");
}

// ============================================
// Main Loop Function
// ============================================

void loop() {
   readSlave();
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


