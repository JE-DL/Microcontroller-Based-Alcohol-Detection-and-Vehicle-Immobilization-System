#include <Wire.h> // Library for I2C communication, used by OLED display
#include <Adafruit_GFX.h> // Core graphics library for OLED
#include <Adafruit_SSD1306.h> // OLED display driver library

// OLED display configuration constants
#define SCREEN_WIDTH 128 // OLED width, in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Create display object

// Pin assignments for hardware components
#define MQ3_PIN 34 // Analog pin connected to MQ3 alcohol sensor output
#define POT_PIN 33 // Analog pin connected to potentiometer
#define RELAY_PIN 12 // Digital output pin controlling relay for motor power
#define HANDBRAKE_BTN_PIN 35 // Digital input pin connected to handbrake push-button switch
#define GAS_PEDAL_BTN_PIN 32 // Digital input pin connected to gas pedal push-button switch

// Threshold for alcohol detection
#define ALCOHOL_THRESHOLD 1600 // Sensor reading above this value means alcohol detected

// Variables to track system states and input debouncing
bool alcoholDetected = false; // True if alcohol sensor reading exceeds threshold
bool handbrakeReleased = false; // True if handbrake is considered released
bool prevHandbrakeBtn = false; // Previous button state to detect press edge

unsigned long lastDebounceTime = 0; // Time when handbrake button was last toggled
const unsigned long debounceDelay = 200; // Debounce delay in milliseconds

void setup() {
  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true); // Halt if OLED fails
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing System...");
  display.display();
  delay(2000); // Show init message for 2 seconds

  // Setup pin modes
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(HANDBRAKE_BTN_PIN, INPUT_PULLUP); // Button is HIGH when not pressed
  pinMode(GAS_PEDAL_BTN_PIN, INPUT_PULLUP);
  // Potentiometer (POT_PIN) is read via analogRead, no pinMode needed

  digitalWrite(RELAY_PIN, LOW); // Start with motor OFF
}

void loop() {
  // Read analog inputs
  int alcoholLevel = analogRead(MQ3_PIN);     // Read MQ-3 sensor
  int potValue = analogRead(POT_PIN);         // Read potentiometer value (0–4095)

  // Determine if alcohol is detected
  alcoholDetected = alcoholLevel > ALCOHOL_THRESHOLD;

  // Read digital inputs (buttons)
  bool handbrakeBtnPressed = digitalRead(HANDBRAKE_BTN_PIN) == LOW;
  bool gasPedalPressed = digitalRead(GAS_PEDAL_BTN_PIN) == LOW;

  // Handle debounced handbrake toggle (only if no alcohol is detected)
  if (handbrakeBtnPressed && !prevHandbrakeBtn && (millis() - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = millis();
    if (!alcoholDetected) {
      handbrakeReleased = !handbrakeReleased; // Toggle state
    }
  }
  prevHandbrakeBtn = handbrakeBtnPressed;

  // Override: If alcohol is detected, force handbrake engaged
  if (alcoholDetected) {
    handbrakeReleased = false;
  }

  // Decide if motor should be ON
  bool motorActive = handbrakeReleased && gasPedalPressed && !alcoholDetected;
  digitalWrite(RELAY_PIN, motorActive ? HIGH : LOW); // Turn motor ON or OFF

  // Display updates
  display.clearDisplay();

  // Alcohol level display
  display.setCursor(0, 0);
  display.print("Alcohol: ");
  display.println(alcoholLevel);

  // Potentiometer value display
  display.setCursor(0, 10);
  display.print("Potentiometer: ");
  display.println(potValue); // This is a raw analog value (0–4095)

  // System status display
  if (alcoholDetected) {
    display.setCursor(0, 30);
    display.println("ALCOHOL DETECTED!");
    display.setCursor(0, 40);
    display.println("Drive Locked!");
  } else {
    display.setCursor(0, 30);
    display.print("Handbrake: ");
    display.println(handbrakeReleased ? "Released" : "Engaged");


    display.setCursor(0, 40);
    if (motorActive) {
      display.println("Driving...");
    } else if (gasPedalPressed && !handbrakeReleased) {
      display.println("Release handbrake");
    } else {
      display.println("Idle");
    }
  }

  display.display(); // Show display buffer
  delay(50); // Short delay for stability
}
