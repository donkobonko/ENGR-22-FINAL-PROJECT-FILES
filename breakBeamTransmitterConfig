// ===============================
// Break Beam + Radio Transmitter
// Sends signal when car breaks beam
// ===============================

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ---------- Break Beam Settings ----------
const int TRACK_ID = 1;

// Break beam signal pin
const int SENSOR_PIN = 2;

// ---------- Radio Pins ----------
// These must match the wiring on THIS Arduino
const int CE_PIN = 9;
const int CSN_PIN = 10;

RF24 radio(CE_PIN, CSN_PIN);

// Must match the car receiver address exactly
const byte address[6] = "00001";

// Prevents repeated sends while beam is still blocked
bool detected = false;

// This struct must match what the receiver expects
struct SensorMessage {
  int trackID;
  unsigned long passTime;
};

void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_PIN, INPUT_PULLUP);

  Serial.println("Starting break beam transmitter...");

  if (!radio.begin()) {
    Serial.println("Radio not responding!");
    while (1);
  }

  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);

  // Transmitter mode
  radio.stopListening();

  Serial.println("Break beam transmitter ready.");
}

void loop() {
  // Beam broken = LOW if using INPUT_PULLUP
  if (digitalRead(SENSOR_PIN) == LOW && detected == false) {
    SensorMessage msg;

    msg.trackID = TRACK_ID;
    msg.passTime = millis();

    bool success = radio.write(&msg, sizeof(msg));

    Serial.print("Track ");
    Serial.print(msg.trackID);
    Serial.print(" detected car at ");
    Serial.print(msg.passTime);
    Serial.print(" ms");

    if (success) {
      Serial.println(" -> Radio sent");
    } else {
      Serial.println(" -> Radio failed");
    }

    detected = true;
    delay(300);
  }

  // Beam clear again
  if (digitalRead(SENSOR_PIN) == HIGH) {
    detected = false;
  }
}




