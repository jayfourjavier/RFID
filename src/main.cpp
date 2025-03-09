#include <Arduino.h>
#include <RTClib.h>
#include <SD.h>

RTC_DS3231 rtc;
DateTime timeToSync;

unsigned long timestamp = 1710012345;
String stickerId = "";            // General scanned RFID
String stickerIdToEnroll = "";    // RFID to be enrolled
String itemToEnroll = "";         // Item name to enroll
bool awaitingEnrollment = false;  // Flag to track ENROLL state
bool awaitingTimeSync = false;    // Flag to track time sync request
String enrollData;
const int chipSelect = 53;

// 📌 Simulated function for general RFID scanning
String scanRfid() {
  return "E280116060000205189A3B2C";  // Simulated RFID tag
}

// 📌 Function to Sync RTC Time
void syncRTC(DateTime _dateTime) {
    rtc.adjust(_dateTime);  // Sync RTC
    Serial.print("✅ RTC Synced to: ");
    Serial.print(_dateTime.year()); Serial.print("-");
    Serial.print(_dateTime.month()); Serial.print("-");
    Serial.print(_dateTime.day()); Serial.print(" ");
    Serial.print(_dateTime.hour()); Serial.print(":");
    Serial.print(_dateTime.minute()); Serial.print(":");
    Serial.println(_dateTime.second());
}

// 📌 Function to Receive & Store DateTime from Serial
void receiveDateTime() {
  if (awaitingTimeSync && Serial.available()) {
    String dateTimeString = Serial.readStringUntil('\n');
    dateTimeString.trim();

    // 🔹 Extract Values (YYYY-MM-DD HH:MM:SS)
    int year = dateTimeString.substring(0, 4).toInt();
    int month = dateTimeString.substring(5, 7).toInt();
    int day = dateTimeString.substring(8, 10).toInt();
    int hour = dateTimeString.substring(11, 13).toInt();
    int minute = dateTimeString.substring(14, 16).toInt();
    int second = dateTimeString.substring(17, 19).toInt();

    // 🔸 Convert to DateTime Object & Sync RTC
    timeToSync = DateTime(year, month, day, hour, minute, second);
    syncRTC(timeToSync); // 🔹 Store & Sync RTC

    awaitingTimeSync = false;  // Reset sync flag
  }
}

// 📌 Function to Scan & Store RFID for Enrollment
void scanRfidToEnroll() {
  stickerIdToEnroll = scanRfid();
  Serial.println("RFID_SCANNED:" + stickerIdToEnroll);
}

// 📌 Function to Handle Actual RFID Enrollment
bool enrollRfid(String _item, String _rfid) {
  if (_item.length() > 0 && _rfid.length() > 0) {
    Serial.println("✅ ENROLL_SUCCESSFUL");
    return true;
  } else {
    Serial.println("❌ Enrollment failed. Missing data.");
    return false;
  }
}

// 📌 Function to Process Enrollment Data
void processEnrollData(String data) {
  int commaIndex = data.indexOf(',');

  if (commaIndex != -1) {
    itemToEnroll = data.substring(0, commaIndex);
    stickerIdToEnroll = data.substring(commaIndex + 1);

    itemToEnroll.trim();
    stickerIdToEnroll.trim();

    Serial.print("🛠️ Enrolling:");
    Serial.print(" Item: " + itemToEnroll);
    Serial.println(" | RFID: " + stickerIdToEnroll);

    // Perform Enrollment
    bool success = enrollRfid(itemToEnroll, stickerIdToEnroll);
    if (!success) {
      Serial.println("❌ Enrollment Failed. Try Again.");
    }
  } else {
    Serial.println("❌ Invalid format. Expected:");
    Serial.println("ENROLL");
    Serial.println("itemname,rfid");
  }

  awaitingEnrollment = false;  // Reset flag
}

void enroll(){
  if (awaitingEnrollment) {
    enrollData = Serial.readStringUntil('\n');
    processEnrollData(enrollData);
  } 
}
// 📌 Function to Handle Serial Commands
void processSerialCommand(String command) {
  command.trim();

  if (command == "SCAN_NOW") {
    Serial.println("🔍 Scan RFID now...");
    stickerId = scanRfid();
    Serial.println("RFID_SCANNED:" + stickerId);
  } 
  else if (command == "ENROLL") {
    Serial.println("📥 Received: ENROLL");
    Serial.println("Waiting for item name and RFID...");
    awaitingEnrollment = true;
  } 
  else if (command == "SYNC_TIME") {
    Serial.println("⏳ Received: SYNC_TIME. Send time in format YYYY-MM-DD HH:MM:SS");
    awaitingTimeSync = true;  // Wait for time input
  } 
  else {
    Serial.println("❌ Unknown command.");
  }
}

void setupSdCard (){
  // Initialize SD card
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Initialization failed!");
    return;
  }
  Serial.println("Initialization done.");

  SD.remove("test.txt");

  // Open a file for writing
  File dataFile = SD.open("test.txt", FILE_WRITE);

  // Check if the file is open
  if (dataFile) {
    Serial.println("Writing to test.txt...");
    dataFile.println("true");
    dataFile.close(); // Close the file
    Serial.println("Write done.");
  } else {
    Serial.println("Error opening test.txt");
  }

  // Now read the file back
  Serial.println("Reading from test.txt...");
  dataFile = SD.open("test.txt");
  if (dataFile) {
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close(); // Close the file
    Serial.println("\nRead done.");
  } else {
    Serial.println("Error opening test.txt");
  }

}

void setup() {
  Serial.begin(9600);
  Serial.println("🔹 RFID Tool Management System Ready");

  if (!rtc.begin()) {
    Serial.println("❌ Error: Couldn't find RTC module!");
    //while (1);
  }

  setupSdCard();

}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    processSerialCommand(command);
  }

  // 🔹 Listen for time sync data separately
  receiveDateTime();
  enroll();

  // Debugging timestamp
  //Serial.println(timestamp);
  //delay(1000);
}
