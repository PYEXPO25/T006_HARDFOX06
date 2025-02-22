#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// WiFi Credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// Server API URL (Change to your Flask API IP)
#define SERVER_URL "http://your_server_ip:5000/sensor"

// Sensor Pins
#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_MOISTURE_AO 32
#define WATER_LEVEL_AO 34
#define RELAY_PUMP 26
#define RELAY_COOLER 27

DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");

    dht.begin();
    pinMode(SOIL_MOISTURE_AO, INPUT);
    pinMode(WATER_LEVEL_AO, INPUT);
    pinMode(RELAY_PUMP, OUTPUT);
    pinMode(RELAY_COOLER, OUTPUT);

    digitalWrite(RELAY_PUMP, HIGH);
    digitalWrite(RELAY_COOLER, HIGH);
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();
        int soilMoistureValue = analogRead(SOIL_MOISTURE_AO);
        int soilMoisturePercent = map(soilMoistureValue, 4095, 0, 0, 100);
        int waterLevel = analogRead(WATER_LEVEL_AO);

        // Control Pump & Cooler
        if (soilMoisturePercent < 30) {
            digitalWrite(RELAY_PUMP, LOW); // Turn ON Pump
        } else {
            digitalWrite(RELAY_PUMP, HIGH); // Turn OFF Pump
        }

        if (temperature > 25.0) {
            digitalWrite(RELAY_COOLER, LOW); // Turn ON Cooler
        } else {
            digitalWrite(RELAY_COOLER, HIGH); // Turn OFF Cooler
        }

        // Create JSON Data
        StaticJsonDocument<200> jsonDoc;
        jsonDoc["temperature"] = temperature;
        jsonDoc["humidity"] = humidity;
        jsonDoc["soil_moisture"] = soilMoisturePercent;
        jsonDoc["water_level"] = waterLevel;

        String jsonString;
        serializeJson(jsonDoc, jsonString);

        // Send Data to Flask API
        HTTPClient http;
        http.begin(SERVER_URL);
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(jsonString);
        Serial.println("HTTP Response: " + String(httpResponseCode));

        http.end();
    }

    delay(5000); // Send data every 5 seconds
}

