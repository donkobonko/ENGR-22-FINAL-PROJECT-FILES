#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// --- Motor Config ---
const byte MOTOR_1 = 5;
const byte MOTOR_2 = 6;
int PWM_speed = 100;
bool motorsEnabled = false; // The boolean toggle state for the motors

// --- Button Config ---
const byte BUTTON_PIN = 4; // Change this to wherever you plug the button in
bool lastButtonState = HIGH; // Default state for INPUT_PULLUP is HIGH
bool currentButtonState;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50; // 50ms is usually perfect to ignore the mechanical bounce

// --- Speed Sensor Config ---
const byte SENSOR_PIN = 2;      
const float SLOTS_ON_DISC = 20.0;
volatile unsigned long pulseCount = 0;
unsigned long prevMillis = 0;
const unsigned long INTERVAL = 1000;

// --- nRF24L01 Config ---
int CE = 9;
int CSN = 10;
RF24 radio(CE, CSN);
const byte address[6] = "00001";

// Interrupt Service Routine (ISR)
void countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  // Motor Setup
  pinMode(MOTOR_1, OUTPUT);
  pinMode(MOTOR_2, OUTPUT);
 
  // Button Setup (Internal pull-up resistor enabled)
  // This means the pin reads HIGH when unpressed, and LOW when pressed
  pinMode(BUTTON_PIN, INPUT_PULLUP);
 
  // Speed Sensor Setup
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), countPulse, FALLING);
 
  // Radio Setup
  if (!radio.begin()) {
    Serial.println("nRF24L01 hardware not responding!");
    while (1);
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();        
 
  Serial.println("Transmitter Ready. Press the button to start the motors.");
}

void loop() {
  // --- 1. Button Debounce & Toggle Logic ---
  int reading = digitalRead(BUTTON_PIN);

  // If the switch changed (due to noise or actually pressing it)
  if (reading != lastButtonState) {
    lastDebounceTime = millis(); // Reset the debouncing timer
  }

  // If the state has been stable longer than our 50ms delay
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
   
    // If the state has actually changed from what we last recorded
    if (reading != currentButtonState) {
      currentButtonState = reading;

      // Only toggle the motors if the new button state is LOW (meaning it was just pressed)
      if (currentButtonState == LOW) {
        motorsEnabled = !motorsEnabled; // Flip the boolean! False becomes True, True becomes False.
       
        Serial.print("Motors toggled: ");
        Serial.println(motorsEnabled ? "ON" : "OFF");
      }
    }
  }
  lastButtonState = reading; // Save the reading for the next loop iteration

  // --- 2. Motor Control ---
  if (motorsEnabled) {
    analogWrite(MOTOR_1, PWM_speed);
    analogWrite(MOTOR_2, PWM_speed);
  } else {
    analogWrite(MOTOR_1, 0); // 0 PWM stops the motors
    analogWrite(MOTOR_2, 0);
  }

  // --- 3. RPM Calculation & Broadcast ---
  unsigned long currentMillis = millis();
 
  if (currentMillis - prevMillis >= INTERVAL) {
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();
   
    float rpm = (pulses / SLOTS_ON_DISC) * (60000.0 / INTERVAL);
   
    bool success = radio.write(&rpm, sizeof(rpm));
   
    Serial.print("Local RPM: ");
    Serial.print(rpm, 0);
    if (success) {
      Serial.println(" -> Broadcast Sent Successfully!");
    } else {
      Serial.println(" -> Broadcast Failed (No Receiver Acknowledgment).");
    }
   
    prevMillis = currentMillis;
  }
}
