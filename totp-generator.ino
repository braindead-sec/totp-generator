#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <RTC.h>
#include <TOTP.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

/*** GLOBALS ***/

#define SCREEN_WIDTH 128     // OLED display width
#define SCREEN_HEIGHT 32     // OLED display height
#define OLED_RESET -1        // OLED resets with Arduino reset
#define SCREEN_ADDRESS 0x3C  // OLED 2wire bus address

const uint8_t maxKeys = 2;     // Max number of keys to hold
const uint16_t maxKeyLen = 64; // Max key length, in bytes

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static DS3231 rtc;
uint8_t keysSet = 0;
boolean timeSet = false;

struct DataObj {
  uint16_t keyLen[maxKeys];
  uint8_t keyName[maxKeys][3];
  uint8_t key[maxKeys][maxKeyLen];
};
DataObj settings;

/*** FUNCTIONS ***/

String getTime() {
  String hr = String(rtc.getHours());
  String mn = String(rtc.getMinutes());
  String sc = String(rtc.getSeconds());
  if (hr.length() == 1) hr = "0"+hr;
  if (mn.length() == 1) mn = "0"+mn;
  if (sc.length() == 1) sc = "0"+sc;
  String now = "UTC"+hr+":"+mn+":"+sc;
  return now;
}

void setTime(uint32_t timestamp) {
  rtc.stopClock();
  rtc.setEpoch(timestamp);
  rtc.startClock();
  timeSet = true;
  Serial.print("RTC set to ");
  Serial.println(timestamp);
  printSmallOLED("Time synced!");
  delay(2000);
}

uint8_t getExpiration() {
  uint8_t secs = rtc.getSeconds();
  if (secs >= 30)
    secs = secs - 30;
  return (30 - secs);
}

void printCode(uint8_t keyId) {
  display.clearDisplay();
  display.setFont(&FreeSansBold9pt7b);
  display.setTextSize(2);
  
  TOTP totp = TOTP(settings.key[keyId], settings.keyLen[keyId]);
  char* code = totp.getCode(rtc.getEpoch());
  
  // Print first 3 digits
  display.setCursor(0,24);
  for (uint8_t i=0; i<3; i++)
    display.print(code[i]);
  
  // Print last 3 digits
  display.setCursor(64,24);
  for (uint8_t i=3; i<6; i++)
    display.print(code[i]);
  
  // Print countdown bar
  uint8_t expiry = getExpiration();
  uint8_t progress_length = (SCREEN_WIDTH / 30) * expiry;
  uint8_t progress_start = (SCREEN_WIDTH / 2) - (progress_length / 2);
  display.drawFastHLine(progress_start, SCREEN_HEIGHT - 1, progress_length, SSD1306_WHITE);
  display.display(); 
}

void printTwoCodes(uint8_t key1, uint8_t key2) {
  uint8_t i;
  display.clearDisplay();

  TOTP totp1 = TOTP(settings.key[key1], settings.keyLen[key1]);
  TOTP totp2 = TOTP(settings.key[key2], settings.keyLen[key2]);

  uint32_t timeNow = rtc.getEpoch();
  char* code1 = totp1.getCode(timeNow);
  char* code2 = totp2.getCode(timeNow);
  
  // Print code 1
  display.setTextSize(1);
  display.setFont(&FreeMono9pt7b);
  display.setCursor(0,14);
  for (i=0; i<3; i++)
    display.print((char)settings.keyName[key1][i]);
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(45,14);
  for (i=0; i<3; i++)
    display.print(code1[i]);
  display.setCursor(83,14);
  for (i=3; i<6; i++)
    display.print(code1[i]);

  // Print code 2
  display.setFont(&FreeMono9pt7b);
  display.setCursor(0,31);
  for (i=0; i<3; i++)
    display.print((char)settings.keyName[key2][i]);
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(45,31);
  for (i=0; i<3; i++)
    display.print(code2[i]);
  display.setCursor(83,31);
  for (i=3; i<6; i++)
    display.print(code2[i]);

  // Print countdown bar
  uint8_t expiry = getExpiration();
  uint8_t progress_length = (SCREEN_HEIGHT / 30) * expiry;
  display.drawFastVLine(SCREEN_WIDTH - 1, SCREEN_HEIGHT - progress_length, progress_length, SSD1306_WHITE);
  display.display();
}

void printSmallOLED(String text) {
  display.clearDisplay();
  display.setFont(&FreeMono9pt7b);
  display.setTextSize(1);
  display.setCursor(0,11);
  display.print(text);
  display.display();
}

uint8_t getKeyCount() {
  uint8_t numKeys = 0;
  for (uint8_t i=0; i<maxKeys; i++)
    if (settings.keyLen[i] > 0 && settings.keyLen[i] <= maxKeyLen)
      numKeys++;
  return numKeys;
}

void getCommand() {
  String code = Serial.readStringUntil('\n');
  
  if (code == "sync") {
    // Sync the time
    uint32_t newTime = Serial.readStringUntil('\n').toInt();
    setTime(newTime);
  
  } else if (code == "get") {
    // List key names (but not keys)
    // Print the number of keys available
    uint16_t numKeys = getKeyCount();
    Serial.println(numKeys, DEC);
    // Print the ID and name of each
    if (numKeys > 0) {
      for (uint16_t i=0; i<numKeys; i++) {
        Serial.print(i+1);
        Serial.print(": ");
        for (uint8_t j : settings.keyName[i])
          Serial.print(char(j));
        Serial.println();
      }
    }
  
  } else if (code == "set") {
    // Add a new HMAC key
    // Read key name from serial (3 chars)
    String newKeyName = Serial.readStringUntil('\n');
    // Read key from serial
    uint8_t newKey[maxKeyLen];
    uint16_t newKeyLen = Serial.readBytesUntil('\n', newKey, maxKeyLen+1);
    // Make sure there is a key slot available
    uint16_t numKeys = getKeyCount();
    if (numKeys >= maxKeys) {
      Serial.print("Only ");
      Serial.print(maxKeys, DEC);
      Serial.println(" keys allowed");
      return;
    }
    if (newKeyLen >= 10 && newKeyLen <= maxKeyLen) {
      // Update settings
      uint16_t i;
      for (i=0; i<3; i++)
        settings.keyName[numKeys][i] = newKeyName[i];
      settings.keyLen[numKeys] = newKeyLen;
      for (i=0; i<newKeyLen; i++)
        settings.key[numKeys][i] = newKey[i];
      while (i < maxKeyLen) {
        settings.key[numKeys][i] = 0xFF;
        i++;
      }
      EEPROM.put(0, settings);
      // Show result
      keysSet++;
      Serial.println("New key set successfully");
      printSmallOLED("New key set");
      delay(1500);
    } else {
      Serial.println("Invalid key length");
    }

  } else if (code == "del") {
    // Delete an HMAC key
    uint16_t numKeys = getKeyCount();
    // Get the key ID to delete
    uint16_t keyId = Serial.readStringUntil('\n').toInt();
    if (keyId >= numKeys) {
      Serial.println("Invalid key ID");
      return;
    }
    // Cascade remaining keys
    while ((keyId+1) < numKeys) {
      uint16_t i;
      settings.keyLen[keyId] = settings.keyLen[keyId+1];
      for (i=0; i<3; i++)
        settings.keyName[keyId][i] = settings.keyName[keyId+1][i];
      for (i=0; i<maxKeyLen; i++)
        settings.key[keyId][i] = settings.key[keyId+1][i];
      keyId++;
    }
    // Zero out the last available key
    settings.keyLen[keyId] = 0;
    for (uint8_t i : settings.keyName[keyId])
      i = 0xFF;
    for (uint8_t i : settings.key[keyId])
      i = 0xFF;
    // Update settings
    EEPROM.put(0, settings);
    // Show result
    keysSet--;
    Serial.println("Erased key");
    printSmallOLED("Key deleted");
    delay(1500);
 
  } else if (code == "wipe") {
    // Reset all settings to defaults
    for (uint16_t i=0; i<maxKeys; i++) {
      settings.keyLen[i] = 0;
      for (uint8_t j : settings.keyName[i]) 
        j = 0xFF;
      for (uint8_t j : settings.key[i]) 
        j = 0xFF;
    }
    EEPROM.put(0, settings);
    keysSet = 0;
    // Reset clock
    rtc.stopClock();
    rtc.setEpoch(0);
    timeSet = false;
    Serial.println("Settings reset");
    printSmallOLED("Settings\nreset");
    delay(2500);
    
  } else {
    // Error for unrecognized command
    Serial.println("Bad command received");
    printSmallOLED("Bad command\nreceived");
    delay(1500);
  }
}

/*** MAIN ***/

void setup() {
  
  // Init serial
  Serial.begin(115200);
  
  // Init OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED init failed");
    while(1);
  }
  display.setTextColor(SSD1306_WHITE);
  
  // Init real-time clock
  rtc.begin();
  rtc.setHourMode(CLOCK_H24);
  if (rtc.getEpoch() > 1640995200) // 1/1/2022
    timeSet = true;

  // Load settings from EEPROM
  EEPROM.get(0, settings);
  keysSet = getKeyCount();
  
}

void loop() {
  
  // Check for serial command
  if (Serial.available())
    getCommand();

  // Show MFA token (or error message)
  if (timeSet == false) {
    printSmallOLED("Connect to\nset clock...");
  } else if (keysSet == 2) {
      printTwoCodes(0, 1);
  } else if (keysSet == 1) {
      printCode(0);
  } else {
    printSmallOLED("Connect to\nset keys...");
  }
  
  // Repeat 5x/second
  // Faster RTC polling causes unexpected results
  delay(200);

}
