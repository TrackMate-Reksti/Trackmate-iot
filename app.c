##include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h> // Provide the token generation process info.

#include <Servo.h>

// Define for GPS
#define GPS_BAUDRATE 9600  // The default baudrate of NEO-6M is 9600

// Define for Firestore
#define FIREBASE_PROJECT_ID "kiddy-65de6"

// Define Serial Number
String SERIAL_NUMBER = "Kiddy12345678";

// SSID/Password Wifi
const char* ssid = "Galaxy";
const char* password = "12571257";

// Define Buzzer
#define BUZZER_PIN 5

// Define Servo
#define SERVO_PIN 9

// Object GPS
unsigned long lastMsg = 0;
TinyGPSPlus gps; 

// Objek servo
Servo myServo;
int degree = 90;
int state = 1;

// Firestore objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables to store the previous GPS coordinates
double prevLatitude = 0.0;
double prevLongitude = 0.0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(GPS_BAUDRATE);

  // Attach servo ke pin
  myServo.attach(SERVO_PIN);
  
  // Set up buzzer pin as output
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Make sure the buzzer is off initially

  // Connect to wifi
  setupWifi();

  // Configure Firestore
  setupFirestore();
}

void setupWifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupFirestore() {
  config.api_key = "YOUR_FIREBASE_API_KEY";
  config.project_id = FIREBASE_PROJECT_ID;

  auth.user.email = "YOUR_FIREBASE_USER_EMAIL";
  auth.user.password = "YOUR_FIREBASE_USER_PASSWORD";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.ready()) {
    Serial.println("Failed to connect to Firebase");
  }
}

void gpsLoop() {
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    // Read GPS data
    if (Serial2.available() > 0) {
      if (gps.encode(Serial2.read())) {
        if (gps.location.isValid()) {
          double currentLatitude = gps.location.lat();
          double currentLongitude = gps.location.lng();

          Serial.print(F("latitude: "));
          Serial.println(currentLatitude);

          Serial.print(F("longitude: "));
          Serial.println(currentLongitude);

          Serial.print(F("altitude: "));
          if (gps.altitude.isValid())
            Serial.println(gps.altitude.meters());
          else
            Serial.println(F("INVALID"));

          Serial.print(F("speed: "));
          if (gps.speed.isValid()) {
            Serial.print(gps.speed.kmph());
            Serial.println(F(" km/h"));
          } else {
            Serial.println(F("INVALID"));
          }

          // Check if the latitude or longitude has changed
          if (currentLatitude != prevLatitude || currentLongitude != prevLongitude) {
            // Activate the buzzer
            digitalWrite(BUZZER_PIN, HIGH);
            delay(1000);  // Buzzer on for 1 second
            digitalWrite(BUZZER_PIN, LOW);
            
            // Update previous latitude and longitude
            prevLatitude = currentLatitude;
            prevLongitude = currentLongitude;
          }

          // Push GPS data to Firestore
          String documentPath = "devices/" + SERIAL_NUMBER;
          FirebaseJson content;
          content.set("fields/latitude/doubleValue", currentLatitude);
          content.set("fields/longitude/doubleValue", currentLongitude);

          if (gps.altitude.isValid()) {
            content.set("fields/altitude/doubleValue", gps.altitude.meters());
          }

          if (gps.speed.isValid()) {
            content.set("fields/speed/doubleValue", gps.speed.kmph());
          }

          if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "")) {
            Serial.println("Data sent to Firestore successfully");
          } else {
            Serial.println("Failed to send data to Firestore");
            Serial.println(fbdo.errorReason());
          }
        }
      }
    }

    if (millis() > 5000 && gps.charsProcessed() < 10) {
      Serial.println(F("No GPS data received: check wiring"));
    }
  }
}

void loop() {
  gpsLoop();
}

