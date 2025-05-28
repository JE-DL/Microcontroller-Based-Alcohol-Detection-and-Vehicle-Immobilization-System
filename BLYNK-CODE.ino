#include <Wire.h> // Library for I2C communication (used by OLED)
#include <Adafruit_GFX.h> // Core graphics library for OLED
#include <Adafruit_SSD1306.h> // OLED display driver library


#include <WiFi.h> // WiFi library for ESP32
#include <BlynkSimpleEsp32.h> // Blynk library for ESP32


#define BLYNK_TEMPLATE_ID "TMPL6Y1T-2Bz6" // Blynk template ID
#define BLYNK_TEMPLATE_NAME "ALCOHOL DETECTION" // Blynk project name
#define BLYNK_AUTH_TOKEN "D7zx46_xqtFfJFIkojKmC0VsyWOlbM6C" // Blynk authentication token


char ssid[] = "ZTE_2.4G_9dt9n6"; // WiFi SSID
char pass[] = "yQcHFnFE"; // WiFi password


#define SCREEN_WIDTH 128 // OLED width, in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels
#define OLED_RESET -1 // OLED reset pin (not used here)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED display object


#define MQ3_PIN 34 // Analog pin for MQ3 alcohol sensor
#define RELAY_PIN 12 // Digital output pin to control relay (motor)
#define HANDBRAKE_BTN_PIN 35 // Input pin for handbrake button
#define GAS_PEDAL_BTN_PIN 32 // Input pin for gas pedal button


#define ALCOHOL_THRESHOLD 1600 // Alcohol threshold value from sensor


bool alcoholDetected = false; // Tracks if alcohol is detected
bool handbrakeReleased = false; // Tracks if handbrake is released
bool prevHandbrakeBtn = false; // Tracks previous handbrake button state


unsigned long lastDebounceTime = 0; // Last time the handbrake button changed
const unsigned long debounceDelay = 200; // Debounce time in milliseconds


unsigned long lastNotificationTime = 0; // Time of last Blynk alert
const unsigned long notificationCooldown = 30000; // Delay between alerts (30 seconds)
bool notificationSent = false; // Prevents multiple alerts


void setup() {
  Serial.begin(115200); // Start serial communication for debugging


  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true); // If OLED init fails, stay here
  }


  display.clearDisplay(); // Clear display buffer
  display.setTextSize(1); // Text size
  display.setTextColor(SSD1306_WHITE); // White text
  display.setCursor(0, 0); // Cursor position
  display.println("Initializing System...");
  display.display(); // Show on OLED
  delay(2000); // Pause to read


  pinMode(RELAY_PIN, OUTPUT); // Relay output
  pinMode(HANDBRAKE_BTN_PIN, INPUT_PULLUP); // Handbrake button input
  pinMode(GAS_PEDAL_BTN_PIN, INPUT_PULLUP); // Gas pedal button input


  digitalWrite(RELAY_PIN, LOW); // Start with motor OFF


  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); // Connect to Blynk
}


void loop() {
  Blynk.run(); // Run Blynk tasks


  int alcoholLevel = analogRead(MQ3_PIN); // Read alcohol sensor
  alcoholDetected = alcoholLevel > ALCOHOL_THRESHOLD; // Check threshold


  bool handbrakeBtnPressed = digitalRead(HANDBRAKE_BTN_PIN) == LOW; // Read handbrake button
  bool gasPedalPressed = digitalRead(GAS_PEDAL_BTN_PIN) == LOW; // Read gas pedal button


  Blynk.virtualWrite(V0, alcoholLevel); // Send alcohol level to Blynk
  Blynk.virtualWrite(V1, alcoholDetected ? 1 : 0); // Send detection status to Blynk


  if (handbrakeBtnPressed && !prevHandbrakeBtn && (millis() - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = millis(); // Update time
    if (!alcoholDetected) {
      handbrakeReleased = !handbrakeReleased; // Toggle state
    }
  }
  prevHandbrakeBtn = handbrakeBtnPressed; // Update previous state


  if (alcoholDetected) {
    handbrakeReleased = false; // Force handbrake engaged


    if (!notificationSent || millis() - lastNotificationTime > notificationCooldown) {
      Blynk.logEvent("alcohol_detected", "Alcohol level too high!"); // Send alert
      notificationSent = true; // Update flag
      lastNotificationTime = millis(); // Record time
    }
  } else {
    notificationSent = false; // Reset alert flag
  }


  bool motorActive = handbrakeReleased && gasPedalPressed && !alcoholDetected; // Check motor condition
  digitalWrite(RELAY_PIN, motorActive ? HIGH : LOW); // Control relay


  display.clearDisplay(); // Clear OLED
  display.setCursor(0, 0);
  display.print("Alcohol: ");
  display.println(alcoholLevel); // Show alcohol value


  if (alcoholDetected) {
    display.setCursor(0, 20);
    display.println("ALCOHOL DETECTED!");
    display.setCursor(0, 30);
    display.println("Drive Locked!"); // Warning message
  } else {
    display.setCursor(0, 20);
    display.print("Handbrake: ");
    display.println(handbrakeReleased ? "Released" : "Engaged"); // Show handbrake


    display.setCursor(0, 30);
    if (motorActive) {
      display.println("Driving..."); // Show driving status
    } else if (gasPedalPressed && !handbrakeReleased) {
      display.println("Release handbrake"); // Instruction
    } else {
      display.println("Idle"); // Default status
    }
  }


  display.display(); // Update OLED screen
  delay(50); // Short delay
}
