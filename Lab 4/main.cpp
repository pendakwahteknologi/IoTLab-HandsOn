#include <ESP8266WiFi.h> // Library for connecting the ESP8266 to a WiFi network
#include <PubSubClient.h> // Library for MQTT communication, allowing the ESP8266 to publish and subscribe to topics
#include <WiFiClientSecureBearSSL.h> // Library for secure WiFi connections using BearSSL, enabling secure communication with AWS IoT Core
#include <time.h> // Library for time functions, used for NTP (Network Time Protocol) to synchronize the device's clock

// WiFi parameters
const char* ssid = "your-ssid"; // Replace with your WiFi SSID
const char* password = "your-password"; // Replace with your WiFi password

// AWS IoT Core parameters
const char* awsEndpoint = "your-aws-endpoint"; // Replace with your AWS IoT endpoint
const int awsPort = 8883; // AWS IoT port for secure MQTT
const char* publishTopic = "esp8266/messages"; // MQTT topic to publish messages
const char* subscribeTopic = "aws/messages"; // MQTT topic to subscribe to messages

// Device Certificate
const char* awsCert = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_DEVICE_CERTIFICATE_CONTENT_HERE
-----END CERTIFICATE-----
)EOF";

// Private Key
const char* awsPrivateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
YOUR_PRIVATE_KEY_CONTENT_HERE
-----END RSA PRIVATE KEY-----
)EOF";

// Amazon CA Certificate
const char* awsRootCA = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_ROOT_CA_CONTENT_HERE
-----END CERTIFICATE-----
)EOF";

// Global variables for secure WiFi and MQTT clients
BearSSL::WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

// Callback function to handle incoming messages
void messageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
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
  Serial.println("WiFi connected"); // Print message when connected
}

// Time synchronization function using NTP
void NTPConnect() {
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "my.pool.ntp.org", "time.nist.gov"); // Adjust timezone as necessary
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

    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe to the topic
      client.subscribe(subscribeTopic);
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
  NTPConnect(); // Synchronize time using NTP
  connectAWS(); // Connect to AWS IoT Core
}

void loop() {
  if (!client.connected()) {
    connectAWS(); // Reconnect if the client is disconnected
  }

  // Publish a message to the MQTT topic
  const char* message = "{\"message\": \"hi from esp8266\"}";
  client.publish(publishTopic, message);

  // Wait for 10 seconds before publishing again
  delay(10000);

  client.loop(); // Maintain the MQTT connection
}
