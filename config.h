// Physical device information for board and sensor
#define DEVICE_ID "Feather HUZZAH ESP8266 WiFi"
#define DHT_TYPE DHT22

// Pin layout configuration
#define LED_PIN 0
#define DHT_PIN 2
#define NEO_PIN 15
#define NEO_COUNT 24
#define FADE_SPEED 10
#define FADE_PAUSE 10
#define RED 255
#define GREEN 255
#define BLUE 255

#define TEMPERATURE_ALERT 30

// Interval time(ms) for sending message to IoT Hub
#define INTERVAL 2000
#define LIGHTS false 
#define FLASH false 

// If don't have a physical DHT sensor, can send simulated data to IoT hub
#define SIMULATED_DATA true 

// EEPROM address configuration
#define EEPROM_SIZE 512

// SSID and SSID password's length should < 32 bytes
// http://serverfault.com/a/45509
#define SSID_LEN 32
#define PASS_LEN 32
#define CONNECTION_STRING_LEN 256

#define MESSAGE_MAX_LEN 256
