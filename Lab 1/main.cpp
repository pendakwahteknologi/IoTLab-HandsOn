#include <Arduino.h> // Include the Arduino library

// Define the pin number for the LED
#define LED 2

void setup() {
  // Initialize serial communication at a baud rate of 115200
  Serial.begin(115200);
  
  // Set the LED pin as an OUTPUT
  pinMode(LED, OUTPUT);
}

void loop() {
  // Turn the LED on (HIGH voltage level)
  digitalWrite(LED, HIGH);
  // Print a message to the Serial Monitor
  Serial.println("LED is on");
  // Wait for 1 second (1000 milliseconds)
  delay(1000);
  
  // Turn the LED off (LOW voltage level)
  digitalWrite(LED, LOW);
  // Print a message to the Serial Monitor
  Serial.println("LED is off");
  // Wait for 1 second (1000 milliseconds)
  delay(1000);
}
