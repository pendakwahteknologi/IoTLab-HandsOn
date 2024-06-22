#include <ESP8266WiFi.h> // Library for managing WiFi connections on the ESP8266
#include <PubSubClient.h> // Library for MQTT communication, allowing the ESP8266 to publish and subscribe to MQTT topics
#include <WiFiClientSecureBearSSL.h> // Library for establishing secure WiFi connections using BearSSL, necessary for secure communication with AWS IoT Core

// WiFi parameters
const char* ssid = "your-ssid";
const char* password = "your-password";

// AWS IoT Core parameters
const char* awsEndpoint = "your-aws-endpoint"; // replace with your AWS IoT endpoint
const int awsPort = 8883;

// Certificates and keys
const char* awsCert = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_DEVICE_CERTIFICATE_CONTENT_HERE
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
  Serial.println("WiFi connected"); // Print message when connected
}

// Function to connect to AWS IoT Core
void connectAWS() {
  // Convert the certificate strings to BearSSL types
  BearSSL::X509List cert(awsCert);
  BearSSL::PrivateKey key(awsPrivateKey);
  BearSSL::X509List ca(awsRootCA);

  // Set the certificates and key for secure connection
  wifiClient.setClientRSACert(&cert, &key);
  wifiClient.setTrustAnchors(&ca);
  
  client.setServer(awsEndpoint, awsPort); // Set the AWS IoT endpoint and port

  // Attempt to connect to AWS IoT Core
  while (!client.connected()) {
    Serial.print("Connecting to AWS IoT...");

    if (client.connect("ESP8266Client")) {
      Serial.println("connected"); // Print message when connected
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state()); // Print the error code
      Serial.println(" try again in 5 seconds");
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

// Time synchronization function
void NTPConnect() {
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "my.pool.ntp.org", "time.nist.gov"); // UTC+8
  time_t now = time(nullptr);
  while (now < 8 * 3600) {
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

void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 baud
  setupWiFi(); // Setup WiFi connection
  NTPConnect(); // Synchronize time using NTP
  connectAWS(); // Connect to AWS IoT Core
}

void loop() {
  if (!client.connected()) {
    connectAWS(); // Reconnect if the client is disconnected
  }
  client.loop(); // Maintain the MQTT connection
}
