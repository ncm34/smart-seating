#include <SPI.h>              // Library to communicate over Serial Peripheral Interface
#include <MFRC522.h>          // Library to communicate with RC522 RFID reader
#include <Ethernet.h>         // Library to communicate with W5100 ethernet shield
#include <utility/w5100.h>
/*
 * Pin layout for ATMEGA chip
 * ------------------------------------
 * Signal      MFRC522      ATMEGA328P       
 * ------------------------------------
 * RST/Reset   RST          9             
 * SPI SS      SDA(SS)      8, 10 .....           
 * SPI MOSI    MOSI         11
 * SPI MISO    MISO         12
 * SPI SCK     SCK          13
 */
 
#define RFID_RST_0_PIN    3     // Shared reset pin across all RC522 (SPI)
#define RFID_RST_1_PIN    4
#define RFID_RST_2_PIN    3
#define RFID_RST_3_PIN    2

#define RFID_0_SS_PIN   8     // RC522 SDA (Chip Select)  
#define RFID_1_SS_PIN   7     
#define RFID_2_SS_PIN   6   
#define RFID_3_SS_PIN   5   

#define ETH_SS_PIN      10    // SDA for W5100
#define LED_PIN         2       // 

#define UID_BUFFER_SIZE 4     // Size of card read (UID)

// recognized RFID cards
byte CARD_0_UID[] = {0x0A, 0x36, 0xB7, 0x35};
byte CARD_1_UID[] = {0x5A, 0x7F, 0xE7, 0x05};
byte CARD_2_UID[] = {0xF4, 0x40, 0x20, 0x2B};
byte CARD_3_UID[] = {0xA9, 0xC7, 0x1A, 0xDB};

// State of each RFID sensor (0x00 represents no card)
byte RFID_0_READ[] = {0x00, 0x00, 0x00, 0x00};
byte RFID_1_READ[] = {0x00, 0x00, 0x00, 0x00};
byte RFID_2_READ[] = {0x00, 0x00, 0x00, 0x00};
byte RFID_3_READ[] = {0x00, 0x00, 0x00, 0x00};

byte MAC_ADDR[] = {0x90, 0xA2, 0xDA,    // Mac address of w5100
                  0x0F, 0x4D, 0x49 };


byte mac[] = {0x90, 0xA2, 0xDA,    // Mac address of w5100
                  0x0F, 0x4D, 0x49 };

char str[] = "GET /read/XY HTTP/1.1";

char HEALTH_URL[] = "http://smart-seating.herokuapp.com/health";  // URL to check server health
char REQUEST_URL[] = "http://smart-seating.herokuapp.com/read";    // URL to talk to server
char server[] = "http://smart-seating.herokuapp.com";    // name address for Google (using DNS)


EthernetClient client;        // Ethernet client that talks to the server
IPAddress ip(10, 0, 1, 50);   // Fallback IP address if DHCP fails with W5100

 
/** Scenarios for 4Card x 4Rfid setup
 *  0 = RFID 0, CARD 0
 *  1 = RFID 0, CARD 1
 *  2 = RFID 0, CARD 2
 *  3 = RFID 0, CARD 3
 *  
 *  4 = RFID 1, CARD 0
 *  5 = RFID 1, CARD 1
 *  6 = RFID 1, CARD 2
 *  7 = RFID 1, CARD 3
 *  
 *  8 = RFID 2, CARD 0
 *  9 = RFID 2, CARD 1
 *  10 = RFID 2, CARD 2
 *  11 = RFID 2, CARD 3
 *  
 *  12 = RFID 3, CARD 0
 *  13 = RFID 3, CARD 1
 *  14 = RFID 3, CARD 2
 *  15 = RFID 3, CARD 3
 */
int scenario = -1;

boolean clientConnected = false;
long updateTimer;


MFRC522 rfid_0(RFID_0_SS_PIN, RFID_RST_0_PIN);    // Create MFRC522 instances
MFRC522 rfid_1(RFID_1_SS_PIN, RFID_RST_0_PIN);    // All RF chips connect to the same RST
MFRC522 rfid_2(RFID_2_SS_PIN, RFID_RST_0_PIN);  
MFRC522 rfid_3(RFID_3_SS_PIN, RFID_RST_0_PIN);  

/**
 * initSerial() - initializes the serial port and waits for Serial.available() to be > 1
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: if the serial port fails to initialize, the function will continue forever
 */
void initSerial() {
  Serial.begin(9600);
  while(!Serial)
    ;
}

/**
 * initSPI() - initializes the Serial Peripheral Interface
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: sets SCK, MOSI, and SS to outputs --> pulling SCK and MOSI low, and SS high
 */
void initSPI() {
  SPI.begin(); 
}

/**
 * initW5100() - initializes W5100 ethernet module
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: disables SD functionality on the module (Pin 4); quietly fails if DHCP can not be configured
 */
void initW5100() {
  
  pinMode(4,OUTPUT); // Disable W5100 SD module
  digitalWrite(4,HIGH);

  // Try to connect to the Internet with DHCP. 
  // Static IP is not tried in the case of failure
  if (Ethernet.begin(MAC_ADDR) == 0) return;
  
  // give the Ethernet shield a second to initialize:
  delay(1000);

  // Test the server's health
  if (client.connect(HEALTH_URL, 80))
    ;
  
  else
    ;

  W5100.setRetransmissionCount(5); // This gives us more tries to get connected

}


/**
 * initRFID() - initializes RFID chip specified by identifier (0-3)
 * INPUTS: rfidNum = {0, 1, 2, 3}
 * OUTPUTS: none
 * SIDE EFFECTS: none
 */
void initRFID(int rfidNum) {
  switch (rfidNum) {
    case 0:
      rfid_0.PCD_Init();
      pinMode(RFID_0_SS_PIN, OUTPUT);
      digitalWrite(RFID_0_SS_PIN, LOW);
      break;
    case 1:
      rfid_1.PCD_Init();
      pinMode(RFID_1_SS_PIN, OUTPUT);
      digitalWrite(RFID_1_SS_PIN, LOW);
      break;
    case 2:
      rfid_2.PCD_Init();
      pinMode(RFID_2_SS_PIN, OUTPUT);
      digitalWrite(RFID_2_SS_PIN, LOW);
      break;
    case 3:
      rfid_3.PCD_Init();
      pinMode(RFID_3_SS_PIN, OUTPUT);
      digitalWrite(RFID_3_SS_PIN, LOW);
      break;
    default: break;
  }

  checkReader0();
  checkReader1();
  checkReader2();
  checkReader3();
}

/**
 * scanRFID(rfidNum) - scans RFID chip specified by identifier (0-3)
 * This works by verifying that a new card is present and reading the 
 * serial data, storing it in rfid_rfidNum.uid.uidByte. The definition
 * of a 'new' card according to MiFare is a card that is in the idle state,
 * not one that has not been seen on the previous scan. This is handled
 * using newCardPresent and updateRFID.
 * INPUTS: rfidNum = {0, 1, 2, 3}
 * OUTPUTS: none
 * SIDE EFFECTS: if scan is successful, uid.uidByte set for the reader scanned
 */
void scanRFID(int rfidNum) {
  
  switch (rfidNum) {
    case 0:
      enableRFID(rfidNum);
      if (!rfid_0.PICC_IsNewCardPresent()) {
        ledOff();
        return;
      }
      
      if (!rfid_0.PICC_ReadCardSerial()) return;

      
      ledOn();
      updateClient(0);
      checkAvail();
      
      break;
    case 1:
      enableRFID(rfidNum);
      if (!rfid_1.PICC_IsNewCardPresent()) {
        ledOff();
        return;
      }
      
      if (!rfid_1.PICC_ReadCardSerial()) return;

      ledOn();
      updateClient(1);
      checkAvail();
      
      break;
    case 2:
      enableRFID(rfidNum);
      if (!rfid_2.PICC_IsNewCardPresent()) {
        ledOff();
        return;
      }
      if (!rfid_2.PICC_ReadCardSerial()) return;

      ledOn();
      updateClient(2);
      checkAvail();
      
      break;
    case 3:
      enableRFID(rfidNum);
      if (!rfid_3.PICC_IsNewCardPresent()) {
        ledOff();
        return;
      }
      
      if (!rfid_3.PICC_ReadCardSerial()) return;
    
      ledOn();
      updateClient(3);
      checkAvail();
      
      break;
    default: return;
  }

}

/**
 * newCardPresent(rfidNum) - checks the RFID scan to see if the card is different
 * from the last scan. Before calling this, you must call scanRFID(rfidNum)
 * INPUTS: rfidNum = {0, 1, 2, 3}
 * OUTPUTS: true if this is a new card, false otherwise (incl. default case)
 * SIDE EFFECTS: 
 */
bool newCardPresent(int rfidNum) {
  switch (rfidNum) {
    case 0:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        if (rfid_0.uid.uidByte[i] != RFID_0_READ[i]) return false;
      break;
    case 1:
     for(int i = 0; i < UID_BUFFER_SIZE; i++)
        if (rfid_1.uid.uidByte[i] != RFID_1_READ[i]) return false;
      break;
    case 2:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        if (rfid_2.uid.uidByte[i] != RFID_2_READ[i]) return false;
      break;
    case 3:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        if (rfid_3.uid.uidByte[i] != RFID_3_READ[i]) return false;
      break;
    default: false;
  }

  return true;
}

/**
 * updateRFID(rfidNum) - updates the stored card UID with the new scan data.
 * Note that this function is only called if a new card is present -- it's a
 * waste of resources to re-write the buffer with the same values...
 * INPUTS: rfidNum = {0, 1, 2, 3}
 * OUTPUTS: none
 * SIDE EFFECTS: RFID_rfidNum_READ[...] will be updated with the uidByte buffer 
 */
void updateRFID(int rfidNum) {
  switch (rfidNum) {
    case 0:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        RFID_0_READ[i] = rfid_0.uid.uidByte[i];
      break;
    case 1:
     for(int i = 0; i < UID_BUFFER_SIZE; i++)
        RFID_1_READ[i] = rfid_1.uid.uidByte[i];
      break;
    case 2:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        RFID_2_READ[i] = rfid_2.uid.uidByte[i];
      break;
    case 3:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        RFID_3_READ[i] = rfid_3.uid.uidByte[i];
      break;
    default: return;
  }
}

/**
 * resetRFID(rfidNum) - resets the card read data for the RFID reader specified
 * by rfidNum. A 'reset' is defined as {0x00, 0x00, 0x00, 0x00}
 * INPUTS: rfidNum = {0, 1, 2, 3}
 * OUTPUTS: none
 * SIDE EFFECTS: RFID_rfidNum_READ[...] will be updated with zeroes
 */
void resetRFID(int rfidNum) {
  switch (rfidNum) {
    case 0:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        RFID_0_READ[i] = 0x00;
      break;
    case 1:
     for(int i = 0; i < UID_BUFFER_SIZE; i++)
        RFID_1_READ[i] = 0x00;
      break;
    case 2:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        RFID_2_READ[i] = 0x00;
      break;
    case 3:
      for(int i = 0; i < UID_BUFFER_SIZE; i++)
        RFID_3_READ[i] = 0x00;
      break;
    default: return;
  }
}

/**
 * enableRFID(rfidNum) - enables RFID chip specified by identifier (0-3)
 * INPUTS: rfidNum = {0, 1, 2, 3}
 * OUTPUTS: none
 * SIDE EFFECTS: turns off all other devices on the same SPI bus
 */
void enableRFID(int rfidNum) {
  
  switch (rfidNum) {
    case 0:
      digitalWrite(RFID_0_SS_PIN, LOW);
      digitalWrite(RFID_1_SS_PIN, HIGH);
      digitalWrite(RFID_2_SS_PIN, HIGH);
      digitalWrite(RFID_3_SS_PIN, HIGH);
      digitalWrite(ETH_SS_PIN, HIGH);
      break;
    case 1:
      digitalWrite(RFID_0_SS_PIN, HIGH);
      digitalWrite(RFID_1_SS_PIN, LOW);
      digitalWrite(RFID_2_SS_PIN, HIGH);
      digitalWrite(RFID_3_SS_PIN, HIGH);
      digitalWrite(ETH_SS_PIN, HIGH);
      break;
    case 2:
      digitalWrite(RFID_0_SS_PIN, HIGH);
      digitalWrite(RFID_1_SS_PIN, HIGH);
      digitalWrite(RFID_2_SS_PIN, LOW);
      digitalWrite(RFID_3_SS_PIN, HIGH);
      digitalWrite(ETH_SS_PIN, HIGH);
      break;
    case 3:
      digitalWrite(RFID_0_SS_PIN, HIGH);
      digitalWrite(RFID_1_SS_PIN, HIGH);
      digitalWrite(RFID_2_SS_PIN, HIGH);
      digitalWrite(RFID_3_SS_PIN, LOW);
      digitalWrite(ETH_SS_PIN, HIGH);
      break;
    default: break;
  }
}

/**
 * enableW5100(rfidNum) - enables W5100 module for network requests
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: turns off all other devices on SPI bus (all RFID readers)
 */
void enableW5100() {
  digitalWrite(RFID_0_SS_PIN, HIGH);
  digitalWrite(RFID_1_SS_PIN, HIGH);
  digitalWrite(RFID_2_SS_PIN, HIGH);
  digitalWrite(RFID_3_SS_PIN, HIGH);
  digitalWrite(ETH_SS_PIN, LOW);

 // initW5100();
}

/**
 * setup() - calls all initialization functions before entering loop()
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: if anything in setup() hangs, loop() will never execute
 */
void setup() {
  ledOff();  
  initSPI();
  initW5100();
  for(int i = 0; i <= 3; i++)
    initRFID(i);
}

/**
 * loop() - implements the RR algorithm to perpetually poll all RFID sensors in circular fashion
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: none
 */
void loop() {
  
  // RR around all 4 RFID chips
  for(int i = 0; i < 4; i++) {
    /* 1. Enable 2. Scan 3. Update 4. Check In */
    enableRFID(i);
    scanRFID(i);
  }

  

  
}

char intToString(int state) {

  if(state == 0) return '0';
  if(state ==1) return '1';
  if(state==2) return '2';
  if(state==3)return '3';

}

void dumpRFID() {
  printByteArray(RFID_0_READ, 4);
  printByteArray(RFID_1_READ, 4);
  printByteArray(RFID_2_READ, 4);
  printByteArray(RFID_3_READ, 4);
}

void updateClient(int state)
{

  /**
   * 
    // recognized RFID cards
    byte CARD_0_UID[] = {0x0A, 0x36, 0xB7, 0x35};
    byte CARD_1_UID[] = {0x5A, 0x7F, 0xE7, 0x05};
    byte CARD_2_UID[] = {0xF4, 0x40, 0x20, 0x2B};
    byte CARD_3_UID[] = {0xA9, 0xC7, 0x1A, 0xDB};
   */
   
  str[10] = intToString(state);
  if(state == 0 && rfid_0.uid.uidByte[0] == 0x0A) str[11] = 'A';
  else if(state == 0 && rfid_0.uid.uidByte[0] == 0x5A) str[11] = 'B';
  else if(state == 0 && rfid_0.uid.uidByte[0] == 0xF4) str[11] = 'C';
  else if(state == 0 && rfid_0.uid.uidByte[0] == 0xA9) str[11] = 'D';
  
  else if(state == 1 && rfid_1.uid.uidByte[0] == 0x0A) str[11] = 'A';
  else if(state == 1 && rfid_1.uid.uidByte[0] == 0x5A) str[11] = 'B';
  else if(state == 1 && rfid_1.uid.uidByte[0] == 0xF4) str[11] = 'C';
  else if(state == 1 && rfid_1.uid.uidByte[0] == 0xA9) str[11] = 'D';
  
  else if(state == 2 && rfid_2.uid.uidByte[0] == 0x0A) str[11] = 'A';
  else if(state == 2 && rfid_2.uid.uidByte[0] == 0x5A) str[11] = 'B';
  else if(state == 2 && rfid_2.uid.uidByte[0] == 0xF4) str[11] = 'C';
  else if(state == 2 && rfid_2.uid.uidByte[0] == 0xA9) str[11] = 'D';

  else if(state == 3 && rfid_3.uid.uidByte[0] == 0x0A) str[11] = 'A';
  else if(state == 3 && rfid_3.uid.uidByte[0] == 0x5A) str[11] = 'B'; 
  else if(state == 3 && rfid_3.uid.uidByte[0] == 0xF4) str[11] = 'C';
  else if(state == 3 && rfid_3.uid.uidByte[0] == 0xA9) str[11] = 'D';
  
 if ((millis() - updateTimer) > 0)
 {
   Ethernet.begin(mac, ip);
   ;//Serial.println("connecting...");
   delay(1000);
   if (client.connect(server, 80))
   {
        // Make a HTTP request:
    // client.println("GET /read/XY HTTP/1.1");
    client.println(str);
    client.println("Host: smart-seating.herokuapp.com");
    client.println("Connection: close");
    client.println();
    clientConnected = true;
   }
   else
   {
     ;//Serial.println("connection failed");
   }
   updateTimer = millis();
 }

 str[11] = '?'; // Reset the string
}

void checkAvail()
{
 if (clientConnected)
 {
   if (client.available())
   {
     char c = client.read();
     Serial.print(c);
   }
   if (!client.connected())
   {
     ;//Serial.println();
     ;//Serial.println("disconnecting.");
     client.stop();
     clientConnected = false;
   }
 }
}

void performNetworkRequest(char* buffer) {

  enableW5100();
client.stop();
int r = client.connect(server, 80);
if (client.connected()) {
    ledWOff();
  }
  else if(r < 0) {
    for(int i = r; i < 0; i++) {
      ledWOn();
      delay(250);
      ledWOff();
    }
  }
  else {
    ledWOff();
  }

      // Make a HTTP request:
    client.println("GET /read/moulees2 HTTP/1.1");
    client.println("Host: smart-seating.herokuapp.com");
    client.println("Connection: close");
    client.println();


  
  
}

/* These functions used to print to serial, but instead, LEDs are turned on in the scan functions */
void checkReader0() {
  // Get the MFRC522 software version
  byte v = rfid_0.PCD_ReadRegister(rfid_0.VersionReg);
  ; 
  ; 
  if (v == 0x91)
    ; 
  else if (v == 0x92)
    ; 
  else
    ; 
  
  if ((v == 0x00) || (v == 0xFF)) {
  ; // Comm failed
  }
}

void checkReader1() {
  // Get the MFRC522 software version
  byte v = rfid_1.PCD_ReadRegister(rfid_1.VersionReg);
  ; 
  ; 
  if (v == 0x91)
    ; 
  else if (v == 0x92)
    ; 
  else
    ; 
  
  if ((v == 0x00) || (v == 0xFF)) {
  ; // Comm failed
  }
}

void checkReader2() {
  // Get the MFRC522 software version
  byte v = rfid_2.PCD_ReadRegister(rfid_2.VersionReg);
  ; 
  ; 
  if (v == 0x91)
    ; 
  else if (v == 0x92)
    ; 
  else
    ; 
  
  if ((v == 0x00) || (v == 0xFF)) {
  ; // Comm failed
  }
}

void checkReader3() {
  // Get the MFRC522 software version
  byte v = rfid_3.PCD_ReadRegister(rfid_3.VersionReg);
  ; 
  ; 
  if (v == 0x91)
    ; 
  else if (v == 0x92)
    ; 
  else
    ; 
  
  if ((v == 0x00) || (v == 0xFF)) {
  ; // Comm failed
  }
}

/**
 * Function to dump a byte array as hex values to Serial.
 * Not used anymore as MEGA offers us no serial out to the PC.
 */
byte*printByteArray(byte *buffer, byte bufferSize) {
    ;
}

void ledOff() {
  pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void ledOn() {
  pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
}

void ledWOn() {

  pinMode(A0, OUTPUT);
digitalWrite(A0, HIGH);

}

void ledWOff() {

  pinMode(A0, OUTPUT);
digitalWrite(A0, HIGH);

}


