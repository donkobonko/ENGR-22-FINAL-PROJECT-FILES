// ===============================
// Car Radio Receiver
// Receives packet from break beam
// Speeds up for 3 seconds, then stops
// ===============================

#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>

// Motor pins
const byte MOTOR_LEFT = 5;
const byte MOTOR_RIGHT = 6;

// Normal driving speeds
int LEFT_PWM = 130;
int RIGHT_PWM = 130;

// Boost settings
int BOOST_PWM_ADD = 120;

int leftBoostPWM;
int rightBoostPWM;

const int MIN_PWM = 0;
const int MAX_PWM = 255;

// Button pin
const byte BUTTON_PIN = 4;

// Wheel sensor pins
const byte SENSOR_LEFT = 3;
const byte SENSOR_RIGHT = 2;

const float SLOTS_ON_DISC = 20.0;

volatile unsigned long pulseCountLeft = 0;
volatile unsigned long pulseCountRight = 0;

// Radio pins for car Arduino
RF24 radio(7, 8); // CE, CSN

const byte address[6] = "00001";

struct SensorMessage {
  int trackID;
  unsigned long passTime;
};

SensorMessage receivedMsg;

bool carEnabled = true;
bool motorsRunning = false;
bool boosting = false;

unsigned long boostStartTime = 0;
const unsigned long BOOST_TIME = 3000; // speed up for 3 seconds

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 700;

bool lastButtonReading = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

void countLeftPulse() {
  pulseCountLeft++;
}

void countRightPulse() {
  pulseCountRight++;
}

void calculateBoostSpeeds() {
  leftBoostPWM = LEFT_PWM + BOOST_PWM_ADD;
  rightBoostPWM = RIGHT_PWM + BOOST_PWM_ADD;

  leftBoostPWM = constrain(leftBoostPWM, MIN_PWM, MAX_PWM);
  rightBoostPWM = constrain(rightBoostPWM, MIN_PWM, MAX_PWM);
}

void runNormalSpeed() {
  analogWrite(MOTOR_LEFT, LEFT_PWM);
  analogWrite(MOTOR_RIGHT, RIGHT_PWM);
  motorsRunning = true;
}

void runBoostSpeed() {
  calculateBoostSpeeds();

  analogWrite(MOTOR_LEFT, leftBoostPWM);
  analogWrite(MOTOR_RIGHT, rightBoostPWM);
  motorsRunning = true;
}

void stopMotors() {
  analogWrite(MOTOR_LEFT, 0);
  analogWrite(MOTOR_RIGHT, 0);
  motorsRunning = false;
}

// Button behavior:
// OFF -> ON resets the car and starts normal speed
// ON  -> OFF stops the car
void checkButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        carEnabled = !carEnabled;

        if (carEnabled) {
          Serial.println("BUTTON: CAR ON");
          boosting = false;
          runNormalSpeed();
        } else {
          Serial.println("BUTTON: CAR OFF");
          boosting = false;
          stopMotors();
        }
      }
    }
  }

  lastButtonReading = reading;
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("CAR RECEIVER STARTING");
  Serial.println("Break beam signal = boost, then stop.");

  pinMode(MOTOR_LEFT, OUTPUT);
  pinMode(MOTOR_RIGHT, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(SENSOR_LEFT, INPUT_PULLUP);
  pinMode(SENSOR_RIGHT, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SENSOR_LEFT), countLeftPulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_RIGHT), countRightPulse, FALLING);

  carEnabled = true;
  calculateBoostSpeeds();
  runNormalSpeed();

  Serial.println("Motors running at normal speed");

  if (!radio.begin()) {
    Serial.println("RADIO FAILED");
    while (1);
  }

  Serial.println("RADIO OK");

  radio.openReadingPipe(1, address);

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setCRCLength(RF24_CRC_16);
  radio.setAutoAck(false);

  radio.startListening();

  Serial.println("RECEIVER READY");
  Serial.println("Waiting for break beam packet...");
}

void loop() {
  checkButton();

  // Receive break beam packet
  if (radio.available()) {
    radio.read(&receivedMsg, sizeof(receivedMsg));

    Serial.println();
    Serial.println("***** BREAK BEAM PACKET RECEIVED *****");
    Serial.print("Track ID: ");
    Serial.println(receivedMsg.trackID);
    Serial.print("Pass time: ");
    Serial.print(receivedMsg.passTime);
    Serial.println(" ms");

    if (carEnabled && !boosting) {
      boosting = true;
      boostStartTime = millis();
      runBoostSpeed();

      Serial.println("Car speeding up. It will stop after boost time.");
    }
  }

  // After boost time, stop completely
  if (carEnabled && boosting) {
    if (millis() - boostStartTime >= BOOST_TIME) {
      boosting = false;
      carEnabled = false;
      stopMotors();

      Serial.println("Boost finished. Car stopped.");
      Serial.println("Press button to turn car ON again.");
    }
  }

  // RPM/status print
  if (millis() - lastPrintTime >= PRINT_INTERVAL) {
    noInterrupts();

    unsigned long leftPulses = pulseCountLeft;
    unsigned long rightPulses = pulseCountRight;

    pulseCountLeft = 0;
    pulseCountRight = 0;

    interrupts();

    float leftRPM = (leftPulses / SLOTS_ON_DISC) * (60000.0 / PRINT_INTERVAL);
    float rightRPM = (rightPulses / SLOTS_ON_DISC) * (60000.0 / PRINT_INTERVAL);

    Serial.print("Car: ");
    Serial.print(carEnabled ? "ON" : "OFF");

    Serial.print(" | Motors: ");
    Serial.print(motorsRunning ? "RUNNING" : "STOPPED");

    Serial.print(" | Mode: ");
    Serial.print(boosting ? "BOOST" : "NORMAL/OFF");

    Serial.print(" | L RPM: ");
    Serial.print(leftRPM);

    Serial.print(" | R RPM: ");
    Serial.print(rightRPM);

    Serial.print(" | L PWM: ");
    Serial.print(boosting ? leftBoostPWM : LEFT_PWM);

    Serial.print(" | R PWM: ");
    Serial.println(boosting ? rightBoostPWM : RIGHT_PWM);

    lastPrintTime = millis();
  }

  // Keep motor command active
  if (carEnabled) {
    if (boosting) {
      runBoostSpeed();
    } else {
      runNormalSpeed();
    }
  } else {
    stopMotors();
  }
}
