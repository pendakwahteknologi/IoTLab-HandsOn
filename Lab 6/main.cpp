#define BLYNK_TEMPLATE_ID "Your_Template_ID" // replace with your Blynk template ID
#define BLYNK_TEMPLATE_NAME "Your_Template_NAME" // replace with your Blynk template Name

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Blynk authentication token
char auth[] = "Your_Auth_Token"; // replace with your Blynk authentication token

// WiFi credentials
char ssid[] = "Your_SSID"; // replace with your WiFi SSID
char pass[] = "Your_WiFi_Password"; // replace with your WiFi password

const int ledPin = LED_BUILTIN; // GPIO pin to control the built-in LED

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize the LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // Initialize LED as off (HIGH means off for the built-in LED)

  // Connect to Blynk
  Blynk.begin(auth, ssid, pass);
  Serial.println("Connected to Blynk");

  // Print IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Run Blynk
  Blynk.run();
}

// Blynk function to control the LED
BLYNK_WRITE(V0) { // Updated virtual pin to V0
  int pinValue = param.asInt(); // Get the value from the Blynk app

  // Turn the LED on or off based on the value from the Blynk app
  if (pinValue == 1) {
    digitalWrite(ledPin, LOW); // Turn the LED on (LOW means on for the built-in LED)
    Serial.println("LED turned ON");
  } else {
    digitalWrite(ledPin, HIGH); // Turn the LED off (HIGH means off for the built-in LED)
    Serial.println("LED turned OFF");
  }
}
