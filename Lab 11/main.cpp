#include <ESP8266WiFi.h> // Include library for ESP8266 WiFi functionality
#include <WiFiClientSecure.h> // Include library for secure WiFi connections
#include <PubSubClient.h> // Include library for MQTT communication
#include <ArduinoJson.h> // Include library for JSON handling
#include <DHT.h> // Include library for DHT sensor

// Define the DHT sensor pin and type
#define DHTPIN 14 // GPIO14 (D5 on ESP8266)
#define DHTTYPE DHT11 // DHT 11 sensor

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

WiFiClientSecure net; // Secure WiFi client
PubSubClient client(net); // MQTT client using the secure WiFi client

// Function to generate dummy temperature data
float generateDummyTemperature() {
  return random(1500, 3500) / 100.0; // Generates temperature between 15.00 to 35.00
}

// Function to generate dummy humidity data
float generateDummyHumidity() {
  return random(3000, 7000) / 100.0; // Generates humidity between 30.00 to 70.00
}

// Function to generate dummy pressure data
float generateDummyPressure() {
  return random(90000, 110000) / 100.0; // Generates pressure between 900.00 to 1100.00 hPa
}

// Function to generate dummy air quality data
float generateDummyAirQuality() {
  return random(50, 300); // Generates air quality index between 50 to 300
}

// Function to generate dummy CO2 levels
float generateDummyCO2() {
  return random(400, 2000); // Generates CO2 levels between 400 to 2000 ppm
}

// Function to generate dummy VOC levels
float generateDummyVOC() {
  return random(0, 500); // Generates VOC levels between 0 to 500 ppb
}

// Function to generate dummy light levels
float generateDummyLight() {
  return random(0, 10000); // Generates light level between 0 to 10000 lux
}

// Function to generate dummy noise levels
float generateDummyNoise() {
  return random(30, 100); // Generates noise level between 30 to 100 dB
}

// Function to synchronize time using NTP
void NTPConnect() {
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "my.pool.ntp.org", "time.nist.gov"); // UTC+8 timezone
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

// Function to connect to WiFi
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Replace with your WiFi credentials
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect to AWS IoT
void connectToAWS() {
  // Set up certificates and keys
  BearSSL::X509List cert(awsCert); // Replace with your AWS certificate
  BearSSL::PrivateKey key(awsPrivateKey); // Replace with your AWS private key
  BearSSL::X509List ca(awsRootCA); // Replace with your AWS root CA

  net.setClientRSACert(&cert, &key); // Set RSA certificate and private key
  net.setTrustAnchors(&ca); // Set trust anchors

  client.setServer(AWS_ENDPOINT, 8883); // Replace with your AWS endpoint

  Serial.println("Connecting to AWS IoT...");
  while (!client.connect("ESP8266Client")) {
    Serial.print("MQTT state: ");
    Serial.println(client.state());
    delay(1000);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  Serial.println("AWS IoT Connected!");
}

// Function to publish message to AWS IoT
void publishMessage() {
  StaticJsonDocument<512> doc;
  doc["device_id"] = "ESP8266-01";
  doc["Temperature"] = dht.readTemperature();
  doc["Humidity"] = dht.readHumidity();
  doc["Pressure"] = generateDummyPressure();
  doc["AirQuality"] = generateDummyAirQuality();
  doc["CO2"] = generateDummyCO2();
  doc["VOC"] = generateDummyVOC();
  doc["Light"] = generateDummyLight();
  doc["Noise"] = generateDummyNoise();
  doc["timestamp"] = time(nullptr); // Get the current time in seconds since the Epoch

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer)) {
    Serial.print("Message published: ");
    Serial.println(jsonBuffer);
  } else {
    Serial.println("Message publish failed.");
  }
}

// Setup function to initialize the program
void setup() {
  Serial.begin(115200); // Start serial communication at 115200 baud
  dht.begin(); // Initialize DHT sensor
  connectToWiFi(); // Connect to WiFi
  NTPConnect(); // Synchronize time using NTP
  connectToAWS(); // Connect to AWS IoT
}

// Main loop function
void loop() {
  if (!client.connected()) {
    Serial.println("Reconnecting to AWS IoT...");
    connectToAWS(); // Reconnect to AWS IoT if disconnected
  }
  client.loop(); // Maintain MQTT connection

  static unsigned long lastPublishTime = 0;
  unsigned long now = millis();
  if (now - lastPublishTime > 60000) { // Publish every 1 minute
    publishMessage(); // Publish sensor data to AWS IoT
    lastPublishTime = now; // Update the last publish time
  }
}
