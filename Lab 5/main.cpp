#include <ESP8266WiFi.h> // Library for managing WiFi connections on the ESP8266
#include <PubSubClient.h> // Library for MQTT communication, allowing the ESP8266 to publish and subscribe to MQTT topics
#include <WiFiClientSecureBearSSL.h> // Library for establishing secure WiFi connections using BearSSL, necessary for secure communication with AWS IoT Core
#include <ESP8266WebServer.h> // Library for creating a web server on the ESP8266
#include <ArduinoJson.h> // Library for parsing JSON data
#include <time.h> // Library for time functions, used for NTP (Network Time Protocol) to synchronize the device's clock

// WiFi parameters
const char* ssid = "your-ssid"; // Replace with your WiFi SSID
const char* password = "your-password"; // Replace with your WiFi password

// AWS IoT Core parameters
const char* awsEndpoint = "your-aws-endpoint"; // Replace with your AWS IoT endpoint
const int awsPort = 8883; // AWS IoT port for secure MQTT communication
const char* controlTopic = "ESP8266/control/led"; // MQTT topic to control the LED
const char* statusTopic = "ESP8266/status/led"; // MQTT topic to publish LED status

// Certificates and keys
const char* awsCert = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_CERTIFICATE_CONTENT_HERE
-----END CERTIFICATE-----
)EOF";

const char* awsPrivateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
YOUR_PRIVATE_KEY_CONTENT_HERE
-----END RSA PRIVATE KEY-----
)EOF";

const char* awsRootCA = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_ROOT_CA_CONTENT_HERE
-----END CERTIFICATE-----
)EOF";

// Global variables for secure WiFi and MQTT clients
BearSSL::WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);
ESP8266WebServer server(80);
const int ledPin = LED_BUILTIN; // GPIO pin to control the built-in LED
bool ledState = false; // Track the LED state
String consoleLog = ""; // Track the console log for webpage

// Callback function to handle incoming MQTT messages
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

  const char* msg = doc["message"];

  // Handle control message
  if (String(topic) == controlTopic) {
    if (String(msg) == "ON") {
      digitalWrite(ledPin, LOW); // LOW turns the LED on for built-in LED
      ledState = true;
      Serial.println("LED turned ON via MQTT");
      consoleLog += "LED turned ON via MQTT<br>";
      client.publish(statusTopic, "{\"message\": \"ON\"}");
    } else if (String(msg) == "OFF") {
      digitalWrite(ledPin, HIGH); // HIGH turns the LED off for built-in LED
      ledState = false;
      Serial.println("LED turned OFF via MQTT");
      consoleLog += "LED turned OFF via MQTT<br>";
      client.publish(statusTopic, "{\"message\": \"OFF\"}");
    }
  }
}

// Function to setup WiFi connection
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Start the WiFi connection

  // Wait for the WiFi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // Wait 500 milliseconds
    Serial.print("."); // Print a dot for each attempt to connect
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

// Time synchronization function using NTP
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

// Function to connect to AWS IoT Core
void connectAWS() {
  // Convert the certificate strings to BearSSL types
  BearSSL::X509List cert(awsCert);
  BearSSL::PrivateKey key(awsPrivateKey);
  BearSSL::X509List ca(awsRootCA);

  // Set the certificates and key
  wifiClient.setClientRSACert(&cert, &key);
  wifiClient.setTrustAnchors(&ca);
  
  client.setServer(awsEndpoint, awsPort); // Set the AWS IoT endpoint and port
  client.setCallback(messageReceived); // Set the callback function for incoming messages

  // Attempt to connect to AWS IoT Core
  while (!client.connected()) {
    Serial.print("Connecting to AWS IoT...");
    consoleLog += "Connecting to AWS IoT...<br>";

    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      consoleLog += "Connected to AWS IoT<br>";
      // Subscribe to the control topic
      client.subscribe(controlTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state()); // Print the error code
      Serial.println(" try again in 5 seconds");
      consoleLog += "failed, rc=" + String(client.state()) + " try again in 5 seconds<br>";
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

// Function to handle the root web page
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

// Function to handle turning the LED on
void handleOn() {
  digitalWrite(ledPin, LOW); // LOW turns the LED on for built-in LED
  ledState = true;
  Serial.println("LED turned ON");
  consoleLog += "LED turned ON<br>";
  client.publish(statusTopic, "{\"message\": \"ON\"}");
  server.send(200, "text/plain", "LED is ON");
}

// Function to handle turning the LED off
void handleOff() {
  digitalWrite(ledPin, HIGH); // HIGH turns the LED off for built-in LED
  ledState = false;
  Serial.println("LED turned OFF");
  consoleLog += "LED turned OFF<br>";
  client.publish(statusTopic, "{\"message\": \"OFF\"}");
  server.send(200, "text/plain", "LED is OFF");
}

// Function to handle status requests
void handleStatus() {
  server.send(200, "text/plain", "LED is " + String(ledState ? "ON" : "OFF"));
}

// Function to handle console log requests
void handleConsole() {
  server.send(200, "text/plain", consoleLog);
}

// Setup function
void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 baud
  pinMode(ledPin, OUTPUT); // Set LED pin as output
  digitalWrite(ledPin, HIGH); // Initialize LED as off
  setupWiFi(); // Setup WiFi connection
  NTPConnect(); // Synchronize time using NTP
  connectAWS(); // Connect to AWS IoT Core

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/status", handleStatus);
  server.on("/console", handleConsole);
  server.begin(); // Start the web server
}

// Main loop function
void loop() {
  if (!client.connected()) {
    connectAWS(); // Reconnect if the client is disconnected
  }

  client.loop(); // Maintain the MQTT connection
  server.handleClient(); // Handle incoming web server requests
}
