#define BLYNK_TEMPLATE_ID "YourTemplateID" // Replace with your Blynk template ID
#define BLYNK_TEMPLATE_NAME "YourTemplateName" // Replace with your Blynk template name

#include <ESP8266WiFi.h> // Library for managing WiFi connections on the ESP8266
#include <BlynkSimpleEsp8266.h> // Library for Blynk functionality
#include <PubSubClient.h> // Library for MQTT communication
#include <WiFiClientSecureBearSSL.h> // Library for secure WiFi connections using BearSSL
#include <ESP8266WebServer.h> // Library for creating a web server on the ESP8266
#include <ArduinoJson.h> // Library for parsing JSON data
#include <time.h> // Library for time functions, used for NTP

// Blynk authentication token
char auth[] = "YourBlynkAuthToken"; // Replace with your Blynk authentication token

// WiFi credentials
char ssid[] = "YourSSID"; // Replace with your WiFi SSID
char pass[] = "YourPassword"; // Replace with your WiFi password

// AWS IoT Core parameters
const char* awsEndpoint = "YourAWSEndpoint"; // Replace with your AWS IoT endpoint
const int awsPort = 8883; // Port for AWS IoT
const char* controlTopic = "ESP8266/control/relay"; // Topic for controlling relays
const char* statusTopic = "ESP8266/status/relay"; // Topic for relay status

// AWS IoT certificates and keys
const char* awsCert = R"EOF(
-----BEGIN CERTIFICATE-----
<Your AWS Device Certificate Here>
-----END CERTIFICATE-----
)EOF";

const char* awsPrivateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
<Your AWS Private Key Here>
-----END RSA PRIVATE KEY-----
)EOF";

const char* awsRootCA = R"EOF(
-----BEGIN CERTIFICATE-----
<Your AWS Root CA Certificate Here>
-----END CERTIFICATE-----
)EOF";

// Relay pins
const int relay1Pin = D6; // GPIO pin for relay 1
const int relay2Pin = D7; // GPIO pin for relay 2

// Global variables
BearSSL::WiFiClientSecure wifiClient; // Secure WiFi client
PubSubClient mqttClient(wifiClient); // MQTT client
ESP8266WebServer server(80); // Web server
bool relay1State = false; // Track the state of relay 1
bool relay2State = false; // Track the state of relay 2
String consoleLog = ""; // Log for console output on the webpage

// Function to publish relay status to AWS IoT
void publishRelayStatus(const char* relay, const char* state) {
  StaticJsonDocument<200> doc;
  doc["device_id"] = "ESP8266-01"; // Replace with your device ID
  doc["relay"] = relay;
  doc["status"] = state;
  doc["timestamp"] = time(nullptr); // Current time in seconds since the Epoch

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish(statusTopic, buffer, n);
}

// Callback function to handle messages received from AWS IoT
void messageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  String message;
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();

  // Add to console log
  consoleLog += "Message arrived [" + String(topic) + "]: " + message + "<br>";

  // Parse the JSON payload
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    consoleLog += "deserializeJson() failed: " + String(error.f_str()) + "<br>";
    return;
  }

  const char* relay = doc["relay"];
  const char* state = doc["status"];

  // Handle control message for relay 1
  if (String(relay) == "relay1") {
    if (String(state) == "ON") {
      digitalWrite(relay1Pin, LOW); // Turn relay 1 on
      relay1State = true;
      Serial.println("Relay 1 turned ON via MQTT");
      consoleLog += "Relay 1 turned ON via MQTT<br>";
      publishRelayStatus("relay1", "ON");
      Blynk.virtualWrite(V1, 1); // Sync with Blynk
    } else if (String(state) == "OFF") {
      digitalWrite(relay1Pin, HIGH); // Turn relay 1 off
      relay1State = false;
      Serial.println("Relay 1 turned OFF via MQTT");
      consoleLog += "Relay 1 turned OFF via MQTT<br>";
      publishRelayStatus("relay1", "OFF");
      Blynk.virtualWrite(V1, 0); // Sync with Blynk
    }
  // Handle control message for relay 2
  } else if (String(relay) == "relay2") {
    if (String(state) == "ON") {
      digitalWrite(relay2Pin, LOW); // Turn relay 2 on
      relay2State = true;
      Serial.println("Relay 2 turned ON via MQTT");
      consoleLog += "Relay 2 turned ON via MQTT<br>";
      publishRelayStatus("relay2", "ON");
      Blynk.virtualWrite(V2, 1); // Sync with Blynk
    } else if (String(state) == "OFF") {
      digitalWrite(relay2Pin, HIGH); // Turn relay 2 off
      relay2State = false;
      Serial.println("Relay 2 turned OFF via MQTT");
      consoleLog += "Relay 2 turned OFF via MQTT<br>";
      publishRelayStatus("relay2", "OFF");
      Blynk.virtualWrite(V2, 0); // Sync with Blynk
    }
  }
}

// Time synchronization using NTP
void NTPConnect() {
  Serial.print("Setting time using SNTP");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // Configure time using NTP servers
  time_t now = time(nullptr);
  while (now < 8 * 3600) { // Wait for time to be set
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" done!");
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

// Function to setup WiFi connection
void setupWiFi() {
  delay(10);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Access web interface via http://");
  Serial.println(WiFi.localIP());
}

// Function to connect to AWS IoT Core
void connectAWS() {
  NTPConnect();

  // Convert the certificate strings to BearSSL types
  BearSSL::X509List cert(awsCert);
  BearSSL::PrivateKey key(awsPrivateKey);
  BearSSL::X509List ca(awsRootCA);

  // Set the certificates and key
  wifiClient.setClientRSACert(&cert, &key);
  wifiClient.setTrustAnchors(&ca);
  
  mqttClient.setServer(awsEndpoint, awsPort);
  mqttClient.setCallback(messageReceived);

  while (!mqttClient.connected()) {
    Serial.print("Connecting to AWS IoT...");
    
    if (mqttClient.connect("ESP8266Client")) {
      Serial.println("connected");
      mqttClient.subscribe(controlTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Function to handle the root web page
void handleRoot() {
  String html = "<html><head><title>ESP8266 Control</title>";
  html += "<style>body { font-family: monospace; text-align: center; background-color: #282c34; color: white; }";
  html += ".container { display: flex; justify-content: center; flex-wrap: wrap; }";
  html += ".box { padding: 20px; margin: 20px; background-color: #444; border-radius: 10px; }";
  html += "button { padding: 10px 20px; margin: 10px; font-size: 16px; cursor: pointer; background-color: #888; color: white; border: none; border-radius: 5px; }";
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
  html += "<div class='container'>";
  html += "<div class='box'><h2>Relay 1</h2>";
  html += "<p id='status1'>Relay 1 is " + String(relay1State ? "ON" : "OFF") + "</p>";
  html += "<button id='onButton1' onclick=\"toggleRelay('relay1', 'on')\">Turn Relay 1 ON</button>";
  html += "<button id='offButton1' onclick=\"toggleRelay('relay1', 'off')\">Turn Relay 1 OFF</button></div>";
  html += "<div class='box'><h2>Relay 2</h2>";
  html += "<p id='status2'>Relay 2 is " + String(relay2State ? "ON" : "OFF") + "</p>";
  html += "<button id='onButton2' onclick=\"toggleRelay('relay2', 'on')\">Turn Relay 2 ON</button>";
  html += "<button id='offButton2' onclick=\"toggleRelay('relay2', 'off')\">Turn Relay 2 OFF</button></div>";
  html += "</div>";
  html += "<div class='console' id='consoleLog'>" + consoleLog + "</div>";
  html += "<script>";
  html += "function toggleRelay(relay, action) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/' + relay + '/' + action, true);";
  html += "  xhr.onreadystatechange = function () {";
  html += "    if (xhr.readyState == 4 && xhr.status == 200) {";
  html += "      updateStatus();";
  html += "      updateConsole();";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "function updateStatus() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/status', true);";
  html += "  xhr.onreadystatechange = function () {";
  html += "    if (xhr.readyState == 4 && xhr.status == 200) {";
  html += "      var status = JSON.parse(xhr.responseText);";
  html += "      document.getElementById('status1').innerHTML = 'Relay 1 is ' + (status.relay1 ? 'ON' : 'OFF');";
  html += "      document.getElementById('status2').innerHTML = 'Relay 2 is ' + (status.relay2 ? 'ON' : 'OFF');";
  html += "      document.getElementById('onButton1').classList.toggle('active', status.relay1);";
  html += "      document.getElementById('offButton1').classList.toggle('active', !status.relay1);";
  html += "      document.getElementById('onButton2').classList.toggle('active', status.relay2);";
  html += "      document.getElementById('offButton2').classList.toggle('active', !status.relay2);";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
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
  html += "setInterval(updateStatus, 1000);"; // Update status every second
  html += "setInterval(updateConsole, 1000);"; // Update console log every second
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Function to handle turning relay 1 on
void handleRelay1On() {
  digitalWrite(relay1Pin, LOW); // Turn relay 1 on
  relay1State = true;
  Serial.println("Relay 1 turned ON");
  consoleLog += "Relay 1 turned ON<br>";
  publishRelayStatus("relay1", "ON");
  Blynk.virtualWrite(V1, 1); // Sync with Blynk
  server.send(200, "text/plain", "Relay 1 is ON");
}

// Function to handle turning relay 1 off
void handleRelay1Off() {
  digitalWrite(relay1Pin, HIGH); // Turn relay 1 off
  relay1State = false;
  Serial.println("Relay 1 turned OFF");
  consoleLog += "Relay 1 turned OFF<br>";
  publishRelayStatus("relay1", "OFF");
  Blynk.virtualWrite(V1, 0); // Sync with Blynk
  server.send(200, "text/plain", "Relay 1 is OFF");
}

// Function to handle turning relay 2 on
void handleRelay2On() {
  digitalWrite(relay2Pin, LOW); // Turn relay 2 on
  relay2State = true;
  Serial.println("Relay 2 turned ON");
  consoleLog += "Relay 2 turned ON<br>";
  publishRelayStatus("relay2", "ON");
  Blynk.virtualWrite(V2, 1); // Sync with Blynk
  server.send(200, "text/plain", "Relay 2 is ON");
}

// Function to handle turning relay 2 off
void handleRelay2Off() {
  digitalWrite(relay2Pin, HIGH); // Turn relay 2 off
  relay2State = false;
  Serial.println("Relay 2 turned OFF");
  consoleLog += "Relay 2 turned OFF<br>";
  publishRelayStatus("relay2", "OFF");
  Blynk.virtualWrite(V2, 0); // Sync with Blynk
  server.send(200, "text/plain", "Relay 2 is OFF");
}

// Function to handle status requests
void handleStatus() {
  String json = "{\"relay1\": " + String(relay1State ? "true" : "false") + ", \"relay2\": " + String(relay2State ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

// Function to handle console log requests
void handleConsole() {
  server.send(200, "text/plain", consoleLog);
}

// Setup function
void setup() {
  Serial.begin(115200);
  
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  
  digitalWrite(relay1Pin, HIGH); // Initialize relay 1 as off
  digitalWrite(relay2Pin, HIGH); // Initialize relay 2 as off
  
  setupWiFi();
  Blynk.begin(auth, ssid, pass);
  Serial.println("Connected to Blynk");

  connectAWS();

  server.on("/", handleRoot);
  server.on("/relay1/on", handleRelay1On);
  server.on("/relay1/off", handleRelay1Off);
  server.on("/relay2/on", handleRelay2On);
  server.on("/relay2/off", handleRelay2Off);
  server.on("/status", handleStatus);
  server.on("/console", handleConsole);
  server.begin();
}

// Main loop function
void loop() {
  Blynk.run();
  mqttClient.loop();
  server.handleClient();
}

// Blynk function to control relay 1
BLYNK_WRITE(V1) {
  int pinValue = param.asInt(); // Get the value from the Blynk app

  // Turn relay 1 on or off based on the value from the Blynk app
  if (pinValue == 1) {
    digitalWrite(relay1Pin, LOW); // Turn relay 1 on
    relay1State = true;
    Serial.println("Relay 1 turned ON via Blynk");
    publishRelayStatus("relay1", "ON");
  } else {
    digitalWrite(relay1Pin, HIGH); // Turn relay 1 off
    relay1State = false;
    Serial.println("Relay 1 turned OFF via Blynk");
    publishRelayStatus("relay1", "OFF");
  }
}

// Blynk function to control relay 2
BLYNK_WRITE(V2) {
  int pinValue = param.asInt(); // Get the value from the Blynk app

  // Turn relay 2 on or off based on the value from the Blynk app
  if (pinValue == 1) {
    digitalWrite(relay2Pin, LOW); // Turn relay 2 on
    relay2State = true;
    Serial.println("Relay 2 turned ON via Blynk");
    publishRelayStatus("relay2", "ON");
  } else {
    digitalWrite(relay2Pin, HIGH); // Turn relay 2 off
    relay2State = false;
    Serial.println("Relay 2 turned OFF via Blynk");
    publishRelayStatus("relay2", "OFF");
  }
}
