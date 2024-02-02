#include <Arduino.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include "ThingSpeak.h"
#include <WiFi.h>
#include "HX711.h"
#include "soc/rtc.h"

const char *ssid = "YOUR_WIFI_NAME";
const char *password = "YOUR_WIFI_PASSWORD";
unsigned long myChannelNumber = THINGSPEAK_CHANNEL_NUMBER;
const char *myWriteAPIKey = "THINGSPEAK_WRITE_API_KEY";

WiFiClient client;

unsigned long lastTime = 0;
unsigned long timerDelay = 60000;

#define DHT_SENSOR_PIN 17
#define DHT_SENSOR_TYPE DHT11
#define LOADCELL_DOUT_PIN 16
#define LOADCELL_SCK_PIN 4

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
HX711 scale;

void setup() {
  Serial.begin(115200);
  dht_sensor.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);

  // Connect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  // HX711 setup
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(218.181818); // Set your scale calibration value
  scale.tare();
}

void loop() {
  // Connect to WiFi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  // Read humidity
  float humi = dht_sensor.readHumidity();
  // Read temperature in Celsius
  float tempC = dht_sensor.readTemperature();
  // Read temperature in Fahrenheit
  float tempF = dht_sensor.readTemperature(true);

  // Check DHT sensor readings
  if (isnan(tempC) || isnan(tempF) || isnan(humi)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // Display DHT sensor readings on LCD
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(tempC);
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Humi: ");
    lcd.print(humi);
    lcd.print("%");

    // Display weight from HX711 on LCD
    lcd.setCursor(0, 2);
    lcd.print("Weight: ");
    lcd.print(scale.get_units(), 1);
    lcd.print(" g");

    // Print DHT sensor readings and weight to serial monitor
    Serial.print("Humidity: ");
    Serial.print(humi);
    Serial.print("% | Temperature: ");
    Serial.print(tempC);
    Serial.print("°C ~ ");
    Serial.print(tempF);
    Serial.print("°F | Weight: ");
    Serial.print(scale.get_units(), 1);
    Serial.println(" g");

    // Update ThingSpeak fields
    ThingSpeak.setField(1, tempC);
    ThingSpeak.setField(2, humi);
    ThingSpeak.setField(3, scale.get_units());

    // Send data to ThingSpeak
    if ((millis() - lastTime) > timerDelay) {
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

      if (x == 200) {
        Serial.println("Channel update successful.");
      } else {
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }

      lastTime = millis();
    }
  }
  // Power down the HX711 for a short duration
  //scale.power_down();
  //delay(5000);
  //scale.power_up();
}
