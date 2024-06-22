#include <ESP8266WiFi.h> // Include the WiFi library for ESP8266
#include <ESP8266Ping.h> // Include the Ping library for ESP8266

// WiFi parameters to be configured by the user
const char* ssid = "your_SSID_here"; // Enter your WiFi SSID (network name)
const char* password = "your_password_here"; // Enter your WiFi password
const char* pingHost = "google.com"; // Host to ping for connectivity check

const int ledPin = LED_BUILTIN; // Onboard LED pin for status indication

// Function prototypes
void displayWiFiDetails();
void pingTest();

void setup(void) {
  // Set the LED pin as an OUTPUT
  pinMode(ledPin, OUTPUT);

  // Initialize serial communication at 115200 baud
  Serial.begin(115200);

  // Start the WiFi connection
  WiFi.begin(ssid, password);

  // Wait for the WiFi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // Wait 500 milliseconds
    Serial.print("."); // Print a dot for each attempt to connect
    digitalWrite(ledPin, !digitalRead(ledPin)); // Blink the LED to indicate connection attempt
  }

  // Turn off the LED when connected
  digitalWrite(ledPin, LOW);

  // Print a new line, then print WiFi connected
  Serial.println("");
  Serial.println("WiFi connected");

  // Display WiFi details
  displayWiFiDetails();
}

void loop() {
  // Continuously display WiFi details and perform ping test every 10 seconds
  displayWiFiDetails();
  pingTest();
  delay(10000); // Wait for 10 seconds before repeating
}

void displayWiFiDetails() {
  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Print the Gateway address
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

  // Print the Subnet Mask
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());

  // Print the DNS servers
  Serial.print("DNS Server 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("DNS Server 2: ");
  Serial.println(WiFi.dnsIP(1));

  // Print the RSSI (Received Signal Strength Indicator)
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  Serial.println("---------");

  // Blink LED to indicate status
  for (int i = 0; i < 2; i++) {
    digitalWrite(ledPin, HIGH);
    delay(250); // Wait 250 milliseconds
    digitalWrite(ledPin, LOW);
    delay(250); // Wait another 250 milliseconds
  }
}

void pingTest() {
  // Print message indicating start of ping test
  Serial.print("Pinging ");
  Serial.print(pingHost);
  Serial.println("...");

  // Perform the ping test
  if (Ping.ping(pingHost)) {
    Serial.println("Ping successful"); // Print message if ping is successful
  } else {
    Serial.println("Ping failed"); // Print message if ping fails
  }
  Serial.println("---------");
}
