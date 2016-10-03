/*
  Turnstile data base and conrtoeler with SD card

  Pin Out
  Receiver:
  DATA Pin2

  SD Reader
  CS Pin4
  MOSI Pin11
  SCK Pin13
  MISO Pin12

  Relay1 Pin10
  Relay2 Pin9
*/

#include <RCSwitch.h>
#include <SPI.h>
#include <SD.h>

const int up_pin = 8;
const int down_pin = 9;

const int down_pin_active = 6;
const int up_pin_active = 7;

int pin_state = 0;

File dbFile;

RCSwitch plSwitch = RCSwitch();

const static String DB_PREFIX = "dbfiles/";
const static char DB_FILE[] = "id_db.txt";
const static unsigned long MASTER_ID1 = 7705666;
const static unsigned long MASTER_ID2 = 13489491;
const static unsigned long MASTER_ID3 = 1260819;
const static unsigned long MASTER_ID4 = 6247763;

bool write_aval = false;

unsigned long start_time;
unsigned long current_time;
unsigned long time_range;

/**
   Creates directory for data base
*/
void setupBaseDirecoty() {

  if (SD.mkdir(DB_PREFIX)) {
    Serial.println("Base dierectory created");
  } else {
    Serial.println("Could not create base dierectory");
  }
}

/**
   Removes invalid data base directory and re - creates new
*/
void clearBaseDirectory() {

  File dir = SD.open(DB_PREFIX, O_CREAT | O_WRITE);
  if (dir.isDirectory()) {
    Serial.println("Base dierectory exists");
  } else {
    SD.remove(DB_PREFIX);
    setupBaseDirecoty();
  }
  dir.close();
}

/**
   Closes all pins
*/
void closeAllPins() {

  digitalWrite(up_pin, HIGH);
  digitalWrite(down_pin, HIGH);
}

/**
   Initializes base directory for data
*/
void initBaseDirectory() {

  if (SD.exists(DB_PREFIX)) {
    clearBaseDirectory();
  } else {
    setupBaseDirecoty();
  }
}

/**
   Initializes SD connection and dcreates data base diorectory
*/
void setup() {
  Serial.begin(9600);
  plSwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
  Serial.println(DB_PREFIX);
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  initBaseDirectory();
  Serial.println("initialization done.");
  pinMode(up_pin, OUTPUT);
  pinMode(down_pin, OUTPUT);
  pinMode(down_pin_active, INPUT);
  pinMode(up_pin_active, INPUT);
  closeAllPins();
  delay(500);
}

/**
   Generates DB file name
*/
String generateDBFile(unsigned long value) {
  return (DB_PREFIX + value);
}

/**
   Scans database for passed ID value and return true or false respectivaly
*/
bool readDB(unsigned long value) {

  bool valid;

  String valueText = generateDBFile(value);
  Serial.println(valueText);
  valid = SD.exists(valueText);

  return valid;
}

/**
   Writes record to database
*/
void writeToDB(unsigned long value) {

  String valueText = generateDBFile(value);
  dbFile = SD.open(valueText, O_CREAT | O_WRITE);
  dbFile.close();
}

/**
   Opens - closes - stops turnstile
*/
void turnTurnstile(bool up) {

  if (up) {
    if (digitalRead(up_pin) == LOW) {
      Serial.println("Up turned off");
      digitalWrite(up_pin, HIGH);
      digitalWrite(down_pin, HIGH);
    } else if (digitalRead(up_pin) == HIGH) {
      Serial.println("Up turned on");
      digitalWrite(up_pin, LOW);
      digitalWrite(down_pin, HIGH);
    }
  } else {
    if (digitalRead(down_pin) == LOW) {
      Serial.println("Down turned off");
      digitalWrite(down_pin, HIGH);
      digitalWrite(up_pin, HIGH);
    } else if (digitalRead(down_pin) == HIGH) {
      Serial.println("Down turned on");
      digitalWrite(down_pin, LOW);
      digitalWrite(up_pin, HIGH);
    }
  }
}

/**
   Opens turnstile up / down
*/
void openByPin() {

  Serial.print("Down pin - ");
  Serial.println(digitalRead(down_pin_active));
  Serial.print("Up pin - ");
  Serial.println(digitalRead(up_pin_active));
  if (digitalRead(down_pin_active) == HIGH && digitalRead(up_pin_active) == LOW) {
    Serial.print("1 if is here");
    turnTurnstile(false);
  } else if (digitalRead(up_pin_active) == HIGH && digitalRead(down_pin_active) == LOW) {
    Serial.print("2 if is here");
    turnTurnstile(true);
  } else if ((digitalRead(up_pin_active) == HIGH && digitalRead(down_pin_active) == HIGH) || (digitalRead(up_pin_active) == LOW && digitalRead(down_pin_active) == LOW)) {
    if (digitalRead(up_pin) == LOW) {
      Serial.print("3 if is here");
      digitalWrite(up_pin, HIGH);
      digitalWrite(down_pin, HIGH);
    } else if (digitalRead(down_pin) == LOW) {
      Serial.print("4 if is here");
      digitalWrite(down_pin, HIGH);
      digitalWrite(up_pin, HIGH);
    }
  }
}

/**
   Validates master identifier
*/
bool validMaster(unsigned long value) {
  return (value == MASTER_ID1 || value == MASTER_ID2 || value == MASTER_ID3 || value == MASTER_ID4);
}

/**
   Validates user and opens device
*/
unsigned long readPl() {

  unsigned long value;

  if (plSwitch.available() > 0) {
    value = plSwitch.getReceivedValue();
    String valueText =  String(value, DEC);
    Serial.println(valueText);
    Serial.println("==========");
    if (validMaster(value)) {
      start_time = millis();
      write_aval = true;
    } else {
      bool valid = readDB(value);
      Serial.print("Data base scanned with: ");
      Serial.println(valid);
      if (write_aval) {
        current_time = millis();
        Serial.println("recording time");
        Serial.println(current_time);
        Serial.println(start_time);
        Serial.println("time range");
        time_range = current_time - start_time;
        //Serial.println(time_range);
        if (!valid && (start_time > 0 && time_range < 10000)) {
          writeToDB(value);
        }
        write_aval = false;
      } else {
        delay(300);
        start_time = 0;
        current_time = 0;
        if (valid) {
          openByPin();
        }
      }
    }
    plSwitch.resetAvailable();
  } else {
    value = -1;
  }

  return value;
}

void loop() {
  unsigned long value = readPl();
}