#include <Arduino.h>
#include <RTClib.h>
#include <SD.h>

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
String enrollData;
const int chipSelect = 53;

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
void scanRfidToenrollRfid() {
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
  //entry.rfid = "E2801160600002D786C7";  // Set RFID tag
  
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
    stickerId = scanRfid();
    Serial.println("RFID_SCANNED:" + stickerId);
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

void setupSdCard (){
  // Initialize SD card
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Initialization failed!");
    return;
  }
  Serial.println("Initialization done.");

  //SD.remove("data.txt");

  /* Check if the file is open
  // Open a file for writing
  File dataFile = SD.open("data.txt", FILE_WRITE);

  if (dataFile) {
    Serial.println("Writing to test.txt...");
       // Loop through each entry and write to the file
       for (unsigned long i = 0; i < sizeof(logEntries) / sizeof(logEntries[0]); i++) {
        dataFile.println(logEntries[i]);
        Serial.println(logEntries[i]);  // Print to Serial Monitor
      }

    dataFile.close(); // Close the file
    Serial.println("Write done.");
  } else {
    Serial.println("Error opening test.txt");
  } 
    */

  // Now read the file back
  Serial.println("Reading from logs.txt...");
  File dataFile = SD.open("data.txt");
  if (dataFile) {
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close(); // Close the file
    Serial.println("\nRead done.");
  } else {
    Serial.println("Error opening data.txt");
  }

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
  enrollRfid();
  syncLogs();

  // Debugging timestamp
  //Serial.println(prepareLog());
  //writeLogs();
  delay(1000);
}
