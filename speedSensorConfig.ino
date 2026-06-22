#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>




// Motor Config
const byte MOTOR_1 = 5; // This is a PWM pin
const byte MOTOR_2 = 6;
int PWM_speed = 100; // Change this to modulate your speeds for the motor




// --- Speed Sensor Config ---
// There are only 2 interrupt pins on the Arduino, D2 and D3.
// D2 is Interrupt 0 and D3 is Interrupt 1.
const byte SENSOR_PIN = 2;       // Must be an interrupt pin
const float SLOTS_ON_DISC = 20.0; // Change this to the number of slots/teeth on our wheels
volatile unsigned long pulseCount = 0;
unsigned long prevMillis = 0;
const unsigned long INTERVAL = 1000; // Update interval in milliseconds (1000ms = 1s)




// --- nRF24L01 Config ---
// MAKE SURE YOU CONFIGURE THIS PART TO MATCH HOW YOU SET IT UP! these are just dummy numbers.
// Since you are allowed to match those pins on the radio to any D pin, just keep track of which pin you're using for CE and CSN.
int CE = 9;
int CSN = 10;
RF24 radio(CE, CSN); // CE on Pin 9, CSN on Pin 10
const byte address[6] = "00001"; // Must match the receiver's address




// Interrupt Service Routine (ISR)
void countPulse() {
  pulseCount++;
}




void setup() {
  Serial.begin(115200);




  // Motor Setup
  pinMode(MOTOR_1, OUTPUT);
  pinMode(MOTOR_2, OUTPUT);
 
  // Speed Sensor Setup
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), countPulse, FALLING);
 
  // Radio Setup
  if (!radio.begin()) {
    Serial.println("nRF24L01 hardware not responding!");
    while (1); // Halt if radio isn't wired correctly
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW); // Low power for close-range testing
  radio.stopListening();         // Set as transmitter
 
  Serial.println("Transmitter Ready.");
}




void loop() {
  analogWrite(MOTOR_1, PWM_speed);
  analogWrite(MOTOR_2, PWM_speed);








  unsigned long currentMillis = millis();
 
  if (currentMillis - prevMillis >= INTERVAL) {
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();
   
    // Calculate RPM
    float rpm = (pulses / SLOTS_ON_DISC) * (60000.0 / INTERVAL);
   
    // Send RPM via Wireless Transceiver
    bool success = radio.write(&rpm, sizeof(rpm));
   
    // Local debugging display
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

