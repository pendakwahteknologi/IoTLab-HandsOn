#include <ESP8266WiFi.h>      // Library for managing WiFi connections on the ESP8266
#include <ESP8266WebServer.h> // Library for creating a web server on the ESP8266

// WiFi credentials
char ssid[] = "YourSSID";      // Replace with your WiFi SSID
char pass[] = "YourPassword";  // Replace with your WiFi password

// Relay pins
const int relay1Pin = D6;      // GPIO pin for relay 1
const int relay2Pin = D7;      // GPIO pin for relay 2

// Relay states
bool relay1State = false;      // State of relay 1 (false = off, true = on)
bool relay2State = false;      // State of relay 2 (false = off, true = on)

// Web server
ESP8266WebServer server(80);
String consoleLog = "";        // Log for tracking web interface actions

// Handle the root web page
void handleRoot() {
  String html = "<html><head><title>ESP8266 Control</title>";
  html += "<style>";
  html += "body { font-family: monospace; text-align: center; background-color: #282c34; color: white; }";
  html += ".relay-box { border: 2px solid #444; border-radius: 10px; padding: 20px; margin: 20px; display: inline-block; }";
  html += "button { padding: 10px 20px; margin: 10px; font-size: 16px; cursor: pointer; background-color: #444; color: white; border: none; border-radius: 5px; }";
  html += "button.active { background-color: #888; }";
  html += ".ascii-art { font-size: 12px; line-height: 1; }";
  html += ".console { margin-top: 20px; padding: 10px; background-color: #333; border-radius: 10px; text-align: left; }";
  html += "</style></head><body>";
  html += "<h1>ESP8266 Control</h1>";
  html += "<pre class='ascii-art'>    _  _\n";
  html += "  _| || |_ \n";
  html += " |_  __  _| \n";
  html += "  _|| || |_ \n";
  html += " |_  __  _| \n";
  html += "   |_||_|   </pre>";

  // Relay 1 control
  html += "<div class='relay-box'>";
  html += "<p id='relay1Status'>Relay 1 is " + String(relay1State ? "ON" : "OFF") + "</p>";
  html += "<button id='onButton1' onclick=\"toggleRelay('relay1', 'on')\">Turn ON Relay 1</button>";
  html += "<button id='offButton1' onclick=\"toggleRelay('relay1', 'off')\">Turn OFF Relay 1</button>";
  html += "</div>";

  // Relay 2 control
  html += "<div class='relay-box'>";
  html += "<p id='relay2Status'>Relay 2 is " + String(relay2State ? "ON" : "OFF") + "</p>";
  html += "<button id='onButton2' onclick=\"toggleRelay('relay2', 'on')\">Turn ON Relay 2</button>";
  html += "<button id='offButton2' onclick=\"toggleRelay('relay2', 'off')\">Turn OFF Relay 2</button>";
  html += "</div>";

  html += "<div class='console' id='consoleLog'>" + consoleLog + "</div>";
  html += "<script>";
  html += "function toggleRelay(relay, action) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/' + relay + '/' + action, true);";
  html += "  xhr.onreadystatechange = function () {";
  html += "    if (xhr.readyState == 4 && xhr.status == 200) {";
  html += "      updateStatus(relay, action);";
  html += "      updateConsole();";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "function updateStatus(relay, action) {";
  html += "  if (relay === 'relay1') {";
  html += "    document.getElementById('relay1Status').innerHTML = 'Relay 1 is ' + (action === 'on' ? 'ON' : 'OFF');";
  html += "    document.getElementById('onButton1').classList.toggle('active', action === 'on');";
  html += "    document.getElementById('offButton1').classList.toggle('active', action === 'off');";
  html += "  } else if (relay === 'relay2') {";
  html += "    document.getElementById('relay2Status').innerHTML = 'Relay 2 is ' + (action === 'on' ? 'ON' : 'OFF');";
  html += "    document.getElementById('onButton2').classList.toggle('active', action === 'on');";
  html += "    document.getElementById('offButton2').classList.toggle('active', action === 'off');";
  html += "  }";
  html += "}";
  html += "function updateConsole() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/console', true);";
  html += "  xhr.onreadystatechange = function () {";
  html += "    if (xhr.readyState == 4 && xhr.status == 200) {";
  html += "      document.getElementById('consoleLog').innerHTML = xhr.responseText;";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "setInterval(function() { updateStatus('relay1', relay1State ? 'on' : 'off'); }, 1000);";
  html += "setInterval(function() { updateStatus('relay2', relay2State ? 'on' : 'off'); }, 1000);";
  html += "setInterval(updateConsole, 1000);"; // Update console log every second
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Handle turning relay 1 on
void handleRelay1On() {
  digitalWrite(relay1Pin, LOW); // LOW turns the relay on
  relay1State = true;
  Serial.println("Relay 1 turned ON");
  consoleLog += "Relay 1 turned ON<br>";
  server.send(200, "text/plain", "Relay 1 is ON");
}

// Handle turning relay 1 off
void handleRelay1Off() {
  digitalWrite(relay1Pin, HIGH); // HIGH turns the relay off
  relay1State = false;
  Serial.println("Relay 1 turned OFF");
  consoleLog += "Relay 1 turned OFF<br>";
  server.send(200, "text/plain", "Relay 1 is OFF");
}

// Handle turning relay 2 on
void handleRelay2On() {
  digitalWrite(relay2Pin, LOW); // LOW turns the relay on
  relay2State = true;
  Serial.println("Relay 2 turned ON");
  consoleLog += "Relay 2 turned ON<br>";
  server.send(200, "text/plain", "Relay 2 is ON");
}

// Handle turning relay 2 off
void handleRelay2Off() {
  digitalWrite(relay2Pin, HIGH); // HIGH turns the relay off
  relay2State = false;
  Serial.println("Relay 2 turned OFF");
  consoleLog += "Relay 2 turned OFF<br>";
  server.send(200, "text/plain", "Relay 2 is OFF");
}

// Handle status request
void handleStatus() {
  String status = "Relay 1 is " + String(relay1State ? "ON" : "OFF") + "<br>";
  status += "Relay 2 is " + String(relay2State ? "ON" : "OFF");
  server.send(200, "text/html", status);
}

// Handle console log request
void handleConsole() {
  server.send(200, "text/plain", consoleLog);
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize the relay pins
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH); // Initialize relay 1 as off
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, HIGH); // Initialize relay 2 as off

  // Initialize WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize web server
  server.on("/", handleRoot);
  server.on("/relay1/on", handleRelay1On);
  server.on("/relay1/off", handleRelay1Off);
  server.on("/relay2/on", handleRelay2On);
  server.on("/relay2/off", handleRelay2Off);
  server.on("/status", handleStatus);
  server.on("/console", handleConsole);
  server.begin();
}

void loop() {
  server.handleClient(); // Handle incoming client requests
}
