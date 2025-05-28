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
#define RELAY_PIN 12 // Digital output pin controlling relay for motor power
#define HANDBRAKE_BTN_PIN 35 // Digital input pin connected to handbrake push-button switch
#define GAS_PEDAL_BTN_PIN 32 // Digital input pin connected to gas pedal push-button switch

// Threshold for alcohol detection
#define ALCOHOL_THRESHOLD 1600 // Sensor reading above this value means alcohol detected

// Variables to track system states and input debouncing
bool alcoholDetected = false; // True if alcohol sensor reading exceeds threshold
bool handbrakeReleased = false; // True if handbrake is considered released, false if engaged
bool prevHandbrakeBtn = false; // Stores previous reading of handbrake button for edge detection

unsigned long lastDebounceTime = 0; // Time when the handbrake button was last toggled
const unsigned long debounceDelay = 200; // Debounce time to avoid multiple triggers (milliseconds)

void setup() {
  // Initialize the OLED display with I2C address 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // If the display fails to initialize, stop the program here
    while (true);
  }

  // Clear the OLED display buffer
  display.clearDisplay();

  // Set text size to normal (1)
  display.setTextSize(1);

  // Set text color to white (pixel on)
  display.setTextColor(SSD1306_WHITE);

  // Position the cursor to the top-left corner
  display.setCursor(0, 0);

  // Print initialization message on display buffer
  display.println("Initializing System...");

  // Send buffer to the physical OLED display
  display.display();

  // Pause 2 seconds to show the message
  delay(2000);

  // Configure the relay control pin as an output pin
  pinMode(RELAY_PIN, OUTPUT);

  // Configure handbrake button pin as input with internal pull-up resistor enabled
  // This means button is normally HIGH, goes LOW when pressed
  pinMode(HANDBRAKE_BTN_PIN, INPUT_PULLUP);

  // Configure gas pedal button pin similarly as input with internal pull-up resistor
  pinMode(GAS_PEDAL_BTN_PIN, INPUT_PULLUP);

  // Start with the motor turned OFF (relay LOW)
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {
  // Read the analog value from the MQ3 sensor (range 0-4095 on ESP32)
  int alcoholLevel = analogRead(MQ3_PIN);

  // Determine if alcohol level exceeds threshold, update flag accordingly
  alcoholDetected = alcoholLevel > ALCOHOL_THRESHOLD;

  // Read handbrake button state; pressed = LOW (because of pull-up resistor)
  bool handbrakeBtnPressed = digitalRead(HANDBRAKE_BTN_PIN) == LOW;

  // Read gas pedal button state; pressed = LOW
  bool gasPedalPressed = digitalRead(GAS_PEDAL_BTN_PIN) == LOW;

  // Debounce and handle handbrake button toggle:
  // - Check if button is currently pressed AND was not pressed before (rising edge)
  // - Check that debounce delay time has passed since last toggle
  if (handbrakeBtnPressed && !prevHandbrakeBtn && (millis() - lastDebounceTime > debounceDelay)) {
    // Update the debounce timer to current time
    lastDebounceTime = millis();

    // Only allow toggling handbrake if no alcohol detected
    if (!alcoholDetected) {
      // Flip the handbrake state between engaged and released
      handbrakeReleased = !handbrakeReleased;
    }
  }

  // Save current handbrake button state for edge detection next iteration
  prevHandbrakeBtn = handbrakeBtnPressed;

  // If alcohol is detected, force handbrake to engaged (not released)
  if (alcoholDetected) {
    handbrakeReleased = false;
  }

  // Determine if motor should be active:
  // Motor only runs if:
  // - Handbrake is released (not engaged)
  // - Gas pedal button is pressed
  // - No alcohol detected
  bool motorActive = handbrakeReleased && gasPedalPressed && !alcoholDetected;

  // Set relay pin HIGH to power motor ON, LOW to power motor OFF
  digitalWrite(RELAY_PIN, motorActive ? HIGH : LOW);

  // Start updating the OLED display for user feedback
  display.clearDisplay();

  // Display the current alcohol sensor reading on the top-left
  display.setCursor(0, 0);
  display.print("Alcohol: ");
  display.println(alcoholLevel);

  // If alcohol detected, show warning messages
  if (alcoholDetected) {
    display.setCursor(0, 20);
    display.println("ALCOHOL DETECTED!");
    display.setCursor(0, 30);
    display.println("Drive Locked!");
  } else {
    // If no alcohol, display handbrake status
    display.setCursor(0, 20);
    display.print("Handbrake: ");
    display.println(handbrakeReleased ? "Released" : "Engaged");

    // Show driving status or instructions based on inputs
    display.setCursor(0, 30);
    if (motorActive) {
      display.println("Driving...");
    } else if (gasPedalPressed && !handbrakeReleased) {
      display.println("Release handbrake");
    } else {
      display.println("Idle");
    }
  }

  // Push the updated display buffer to the OLED screen
  display.display();

  // Small delay for system stability and to reduce display flicker
  delay(50);
}