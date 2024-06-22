#define BLYNK_TEMPLATE_ID "TMPL6cqfD0QRW"
#define BLYNK_TEMPLATE_NAME "ESP8266"

#include <ESP8266WiFi.h> // Library for managing WiFi connections on the ESP8266
#include <BlynkSimpleEsp8266.h> // Library for Blynk functionality
#include <PubSubClient.h> // Library for MQTT communication
#include <WiFiClientSecureBearSSL.h> // Library for secure WiFi connections using BearSSL
#include <ESP8266WebServer.h> // Library for creating a web server on the ESP8266
#include <ArduinoJson.h> // Library for parsing JSON data
#include <time.h> // Library for time functions, used for NTP (Network Time Protocol)

// Blynk authentication token
char auth[] = "your_blynk_auth_token"; // replace with your Blynk authentication token

// WiFi credentials
char ssid[] = "your_wifi_ssid"; // replace with your WiFi SSID
char pass[] = "your_wifi_password"; // replace with your WiFi password

// AWS IoT Core parameters
const char* awsEndpoint = "your_aws_endpoint"; // replace with your AWS IoT endpoint
const int awsPort = 8883;
const char* controlTopic = "ESP8266/control/led";
const char* statusTopic = "ESP8266/status/led";

// Certificates and keys
const char* awsCert = R"EOF(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)EOF";

const char* awsPrivateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
...
-----END RSA PRIVATE KEY-----
)EOF";

const char* awsRootCA = R"EOF(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)EOF";

// Global variables
BearSSL::WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
ESP8266WebServer server(80);
const int ledPin = LED_BUILTIN; // GPIO pin to control the built-in LED
bool ledState = false; // Track the LED state
String consoleLog = ""; // Track the console log for webpage

// Function to handle received MQTT messages
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

  const char* msg = doc["state"]["reported"]["message"];

  // Handle control message
  if (String(topic) == controlTopic) {
    if (String(msg) == "ON") {
      digitalWrite(ledPin, LOW); // LOW turns the LED on for built-in LED
      ledState = true;
      Serial.println("LED turned ON via MQTT");
      consoleLog += "LED turned ON via MQTT<br>";
      StaticJsonDocument<200> doc;
      doc["state"]["reported"]["message"] = "ON";
      char buffer[256];
      size_t n = serializeJson(doc, buffer);
      mqttClient.publish(statusTopic, buffer, n);
      Blynk.virtualWrite(V1, 1); // Sync with Blynk
    } else if (String(msg) == "OFF") {
      digitalWrite(ledPin, HIGH); // HIGH turns the LED off for built-in LED
      ledState = false;
      Serial.println("LED turned OFF via MQTT");
      consoleLog += "LED turned OFF via MQTT<br>";
      StaticJsonDocument<200> doc;
      doc["state"]["reported"]["message"] = "OFF";
      char buffer[256];
      size_t n = serializeJson(doc, buffer);
      mqttClient.publish(statusTopic, buffer, n);
      Blynk.virtualWrite(V1, 0); // Sync with Blynk
    }
  }
}

// Function to set up WiFi connection
void setupWiFi() {
  delay(10);
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

  // Add to console log
  consoleLog += "WiFi connected<br>";
  consoleLog += "IP address: " + WiFi.localIP().toString() + "<br>";
  consoleLog += "Access web interface via http://" + WiFi.localIP().toString() + "<br>";
}

// Function to connect to AWS IoT Core
void connectAWS() {
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
    consoleLog += "Connecting to AWS IoT...<br>";

    if (mqttClient.connect("ESP8266Client")) {
      Serial.println("connected");
      consoleLog += "Connected to AWS IoT<br>";
      // Subscribe to the control topic
      mqttClient.subscribe(controlTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      consoleLog += "failed, rc=" + String(mqttClient.state()) + " try again in 5 seconds<br>";
      delay(5000);
    }
  }
}

// Function to handle the root webpage
void handleRoot() {
  String html = "<html><head><title>ESP8266 Control</title>";
  html += "<style>body { font-family: monospace; text-align: center; background-color: #282c34; color: white; }";
  html += "button { padding: 10px 20px; margin: 10px; font-size: 16px; cursor: pointer; background-color: #444; color: white; border: none; }";
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
  html += "<p id='status'>LED is " + String(ledState ? "ON" : "OFF") + "</p>";
  html += "<button id='onButton' onclick=\"toggleLED('on')\">Turn ON</button>";
  html += "<button id='offButton' onclick=\"toggleLED('off')\">Turn OFF</button>";
  html += "<div class='console' id='consoleLog'>" + consoleLog + "</div>";
  html += "<script>";
  html += "function toggleLED(action) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/' + action, true);";
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
  html += "      document.getElementById('status').innerHTML = xhr.responseText;";
  html += "      var status = xhr.responseText.includes('ON');";
  html += "      document.getElementById('onButton').classList.toggle('active', status);";
  html += "      document.getElementById('offButton').classList.toggle('active', !status);";
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

// Function to handle the ON command from the web interface
void handleOn() {
  digitalWrite(ledPin, LOW); // LOW turns the LED on for built-in LED
  ledState = true;
  Serial.println("LED turned ON");
  consoleLog += "LED turned ON<br>";
  StaticJsonDocument<200> doc;
  doc["state"]["reported"]["message"] = "ON";
  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish(statusTopic, buffer, n);
  Blynk.virtualWrite(V1, 1); // Sync with Blynk
  server.send(200, "text/plain", "LED is ON");
}

// Function to handle the OFF command from the web interface
void handleOff() {
  digitalWrite(ledPin, HIGH); // HIGH turns the LED off for built-in LED
  ledState = false;
  Serial.println("LED turned OFF");
  consoleLog += "LED turned OFF<br>";
  StaticJsonDocument<200> doc;
  doc["state"]["reported"]["message"] = "OFF";
  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish(statusTopic, buffer, n);
  Blynk.virtualWrite(V1, 0); // Sync with Blynk
  server.send(200, "text/plain", "LED is OFF");
}

// Function to handle the status request from the web interface
void handleStatus() {
  server.send(200, "text/plain", "LED is " + String(ledState ? "ON" : "OFF"));
}

// Function to handle the console log request from the web interface
void handleConsole() {
  server.send(200, "text/plain", consoleLog);
}

// Setup function to initialize the ESP8266
void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize the LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // Initialize LED as off

  // Initialize WiFi and connect to Blynk
  setupWiFi();
  Blynk.begin(auth, ssid, pass);
  Serial.println("Connected to Blynk");

  // Connect to AWS IoT
  connectAWS();

  // Initialize web server
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/status", handleStatus);
  server.on("/console", handleConsole);
  server.begin();
}

// Main loop function to run Blynk and MQTT client
void loop() {
  // Run Blynk and MQTT client
  Blynk.run();
  mqttClient.loop();
  server.handleClient();
}

// Blynk function to control the LED
BLYNK_WRITE(V1) {
  int pinValue = param.asInt(); // Get the value from the Blynk app

  // Turn the LED on or off based on the value from the Blynk app
  if (pinValue == 1) {
    digitalWrite(ledPin, LOW); // Turn the LED on
    ledState = true;
    Serial.println("LED turned ON via Blynk");
    StaticJsonDocument<200> doc;
    doc["state"]["reported"]["message"] = "ON";
    char buffer[256];
    size_t n = serializeJson(doc, buffer);
    mqttClient.publish(statusTopic, buffer, n);
  } else {
    digitalWrite(ledPin, HIGH); // Turn the LED off
    ledState = false;
    Serial.println("LED turned OFF via Blynk");
    StaticJsonDocument<200> doc;
    doc["state"]["reported"]["message"] = "OFF";
    char buffer[256];
    size_t n = serializeJson(doc, buffer);
    mqttClient.publish(statusTopic, buffer, n);
  }
}
