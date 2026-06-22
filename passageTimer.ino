// ===============================
// Track Sensor + Radio Transmitter
// ===============================

// Radio libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ---------- User Settings ----------

// Track sensor ID
// Change to 2 if using the second track sensor
// Change this!
const int TRACK_ID = 1;

// Signal pin from the break beam sensor
// Change this!
const int SENSOR_PIN = 2;

// Radio pins
// Change these to match ratio wiring!
const int CE_PIN = 9;
const int CSN_PIN = 10;

// -----------------------------------


// Create radio object
RF24 radio(CE_PIN, CSN_PIN);

// Radio address
// Receiver must use the same address
// Change this to match receiver address!
const byte address[6] = "00001";


// Prevents multiple detections while a car is still blocking the sensor
bool detected = false;


// Data packet that will be sent wirelessly
struct SensorMessage
{
    int trackID;              // Which track sensor detected the car
    unsigned long passTime;   // Detection timestamp (milliseconds)
};


void setup()
{
    // Open serial monitor for debugging
    Serial.begin(115200);

    // Configure sensor pin as input
    // INPUT_PULLUP means:
    // Normal state = HIGH
    // Triggered state = LOW
    pinMode(SENSOR_PIN, INPUT_PULLUP);

    // Initialize radio
    if (!radio.begin())
    {
        Serial.println("Radio not responding!");

        // Stop program if radio is not connected correctly
        while (1);
    }

    // Configure transmitter settings
    radio.openWritingPipe(address);

    // Use low power (good for short-range testing)
    radio.setPALevel(RF24_PA_LOW);

    // Set radio as transmitter
    radio.stopListening();

    Serial.println("Track sensor transmitter ready.");
}


void loop()
{
    // If sensor is triggered
    // and we have not already recorded this car
    // Change this if sensor output high!(exchange LOW and HIGH)
    if (digitalRead(SENSOR_PIN) == LOW && detected == false)
    {
        // Create a message packet
        SensorMessage msg;

        // Store track sensor ID
        msg.trackID = TRACK_ID;

        // Record current time
        // millis() = milliseconds since Arduino started
        // We use this to caluculate the departure time difference between two cars
        msg.passTime = millis();

        // Send packet through radio
        bool success = radio.write(&msg, sizeof(msg));

        // Local debugging output
        Serial.print("Track ");
        Serial.print(TRACK_ID);

        Serial.print(" detected car at ");

        Serial.print(msg.passTime);

        Serial.print(" ms");

        // Show transmission status
        if (success)
        {
            Serial.println(" -> Radio sent");
        }
        else
        {
            Serial.println(" -> Radio failed");
        }

        // Prevent repeated detections
        detected = true;

        // Small debounce delay
        delay(300);
    }

    // Car has left the sensor area
    // Allow next detection
    // Change this if it's necessary!
    if (digitalRead(SENSOR_PIN) == HIGH)
    {
        detected = false;
    }
}                         
