#include <ESP8266WiFi.h> // Include the WiFi library for ESP8266
#include <PubSubClient.h> // Include the MQTT library for ESP8266
#include <WiFiClientSecureBearSSL.h> // Include the BearSSL library for secure connections

// WiFi parameters
const char* ssid = "your_SSID_here"; // Enter your WiFi SSID (network name)
const char* password = "your_password_here"; // Enter your WiFi password

// AWS IoT Core parameters
const char* awsEndpoint = "your_aws_endpoint_here"; // Enter your AWS IoT endpoint
const int awsPort = 8883; // AWS IoT port for secure MQTT communication

// Certificates and keys (enter your own AWS IoT certificates and private key)
const char* awsCert = R"EOF(
-----BEGIN CERTIFICATE-----
...Your AWS Device Certificate Here...
-----END CERTIFICATE-----
)EOF";

const char* awsPrivateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
...Your AWS Private Key Here...
-----END RSA PRIVATE KEY-----
)EOF";

const char* awsRootCA = R"EOF(
-----BEGIN CERTIFICATE-----
...Your AWS Root CA Here...
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

void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 baud
  setupWiFi(); // Setup WiFi connection
  connectAWS(); // Connect to AWS IoT Core
}

void loop() {
  if (!client.connected()) {
    connectAWS(); // Reconnect if the client is disconnected
  }
  client.loop(); // Maintain the MQTT connection
}
