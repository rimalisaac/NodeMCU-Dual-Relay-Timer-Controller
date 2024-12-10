#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// WiFi credentials
const char* ssid = "Your SSID";
const char* password = "Your Password";

// Define relay pins
const int RELAY1_PIN = D3;
const int RELAY2_PIN = D4;

// EEPROM addresses
const int EEPROM_SIZE = 512;
const int ADDR_RELAY1_ON = 0;
const int ADDR_RELAY1_OFF = 20;
const int ADDR_RELAY2_ON = 40;
const int ADDR_RELAY2_OFF = 60;
const int ADDR_SCHEDULE_STATE = 80;
bool midnightAutoResetEnabled = false;
const int ADDR_MIDNIGHT_RESET = 100; // New EEPROM address for midnight reset state


// Time storage
String relay1OnTime = "00:00";
String relay1OffTime = "00:00";
String relay2OnTime = "00:00";
String relay2OffTime = "00:00";
bool scheduleEnabled = true;

// NTP Client setup for IST
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
const long gmtOffset_sec = 19800; // IST offset of 5:30 hours in seconds

// Create web server object
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  
  // Set relay pins as outputs
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // Set initial relay states to OFF
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  
  // Load saved times and schedule state from EEPROM
  loadTimesFromEEPROM();
  scheduleEnabled = EEPROM.read(ADDR_SCHEDULE_STATE) == 1;
  midnightAutoResetEnabled = EEPROM.read(ADDR_MIDNIGHT_RESET) == 1;
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize NTP Client
  timeClient.begin();
  timeClient.setTimeOffset(gmtOffset_sec);

  // Define server routes
  server.on("/", handleRoot);
  server.on("/setRelay1Times", HTTP_POST, handleRelay1Times);
  server.on("/setRelay2Times", HTTP_POST, handleRelay2Times);
  server.on("/relay1on", handleRelay1On);
  server.on("/relay1off", handleRelay1Off);
  server.on("/relay2on", handleRelay2On);
  server.on("/relay2off", handleRelay2Off);
  server.on("/enableSchedule", handleEnableSchedule);
  server.on("/disableSchedule", handleDisableSchedule);
  // In setup(), add these routes
  server.on("/enableMidnightReset", handleEnableMidnightReset);
  server.on("/disableMidnightReset", handleDisableMidnightReset);

  server.begin();
}

void loop() {
  server.handleClient();
  timeClient.update();
  checkScheduledTasks();
  delay(1000);
}

void handleRoot() {
  String currentTime = timeClient.getFormattedTime();
  
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial; text-align: center; margin: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; }";
  html += ".button { background-color: #4CAF50; color: white; padding: 10px 20px; ";
  html += "border: none; border-radius: 4px; cursor: pointer; margin: 5px; }";
  html += ".button:hover { background-color: #45a049; }";
  html += ".button.red { background-color: #f44336; }";
  html += ".button.red:hover { background-color: #da190b; }";
  html += ".button.blue { background-color: #2196F3; }";
  html += ".button.blue:hover { background-color: #0b7dda; }";
  html += ".input-group { margin: 20px 0; padding: 20px; background-color: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += "input[type='time'] { padding: 8px; margin: 5px; border: 1px solid #ddd; border-radius: 4px; }";
  html += ".status { font-weight: bold; margin: 10px 0; }";
  html += ".time-display { background: #f8f9fa; padding: 15px; border-radius: 8px; margin: 15px 0; }";
  html += ".current-time { font-size: 1.4em; color: #333; font-weight: bold; }";
  html += ".current-date { font-size: 1.1em; color: #666; margin-top: 5px; }";
  html += ".schedule-status { font-size: 1.1em; color: " + String(scheduleEnabled ? "#4CAF50" : "#f44336") + "; margin: 10px 0; }";
  html += "</style>";
  
  // Add JavaScript for auto-updating time
  html += "<script>";
  html += "function updateDateTime() {";
  html += "  const now = new Date();";
  html += "  const timeOptions = {";
  html += "    timeZone: 'Asia/Kolkata',";
  html += "    hour12: false,";
  html += "    hour: '2-digit',";
  html += "    minute: '2-digit',";
  html += "    second: '2-digit'";
  html += "  };";
  html += "  const dateOptions = {";
  html += "    timeZone: 'Asia/Kolkata',";
  html += "    weekday: 'long',";
  html += "    year: 'numeric',";
  html += "    month: 'long',";
  html += "    day: 'numeric'";
  html += "  };";
  html += "  document.getElementById('current-time').innerHTML = now.toLocaleTimeString('en-US', timeOptions);";
  html += "  document.getElementById('current-date').innerHTML = now.toLocaleDateString('en-US', dateOptions);";
  html += "}";
  html += "setInterval(updateDateTime, 1000);";
  html += "document.addEventListener('DOMContentLoaded', updateDateTime);";
  html += "</script>";
  html += "</head><body>";
  
  html += "<div class='container'>";
  html += "<h1>NodeMCU Relay Control</h1>";
  
  html += "<div class='time-display'>";
  html += "<div class='current-time'>Time (IST): <span id='current-time'>" + currentTime + "</span></div>";
  html += "<div class='current-date'><span id='current-date'></span></div>";
  html += "</div>";
  
  html += "<div class='schedule-status'>Schedule Status: " + String(scheduleEnabled ? "Enabled" : "Disabled") + "</div>";
  
  // Schedule Control
  html += "<div class='input-group'>";
  html += "<h2>Schedule Control</h2>";
  if (scheduleEnabled) {
    html += "<a href='/disableSchedule'><button class='button red'>Disable Auto Schedule</button></a>";
  } else {
    html += "<a href='/enableSchedule'><button class='button blue'>Enable Auto Schedule</button></a>";
  }
  html += "</div>";
  // Modify the handleRoot() function to add the midnight reset control
// Add this after the schedule control section:
html += "<div class='input-group'>";
html += "<h2>Midnight Auto-Reset</h2>";
html += "<p>Automatically turn off all relays at midnight</p>";
if (midnightAutoResetEnabled) {
    html += "<a href='/disableMidnightReset'><button class='button red'>Disable Midnight Reset</button></a>";
} else {
    html += "<a href='/enableMidnightReset'><button class='button blue'>Enable Midnight Reset</button></a>";
}
html += "<div class='status'>Status: " + String(midnightAutoResetEnabled ? "Enabled" : "Disabled") + "</div>";
html += "</div>";
  // Relay 1 Controls
  html += "<div class='input-group'>";
  html += "<h2>Relay 1</h2>";
  html += "<form action='/setRelay1Times' method='POST'>";
  html += "<div>ON Time: <input type='time' name='relay1on' value='" + relay1OnTime + "'></div>";
  html += "<div>OFF Time: <input type='time' name='relay1off' value='" + relay1OffTime + "'></div>";
  html += "<input type='submit' class='button' value='Save Relay 1 Times'>";
  html += "</form>";
  html += "<div class='status'>Status: " + String(digitalRead(RELAY1_PIN) ? "ON" : "OFF") + "</div>";
  html += "<a href='/relay1on'><button class='button'>Turn ON</button></a>";
  html += "<a href='/relay1off'><button class='button'>Turn OFF</button></a>";
  html += "</div>";
  
  // Relay 2 Controls
  html += "<div class='input-group'>";
  html += "<h2>Relay 2</h2>";
  html += "<form action='/setRelay2Times' method='POST'>";
  html += "<div>ON Time: <input type='time' name='relay2on' value='" + relay2OnTime + "'></div>";
  html += "<div>OFF Time: <input type='time' name='relay2off' value='" + relay2OffTime + "'></div>";
  html += "<input type='submit' class='button' value='Save Relay 2 Times'>";
  html += "</form>";
  html += "<div class='status'>Status: " + String(digitalRead(RELAY2_PIN) ? "ON" : "OFF") + "</div>";
  html += "<a href='/relay2on'><button class='button'>Turn ON</button></a>";
  html += "<a href='/relay2off'><button class='button'>Turn OFF</button></a>";
  html += "</div>";
  
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleRelay1Times() {
  if (server.hasArg("relay1on") && server.hasArg("relay1off")) {
    relay1OnTime = server.arg("relay1on");
    relay1OffTime = server.arg("relay1off");
    writeStringToEEPROM(ADDR_RELAY1_ON, relay1OnTime);
    writeStringToEEPROM(ADDR_RELAY1_OFF, relay1OffTime);
    EEPROM.commit();
    Serial.println("Relay 1 times updated - ON: " + relay1OnTime + " OFF: " + relay1OffTime);
  }
  handleRoot();
}

void handleRelay2Times() {
  if (server.hasArg("relay2on") && server.hasArg("relay2off")) {
    relay2OnTime = server.arg("relay2on");
    relay2OffTime = server.arg("relay2off");
    writeStringToEEPROM(ADDR_RELAY2_ON, relay2OnTime);
    writeStringToEEPROM(ADDR_RELAY2_OFF, relay2OffTime);
    EEPROM.commit();
    Serial.println("Relay 2 times updated - ON: " + relay2OnTime + " OFF: " + relay2OffTime);
  }
  handleRoot();
}

void handleRelay1On() {
  digitalWrite(RELAY1_PIN, HIGH);
  Serial.println("Relay 1 turned ON manually");
  handleRoot();
}

void handleRelay1Off() {
  digitalWrite(RELAY1_PIN, LOW);
  Serial.println("Relay 1 turned OFF manually");
  handleRoot();
}

void handleRelay2On() {
  digitalWrite(RELAY2_PIN, HIGH);
  Serial.println("Relay 2 turned ON manually");
  handleRoot();
}

void handleRelay2Off() {
  digitalWrite(RELAY2_PIN, LOW);
  Serial.println("Relay 2 turned OFF manually");
  handleRoot();
}

void handleEnableSchedule() {
  scheduleEnabled = true;
  EEPROM.write(ADDR_SCHEDULE_STATE, 1);
  EEPROM.commit();
  Serial.println("Schedule enabled");
  handleRoot();
}

void handleDisableSchedule() {
  scheduleEnabled = false;
  EEPROM.write(ADDR_SCHEDULE_STATE, 0);
  EEPROM.commit();
  Serial.println("Schedule disabled");
  handleRoot();
}

// Add these new handler functions
void handleEnableMidnightReset() {
  midnightAutoResetEnabled = true;
  EEPROM.write(ADDR_MIDNIGHT_RESET, 1);
  EEPROM.commit();
  Serial.println("Midnight auto-reset enabled");
  handleRoot();
}

void handleDisableMidnightReset() {
  midnightAutoResetEnabled = false;
  EEPROM.write(ADDR_MIDNIGHT_RESET, 0);
  EEPROM.commit();
  Serial.println("Midnight auto-reset disabled");
  handleRoot();
}

void saveTimesToEEPROM() {
  writeStringToEEPROM(ADDR_RELAY1_ON, relay1OnTime);
  writeStringToEEPROM(ADDR_RELAY1_OFF, relay1OffTime);
  writeStringToEEPROM(ADDR_RELAY2_ON, relay2OnTime);
  writeStringToEEPROM(ADDR_RELAY2_OFF, relay2OffTime);
  EEPROM.commit();
  Serial.println("All times saved to EEPROM");
}

void loadTimesFromEEPROM() {
  relay1OnTime = readStringFromEEPROM(ADDR_RELAY1_ON);
  relay1OffTime = readStringFromEEPROM(ADDR_RELAY1_OFF);
  relay2OnTime = readStringFromEEPROM(ADDR_RELAY2_ON);
  relay2OffTime = readStringFromEEPROM(ADDR_RELAY2_OFF);
  Serial.println("Times loaded from EEPROM");
}

void writeStringToEEPROM(int addr, String str) {
  int len = str.length();
  for (int i = 0; i < len; i++) {
    EEPROM.write(addr + i, str[i]);
  }
  EEPROM.write(addr + len, '\0');
}

String readStringFromEEPROM(int addr) {
  String str = "";
  char c;
  while ((c = EEPROM.read(addr)) != '\0' && addr < EEPROM_SIZE) {
    str += c;
    addr++;
  }
  return str.length() > 0 ? str : "00:00";
}

void checkScheduledTasks() {
  if (!scheduleEnabled) return;
  
  String currentTime = timeClient.getFormattedTime().substring(0, 5);
  
  // Reset at midnight
  if (currentTime == "00:00") {
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    Serial.println("Midnight reset - all relays turned OFF");
    return;
  }
  
  // Check and update relay states based on schedule
  if (currentTime == relay1OnTime) {
    digitalWrite(RELAY1_PIN, HIGH);
    Serial.println("Relay 1 turned ON by schedule");
  }
  if (currentTime == relay1OffTime) {
    digitalWrite(RELAY1_PIN, LOW);
    Serial.println("Relay 1 turned OFF by schedule");
  }
  if (currentTime == relay2OnTime) {
    digitalWrite(RELAY2_PIN, HIGH);
    Serial.println("Relay 2 turned ON by schedule");
  }
  if (currentTime == relay2OffTime) {
    digitalWrite(RELAY2_PIN, LOW);
    Serial.println("Relay 2 turned OFF by schedule");
  }
}

// Optional: Add WiFi reconnection handling
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nReconnected to WiFi");
      Serial.println("IP address: " + WiFi.localIP().toString());
    } else {
      Serial.println("\nFailed to reconnect. Restarting...");
      ESP.restart();
    }
  }
}

