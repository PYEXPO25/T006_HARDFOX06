#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <DHT.h>
#include <ArduinoJson.h>

// WiFi & Firebase Credentials
#define WIFI_SSID "Aadil"
#define WIFI_PASSWORD "Aadil12345*"
#define API_KEY "AIzaSyBCfakAt32puSbDh4owrDC2tKpXhoqraN0"
#define DATABASE_URL "https://automated-crop-management-default-rtdb.firebaseio.com/"
#define USER_EMAIL "aadilnajya@gmail.com"
#define USER_PASSWORD "Aadilbeen006"

// Sensor & Actuator Pins
#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_MOISTURE_AO 32    // Capacitive soil moisture sensor analog pin
#define WATER_LEVEL_AO 34      // Water level sensor analog pin
#define RELAY_PUMP 26          // Relay module controlling the water pump
#define RELAY_COOLER 27        // Relay module controlling the Peltier cooler
#define BUZZER_PIN 25          // Buzzer for alerts

DHT dht(DHTPIN, DHTTYPE);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;

// Global variables to store sensor data
float currentTemperature = 0.0;
float currentHumidity = 0.0;
int currentSoilMoisture = 0;
int currentWaterLevel = 0;

void setup() {
    Serial.begin(115200);
    
    // WiFi Connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(3000);
    }
    Serial.println("\nConnected with IP: " + WiFi.localIP().toString());

    // Firebase Configuration
    config.api_key = API_KEY;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    config.database_url = DATABASE_URL;
    config.token_status_callback = tokenStatusCallback;
    Firebase.reconnectNetwork(true);
    fbdo.setBSSLBufferSize(4096, 1024);
    fbdo.setResponseSize(2048);
    Firebase.begin(&config, &auth);
    Firebase.setDoubleDigits(5);
    config.timeout.serverResponse = 20 * 1000;

    // Initialize Sensors & Actuators
    dht.begin();
    pinMode(SOIL_MOISTURE_AO, INPUT);
    pinMode(WATER_LEVEL_AO, INPUT);
    pinMode(RELAY_PUMP, OUTPUT);
    pinMode(RELAY_COOLER, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Initial state: Relays OFF (assuming active-low relay module)
    digitalWrite(RELAY_PUMP, HIGH);
    digitalWrite(RELAY_COOLER, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
    if (Firebase.ready() && (millis() - sendDataPrevMillis > 10000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();
        
        // Read sensor data
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();
        int soilMoistureValue = analogRead(SOIL_MOISTURE_AO);
        int waterLevel = analogRead(WATER_LEVEL_AO);

        // Convert soil moisture analog value to percentage (adjust calibration values)
        int soilMoisturePercent = map(soilMoistureValue, 4095, 1000, 0, 100); // Calibrate for your sensor

        // Update global variables
        currentTemperature = temperature;
        currentHumidity = humidity;
        currentSoilMoisture = soilMoisturePercent;
        currentWaterLevel = waterLevel;

        // Create JSON object
        FirebaseJson jsonDoc;
        if (!isnan(temperature)) {
            jsonDoc.set("temperature", temperature);
            jsonDoc.set("humidity", humidity);
        }
        jsonDoc.set("soil_moisture", soilMoisturePercent);
        jsonDoc.set("water_level", waterLevel);
        
        // Print JSON to Serial Monitor
        String jsonStr;
        jsonDoc.toString(jsonStr, true);
        Serial.println(jsonStr);

        // Control Pump & Cooler based on thresholds
        digitalWrite(RELAY_PUMP, soilMoisturePercent < 3 ? LOW:HIGH);  // LOW activates relay
        digitalWrite(RELAY_COOLER, temperature > 28.0 ? LOW:HIGH);     // LOW activates relay
        
        // Buzzer Alert for Low Water Level (adjust threshold as needed)
        bool waterLow = waterLevel < 1000; // Calibrate this threshold
        digitalWrite(BUZZER_PIN, waterLow ? HIGH : LOW);
        if (waterLow) {
            Serial.println("{\"alert\":\"Water level low! Buzzer ON\"}");
        }

        // Upload to Firebase
        if (Firebase.RTDB.setJSON(&fbdo, "/sensorData", &jsonDoc)) {
            Serial.println("Data sent to Firebase!");
        } else {
            Serial.println("Firebase error: " + fbdo.errorReason());
        }
    }

    // Handle Serial Commands
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command == "PUMP_ON") {
            digitalWrite(RELAY_PUMP, LOW);
            Serial.println("{\"response\":\"Pump turned ON\"}");
        } else if (command == "PUMP_OFF") {
            digitalWrite(RELAY_PUMP, HIGH);
            Serial.println("{\"response\":\"Pump turned OFF\"}");
        } else if (command == "COOLER_ON") {
            digitalWrite(RELAY_COOLER, LOW);
            Serial.println("{\"response\":\"Cooler turned ON\"}");
        } else if (command == "COOLER_OFF") {
            digitalWrite(RELAY_COOLER, HIGH);
            Serial.println("{\"response\":\"Cooler turned OFF\"}");
        } else if (command == "STATUS") {
            FirebaseJson statusJson;
            statusJson.set("temperature", currentTemperature);
            statusJson.set("humidity", currentHumidity);
            statusJson.set("soil_moisture", currentSoilMoisture);
            statusJson.set("water_level", currentWaterLevel);
            String statusStr;
            statusJson.toString(statusStr, true);
            Serial.println(statusStr);
        } else {
            Serial.println("{\"error\":\"Unknown command\"}");
        }
    }

    delay(1000); // Adjusted delay for better responsiveness
}