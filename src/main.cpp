#include <Arduino.h>
#include <RTClib.h>
#include <SD.h>
#include <Wiegand.h>

WIEGAND rfid;
RTC_DS3231 rtc;
DateTime timeToSync, timeToLog;
unsigned long timestamp = 1710012345;
String stickerId = "";            // General scanned RFID
String stickerIdToEnroll = "";    // RFID to be enrolled
String itemToEnroll = "";         // Item name to enroll
bool awaitingEnrollment = false;  // Flag to track ENROLL state
bool awaitingTimeSync = false;    // Flag to track time sync request
bool awaitingLogSync = false;    // Flag to track time sync request
bool isGoingOut = false;
bool isDoneEnrolling = false;
String enrollData;
const int chipSelect = 53;

unsigned long rfidNew = 0;
unsigned long rfidToEnroll = 0;
bool gotRfidScan = false;
bool savedRfidToEnroll = false;


const byte RFID_D0_PIN = 2;
const byte RFID_D1_PIN = 3;

struct itemList {
  unsigned long itemRfid;
  String itemName;
};

itemList aviationTools[] = {
  {273129501, "Torque Wrench"},
  {273129502, "Safety Wire Pliers"},
  {273129503, "Rivet Gun"},
};

// Define a struct for log entries
struct logEntry {
  String timestamp;
  String rfid;
  String itemName;
  String action;
};

logEntry entryToLog;

// Sample RFID log entries
String logEntries[] = {
  //"Timestamp,RFID #,Item Name,Action",
  "2025-03-09 16:51:09,E2801160600002A6951B,Pneumatic Riveter,Exit",
  "2025-03-09 22:13:09,E2801160600002D786C7,Cable Cutter,Exit",
  "2025-03-09 14:59:09,E280116060000242CB7C,Crimping Tool,Exit",
  "2025-03-10 00:43:09,E2801160600002BBFC11,Aircraft Drill,Entry",
  "2025-03-09 17:32:09,E280116060000273363E,Borescope,Entry",
  "2025-03-09 22:18:09,E28011606000027453EC,Safety Wire Pliers,Exit"
};

struct itemStatus {
  unsigned long itemRfid;
  bool isInside; // true = inside, false = outside
};





void setupRfid (){
  rfid.begin(RFID_D0_PIN, RFID_D1_PIN);
  Serial.println("RFID Reader active");
}

void scanRfidReader(unsigned long &_id) {
  unsigned long startTime = millis();  // Start time for timeout
  while (!rfid.available()) {
    if (millis() - startTime > 5000) {  // Timeout after 5 seconds
      Serial.println("⚠️ RFID scan timeout!");
      return;
    }
  }
  _id = rfid.getCode();
  Serial.print("✅ Scanned RFID: ");
  Serial.println(_id);
  gotRfidScan = true;
}






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
void scanRfidToenroll() {
  if(gotRfidScan){
    rfidToEnroll = rfidNew;
    Serial.print("RFID to enroll: ");
    Serial.println(rfidToEnroll);
  } else {
    scanRfidReader(rfidNew);
  }
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


// Function to determine whether the item enters or exits the door
String getAction(unsigned long _itemRfid) {

  File file = SD.open("itemStatus.txt"); // Open file in read mode

  if (!file) {
      Serial.println("Error opening itemStatus.txt");
      return "error"; // Return error if file cannot be opened
  }

  String rfidTarget = String(_itemRfid); // Convert RFID to string for comparison

  while (file.available()) {
      String line = file.readStringUntil('\n'); // Read one line

      int commaIndex = line.indexOf(','); // Find comma position
      if (commaIndex == -1) continue; // Skip malformed lines

      String rfidStr = line.substring(0, commaIndex); // Extract RFID (before comma)
      String status = line.substring(commaIndex + 1); // Extract status (after comma)
      
      rfidStr.trim(); // Remove spaces
      status.trim();  // Remove spaces & newline characters

      if (rfidStr.equals(rfidTarget)) { // Use string comparison
          file.close();
          return (status.equals("true")) ? "exit" : "entry"; // Return action
      }
  }

  file.close();
  return "not found"; // Return if RFID is not in the file
}


// Function to get the current time and return it as a formatted string
String getTime() {
  DateTime now = rtc.now(); // Get current time from RTC
  String _dateTime = String(now.year()) + "-" + 
              String(now.month()) + "-" + 
              String(now.day()) + " " + 
              String(now.hour()) + ":" + 
              String(now.minute()) + ":" + 
              String(now.second());

  Serial.print("Current Timestamp: " + _dateTime + " | "); // Debugging output
  return _dateTime;
}

// Function to prepare a log entry as a formatted string
String prepareLog() {
  entryToLog.timestamp = getTime();  // Get and format timestamp
  entryToLog.action = "Exit";  // Set action
  //entry.itemName = "Test Item";  // Set item name
  entryToLog.rfid = rfidToEnroll;  // Set RFID tag
  
  // Format log entry
  String logLine = entryToLog.timestamp + "," + entryToLog.rfid + "," + entryToLog.itemName + "," + entryToLog.action;
  
  // Print the log for debugging
  Serial.println("Prepared Log Entry: " + logLine);
  
  return logLine;
}

void writeLogs(){
  // Open a file for writing
  File dataFile = SD.open("data.txt", FILE_WRITE);

  // Check if the file is open
  if (dataFile) {
    Serial.println("Writing to test.txt...");
    dataFile.println(prepareLog());
    dataFile.close(); // Close the file
    Serial.println("Write done.");
  } else {
    Serial.println("Error opening test.txt");
  }
}

// 📌 Function to Process Enrollment Data
void processEnrollData(String data) {
  int commaIndex = data.indexOf(',');

  if (commaIndex != -1) {
    itemToEnroll = data.substring(0, commaIndex);
    stickerIdToEnroll = data.substring(commaIndex + 1);

    itemToEnroll.trim();  // Trim in place
    stickerIdToEnroll.trim();  // Trim in place
    
    entryToLog.itemName = itemToEnroll;  // Assign after trimming
    entryToLog.rfid = stickerIdToEnroll;  // Assign after trimming
    

    Serial.print("🛠️ Enrolling:");
    Serial.print(" Item: " + itemToEnroll);
    Serial.println(" | RFID: " + stickerIdToEnroll);

    writeLogs();

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

void enrollRfid(){
  if (awaitingEnrollment) {
    enrollData = Serial.readStringUntil('\n');
    processEnrollData(enrollData);
  } 
}
// 📌 Function to Handle Serial Commands
void processSerialCommand(String command) {
  command.trim();

  if (command == "SCAN_NOW") {  
    Serial.println("Scan RFID now...");
    scanRfidToenroll();
    rfidToEnroll = rfidNew;
    Serial.print("RFID_SCANNED: ");
    Serial.print(rfidToEnroll);
  } 
  else if (command == "ENROLL") {
    Serial.println("Received: ENROLL");
    Serial.println("Waiting for item name and RFID...");
    awaitingEnrollment = true;

  } 
  else if (command == "SYNC_TIME") {
    Serial.println("Received: SYNC_TIME. Send time in format YYYY-MM-DD HH:MM:SS");
    awaitingTimeSync = true;  // Wait for time input
  } 
  else if (command == "SYNC_LOGS") {
    Serial.println("Received: SYNC_LOG.");
    awaitingLogSync = true;  // Wait for time input
  } 
  else {
    Serial.println("❌ Unknown command.");
  }
}

void deleteFile(String _filename){
  SD.remove(_filename);
  Serial.print(_filename);
  Serial.println(" deleted.");
}

void readFile(String filename) {
  File file = SD.open(filename, FILE_WRITE); // Open file in read mode
  if (file) {
      Serial.println("Open success " + filename);
      while (file.available()) {
        Serial.write(file.read()); // Print each byte to Serial
      }
      file.close(); // Close the file after reading
  } else {
      Serial.println("Error opening " + filename);
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

  readFile("items.txt");
  readFile("logs.txt");
  readFile("itemStat.txt");

  //deleteFile("logs.txt");
  //deleteFile("items.txt");
  //deleteFile("itemStat.txt");
}

void addItem(itemList _item){

    // Open file for writing
    File itemsFile = SD.open("items.txt", FILE_WRITE);
    
    if (itemsFile) {
        Serial.println("Writing to items.txt");
        String lineWrite = _item.itemName + "," + String(_item.itemRfid);
        itemsFile.println(lineWrite); 
        itemsFile.close();
        Serial.println("Write successful.");
        readFile("items.txt");
    } else {
        Serial.println("Error opening items.txt");
    }
}

void addItemStatusToSd(itemStatus _status) {
  // Open file for appending
  File itemStatusFile = SD.open("itemStat.txt", FILE_WRITE);
  if (itemStatusFile) {
      Serial.println("Writing to itemStat.txt");
      // Convert itemRfid properly before concatenation
      String lineWrite = String(_status.itemRfid) + "," + (_status.isInside ? "true" : "false");
      itemStatusFile.println(lineWrite); 
      itemStatusFile.close();
      
      Serial.println("Write successful.");
  } else {
      Serial.println("Error opening itemStat.txt");
  }
}

void addItemStatus(){
  itemStatus sampleItem;
  sampleItem.itemRfid = 456456456;
  sampleItem.isInside = false;
  addItemStatusToSd(sampleItem);
}


void syncLogs() {
  if (awaitingLogSync) {
      Serial.println("START_LOGS");  // 🔥 Notify Python that logs are starting
      
      File logFile = SD.open("data.txt");  // Open the log file on SD card
      if (logFile) {
          while (logFile.available()) {
              Serial.println(logFile.readStringUntil('\n'));  // Send line by line
              delay(100);  // Prevent buffer overflow
          }
          logFile.close();
      } else {
          Serial.println("ERROR: Cannot open logs.txt");  // Error handling
      }

      Serial.println("END_LOGS");  // 🔥 Notify Python that logs are done
      awaitingLogSync = false;  // Reset flag
  }
}




void setup() {
  Serial.begin(9600);
  Serial3.begin(9600);
  Serial.println("🔹 RFID Tool Management System Ready");

  if (!rtc.begin()) {
    Serial.println("❌ Error: Couldn't find RTC module!");
    //while (1);
  }

  setupSdCard();
  setupRfid();

  /*TO ADD ITEM BY APPENDING A CSV STRING TO THE ITEMS.TXT FILE
  * Create instance of itemList struct
  * assign itemName and itemRfid of the instance
  * call addItem with the instance of the class as the argument
  */
  itemList newTool;
  newTool.itemName = "Caliper";
  newTool.itemRfid = 123123123;
  //addItem(newTool);

  addItemStatus();


}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    processSerialCommand(command);
  }

  // 🔹 Listen for time sync data separately
  receiveDateTime();
  enrollRfid();
  syncLogs();
  //scanRfidToenroll();

  
}
