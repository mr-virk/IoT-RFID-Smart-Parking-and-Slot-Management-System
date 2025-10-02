#define BLYNK_TEMPLATE_ID "id"
#define BLYNK_TEMPLATE_NAME "name"
#define BLYNK_AUTH_TOKEN "token"

#define SS_PIN D4
#define RST_PIN D0

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

#include <SPI.h>
#include <Wire.h>
#include <Servo.h>
#include <EEPROM.h>
#include <PCF8574.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp8266.h>

//Your WiFi credentials.
//Set password to "" for open networks.
char ssid[] = "wifi";
char pass[] = "pass";

LiquidCrystal_I2C lcd(0x27, 20, 4);
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo myservo;

byte Lock[8] = { 0b01110, 0b10001, 0b10001, 0b11111, 0b11011, 0b11011, 0b11111, 0b00000 };
byte Check[8] = { 0b00000, 0b00000, 0b00001, 0b00010, 0b10100, 0b11000, 0b00000, 0b00000 };

const int MAX_STORED_CARDS = 8;
const int CARD_UID_LENGTH = 8;  // Number of characters in the card UID

char storedCards[MAX_STORED_CARDS][CARD_UID_LENGTH + 1];  // Array to store the UIDs
int numStoredCards = 0;

unsigned long previousTime = 0;
const unsigned long interval = 1000;  // 1 second

unsigned long uptimeStartTime = 0;  // Records the start time of the device

// Blynk virtual pin for displaying uptime
const int uptimePin = V1;

// PCF8574 I2C address
const int PCF8574_ADDRESS = 0x20;

// IR sensor pins connected to PCF8574
const int IR_SENSOR_PINS[] = { 0, 1, 2, 3 };

// Slot status
bool slotStatus[] = { false, false, false, false };

bool isSlotAvailable = false;

// exit variable
#define exitSensor A0
int exitSensorVal;

//RFID
byte storedUID[10]; // Declare a global array to store the UID
byte storedUIDSize = 0; // Declare a global variable to store the UID size

// Attach virtual serial terminal to Virtual Pin V1
WidgetTerminal terminal(V15);

String remoteInput;

void checkTimers();
void detectCard();
void addCard();
bool checkStoredCards(const char* cardUID);
void getCardUID(char* uidBuffer);
void lcdIntro();
void lcdScancard();
void cardAcceptedlcd();
void cardDeclinedlcd();
void adminMenu();
void aboutSystem();
void openGate();
void initializePCF8574();
int numSlotsAvailable();
void updateSlotStatus();

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.clear();
  lcd.backlight();

  lcd.createChar(6, Lock);
  lcd.createChar(7, Check);
  
  lcdIntro();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  SPI.begin();
  Wire.begin();
  initializePCF8574();  // Initialize PCF8574

  mfrc522.PCD_Init();;

  Serial.println("    (By: mr-virk)");

  myservo.attach(D3, 600, 2300);

  pinMode(exitSensor, INPUT);

  lcd.clear();
  lcdScancard();

  // Clear the terminal content
  terminal.clear();
  terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
  terminal.println(F("-------------"));
  terminal.println("=======================================");
  terminal.println("IoT RFID Smart Parking & Slot Management System");
  terminal.println("=======================================");
  terminal.println("|                                     ");
  terminal.println("|         Type '3' to go to about     ");
  terminal.println("|                                     ");
  terminal.print("|  Slots Available: ");
  terminal.print(numSlotsAvailable());
  terminal.println("                    ");
  terminal.println("|                                     ");
  terminal.println("|        Stored Cards:                ");
  for (int i = 0; i < numStoredCards; i++) {
    Serial.print("|  - ");
    Serial.println(storedCards[i]);
  }
  terminal.println("|                                     ");
  terminal.println("=======================================");
  terminal.println("|     System working normally...      |");
  terminal.println("=======================================");
  terminal.flush();

  // Initialize EEPROM
  EEPROM.begin(512); // Adjust the size according to your EEPROM size
}

void loop() {
  Blynk.run();
  updateUptime();
  checkTimers();  // Call the global millis function
  detectAndHandleCard();

  if (Serial.available()) {
    char option = Serial.read();
    switch (option) {
      case '1':
        addCard(storedUID, storedUIDSize);
        break;
      case '2':
        deleteCard(storedUID, storedUIDSize);
        break;
      case '3':
        aboutSystem();
        break;
      default:
        break;
    }
  }
}

BLYNK_CONNECTED() {
  // Restore switch states from Blynk app on connection
  Blynk.syncVirtual(V1, V2, V11, V12, V13, V14);
}

BLYNK_WRITE(V3) {
  myservo.write(param.asInt());
  terminal.println();
  if (param.asInt() > 90) {
    Blynk.logEvent("gate_override", "Alert: Gate has been Opened Mannually!");
    
    terminal.println("[ MANNUAL OVERRIDE: Gate Opened! ]");

    lcd.setCursor(0, 3);
    lcd.print("! MANNUAL OVERRIDE !");
    delay(1000);
    lcd.setCursor(0, 3);
    lcd.print("                   ");

  } else if (param.asInt() == 90) {
    Blynk.logEvent("gate_override", "Alert: Gate has been Closed Mannually!");
    
    terminal.println("[ MANNUAL OVERRIDE: Gate Closed! ]");
  }
}

BLYNK_WRITE(V4) {
  int addBtn = param.asInt(); //recieves inverted value.
  if (addBtn == 0) {
    addCard(storedUID, storedUIDSize);
  }
}

BLYNK_WRITE(V5) {
  int delBtn = param.asInt();
  if (delBtn == 0) {
    deleteCard(storedUID, storedUIDSize); // Pass the stored UID and UID size
  }
}

BLYNK_WRITE(V15) {
  remoteInput = param.asStr();

  if (remoteInput == String("update")) {
    terminal.clear();
    terminal.println();
    terminal.println("=======================================");
    terminal.println("IoT RFID Smart Parking & Slot Management System");
    terminal.println("=======================================");
    terminal.print("|  Uptime: ");
    terminal.print(millis() / 1000);  // Uptime in seconds
    terminal.println(" seconds                ");
    terminal.println("|                                     ");
    terminal.print("|  Slots Available: ");
    terminal.print(numSlotsAvailable());
    terminal.println("                    ");
    terminal.println("|                                     ");
    terminal.println("|        Stored Cards:                ");
    for (int i = 0; i < numStoredCards; i++) {
      Serial.print("|  - ");
      Serial.println(storedCards[i]);
    }
    terminal.println("|                                     ");
    terminal.println("=======================================");
    terminal.println("|     System working normally...      |");
    terminal.println("=======================================");
    terminal.println();
    terminal.flush();
  } else if (remoteInput == String("about")) {
    terminal.println();
    terminal.flush();
  }
  // Ensure everything is sent
  terminal.flush();
}

void updateUptime() {
  unsigned long currentMillis = millis();
  unsigned long uptimeSeconds = (currentMillis - uptimeStartTime) / 1000;

  // Update the uptime on Blynk app
  Blynk.virtualWrite(uptimePin, uptimeSeconds);
}


void checkTimers() {
  unsigned long currentTime = millis();

  if (currentTime - previousTime >= interval) {
    previousTime = currentTime;

    // Code to execute every 1 second
    updateSlotStatus();
    detectExit();
    //adminMenu();
  }
}

void lcdIntro() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("|Connecting to WiFi|");
  lcd.setCursor(0, 1);
  lcd.print("|------------------|");
}

void lcdScancard() {
  lcd.setCursor(0, 2);
  lcd.write(6);
  lcd.setCursor(2, 2);
  lcd.print("[Scan your Card]");
  lcd.setCursor(19, 2);
  lcd.write(6);
}

void cardAcceptedlcd() {
  lcd.setCursor(0, 2);
  lcd.write(7);
  lcd.setCursor(2, 2);
  lcd.print("[Card Accepted!]");
  lcd.setCursor(19, 2);
  lcd.write(7);
}

void cardDeclinedlcd() {
  lcd.setCursor(0, 2);
  lcd.print("X");
  lcd.setCursor(2, 2);
  lcd.print("[Card Declined!]");
  lcd.setCursor(19, 2);
  lcd.print("X");

  delay(1000);
}

void adminMenu() {
  Serial.println();
  Serial.println("=======================================");
  Serial.println("IoT RFID Smart Parking & Slot Management System");
  Serial.println("=======================================");
  Serial.println("|                                     ");
  Serial.println("|           Menu Options:             ");
  Serial.println("|                                     ");
  Serial.println("|  1. Add Card                        ");
  Serial.println("|  2. Delete Card                     ");
  Serial.println("|  3. About                           ");
  Serial.println("|                                     ");
  Serial.print("|  Uptime: ");
  Serial.print(millis() / 1000);  // Uptime in seconds
  Serial.println(" seconds                ");
  Serial.println("|                                     ");
  Serial.print("|  Slots Available: ");
  Serial.print(numSlotsAvailable());
  Serial.println("                    ");
  Serial.println("|                                     ");
  Serial.println("|        Stored Cards:                ");
  for (int i = 0; i < numStoredCards; i++) {
    Serial.print("|  - ");
    Serial.println(storedCards[i]);
  }
  Serial.println("|                                     ");
  Serial.println("=======================================");
  Serial.println("|     System working normally...      |");
  Serial.println("=======================================");
  Serial.println();
}

void aboutSystem() {
  Serial.println("[ IoT RFID Smart Parking & Slot Management System ]");
  Serial.println("Frimware Version: v2.2");
  Serial.println("Developed by mr-virk");
  Serial.println("Github: https://github.com/mr-virk");
}

void openGate() {
  if (numSlotsAvailable() > 0) {
    myservo.write(190);
    Blynk.virtualWrite(V2, HIGH);
    delay(2000);
    myservo.write(100);
    Blynk.virtualWrite(V2, LOW);
  } else {
    lcd.setCursor(0, 3);
    lcd.print("No Slots Available!");
    delay(1000);
    lcd.setCursor(0, 3);
    lcd.print("                   ");  // Clear the "No Slots Available!" line
  }
}

void initializePCF8574() {
  Wire.beginTransmission(PCF8574_ADDRESS);
  Wire.write(0xFF);  // Set all pins as input
  Wire.endTransmission();
}

int numSlotsAvailable() {
  int availableSlots = 0;

  for (int i = 0; i < 4; i++) {
    if (!slotStatus[i]) {
      availableSlots++;
    }
  }

  return availableSlots;
}

void updateSlotStatus() {
  Wire.requestFrom(PCF8574_ADDRESS, 1);
  uint8_t inputStatus = Wire.read();

  int numOccupiedSlots = 0;

  for (int i = 0; i < 4; i++) {
    bool isOccupied = !bitRead(inputStatus, IR_SENSOR_PINS[i]);
    slotStatus[i] = isOccupied;

    if (isOccupied) {
      numOccupiedSlots++;
    }
  }

  isSlotAvailable = (numOccupiedSlots < 4);

  if (!isSlotAvailable) {
    delay(1000);
    lcd.setCursor(0, 3);
    lcd.print("                     ");
  }

  lcd.setCursor(0, 0);
  lcd.print("Slot 1:");
  lcd.setCursor(8, 0);
  lcd.write(slotStatus[0] ? 6 : 7);
  if (slotStatus[0] == HIGH) {
    Blynk.virtualWrite(V11, HIGH);
  } else {
    Blynk.virtualWrite(V11, LOW);
  }

  lcd.setCursor(10, 0);
  lcd.print("Slot 2:");
  lcd.setCursor(18, 0);
  lcd.write(slotStatus[1] ? 6 : 7);
  if (slotStatus[1] == HIGH) {
    Blynk.virtualWrite(V12, HIGH);
  } else {
    Blynk.virtualWrite(V12, LOW);
  }

  lcd.setCursor(0, 1);
  lcd.print("Slot 3:");
  lcd.setCursor(8, 1);
  lcd.write(slotStatus[2] ? 6 : 7);
  if (slotStatus[2] == HIGH) {
    Blynk.virtualWrite(V13, HIGH);
  } else {
    Blynk.virtualWrite(V13, LOW);
  }

  lcd.setCursor(10, 1);
  lcd.print("Slot 4:");
  lcd.setCursor(18, 1);
  lcd.write(slotStatus[3] ? 6 : 7);
  if (slotStatus[3] == HIGH) {
    Blynk.virtualWrite(V14, HIGH);
  } else {
    Blynk.virtualWrite(V14, LOW);
  }
}

// Detect exit
void detectExit() {
  exitSensorVal = analogRead(exitSensor);

  if (exitSensorVal < 1024) {
    myservo.write(190);
    Blynk.virtualWrite(V2, HIGH);

    

    lcd.setCursor(0, 3);
    lcd.print("! Exit Gate Opened !");

    delay(2000);

    lcd.setCursor(0, 3);
    lcd.print("                    ");

    myservo.write(100);
    Blynk.virtualWrite(V2, LOW);
  }
  myservo.write(100);
}

//RFID and EEPROM
bool isCardStored(byte* uid, byte uidSize) {
  return findStoredCard(uid, uidSize) != -1;
}

int findStoredCard(byte * uid, byte uidSize) {
  for (int i = 0; i < EEPROM.length(); i++) {
    byte storedUidSize = EEPROM.read(i);
    if (storedUidSize == uidSize) {
      bool match = true;
      for (int j = 0; j < uidSize; j++) {
        if (EEPROM.read(i + 1 + j) != uid[j]) {
          match = false;
          break;
        }
      }
      if (match) {
        return i;
      }
    }
  }
  return -1; // Card not found
}

int findEmptySlot() {
  for (int i = 0; i < EEPROM.length(); i++) {
    if (EEPROM.read(i) == 0) {
      return i;
    }
  }
  return -1; // No empty slot found
}

void detectAndHandleCard() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    byte uidSize = mfrc522.uid.size;
    for (byte i = 0; i < uidSize; i++) {
      storedUID[i] = mfrc522.uid.uidByte[i];
    }
    storedUIDSize = uidSize;

    if (isCardStored(storedUID, storedUIDSize)) {
      cardAcceptedlcd();
      terminal.println();
      terminal.println("Card Accepted!");
      terminal.println("[ Access Granted ]");
      openGate();
      delay(1000);
      lcdScancard();
    } else {
      Serial.println("Card is not stored.");
      terminal.println();
      terminal.println("Card Declined!");
      terminal.println("[ Access Denied ]");
      cardDeclinedlcd();
    }

    lcdScancard();

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

void addCard(byte* uid, byte uidSize) {

  addCardToFlash(storedUID, storedUIDSize);

  Blynk.logEvent("card_added", "Alert: New Card been added!");

  lcd.setCursor(0, 3);
  lcd.print("|  > Card Added <  |");
  delay(1000);
  lcd.setCursor(0, 3);
  lcd.print("                   ");

  Serial.println("Card Added");
  terminal.println("Card Added");

  Serial.println("Session Completed!");
  terminal.println("Session Completed!");

  terminal.flush();
}


void deleteCard(byte* uid, byte uidSize) {

  removeCardFromFlash(uid, uidSize);

  Blynk.logEvent("card_deleted", "Alert: New Card been added!");

  lcd.setCursor(0, 3);
  lcd.print("| > Card Deleted < |");
  delay(1000);
  lcd.setCursor(0, 3);
  lcd.print("                   ");

  Serial.println("Card Deleted");
  terminal.println("Card Deleted");

  Serial.println("Session Completed!");
  terminal.println("Session Completed!");

  terminal.flush();
}

bool addCardToFlash(byte* uid, byte uidSize) {
  if (isCardStored(uid, uidSize)) {
    return false; // Card is already stored
  }

  int emptySlot = findEmptySlot();
  if (emptySlot == -1) {
    return false; // EEPROM is full
  }

  EEPROM.write(emptySlot, uidSize);
  for (int i = 0; i < uidSize; i++) {
    EEPROM.write(emptySlot + 1 + i, uid[i]);
  }
  EEPROM.commit();

  return true; // Card added successfully
}

bool removeCardFromFlash(byte* uid, byte uidSize) {
  int storedSlot = findStoredCard(uid, uidSize);
  if (storedSlot == -1) {
    return false; // Card is not stored
  }

  for (int i = 0; i < uidSize + 1; i++) {
    EEPROM.write(storedSlot + i, 0);
  }
  EEPROM.commit();

  return true; // Card removed successfully
}
