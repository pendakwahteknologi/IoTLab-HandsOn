// credentials.h
#ifndef SECRETS_H
#define SECRETS_H

// WiFi credentials
#define WIFI_SSID "your_wifi_ssid" // Replace with your WiFi SSID
#define WIFI_PASSWORD "your_wifi_password" // Replace with your WiFi password

// AWS IoT Core parameters
#define AWS_IOT_PUBLISH_TOPIC "home/esp8266-01/sensor_data" // Topic to publish sensor data
#define AWS_ENDPOINT "your_aws_endpoint" // Replace with your AWS IoT endpoint

// Device Certificate
const char* awsCert = R"EOF(
-----BEGIN CERTIFICATE-----
<Your Device Certificate Here>
-----END CERTIFICATE-----
)EOF";

// Private Key
const char* awsPrivateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
<Your Private Key Here>
-----END RSA PRIVATE KEY-----
)EOF";

// Amazon CA Certificate
const char* awsRootCA = R"EOF(
-----BEGIN CERTIFICATE-----
<Your Root CA Certificate Here>
-----END CERTIFICATE-----
)EOF";

#endif // SECRETS_H
