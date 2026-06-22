#include <SPI.h>
#include <RF24.h>


const uint8_t CAR_ID = 1;
const int ENCODER_SLOTS = 20;
const float WHEEL_CIRCUMFERENCE_CM = 21.4;
const uint8_t INITIAL_PWM = 180;


const uint64_t TX_PIPE = 0xE8E8F0F0A1LL;
const uint64_t RX_PIPE = 0xE8E8F0F0B1LL;


// Pins
#define ENCODER_PIN 2
#define MOTOR_IN1 7
#define MOTOR_IN2 8
#define MOTOR_ENA 6
#define RADIO_CE 9
#define RADIO_CSN 10


struct TelemetryPacket {
  uint8_t car_id;
  float velocity_cms;
  uint32_t timestamp_ms;
};


struct CommandPacket {
  uint8_t car_id;
  uint8_t target_pwm;
};


RF24 radio(RADIO_CE, RADIO_CSN);


volatile uint32_t pulse_count = 0;


uint32_t last_calc_ms = 0;
uint32_t last_send_ms = 0;


float current_velocity = 0.0;
uint8_t current_pwm = INITIAL_PWM;


const uint32_t CALC_INTERVAL_MS = 100;


// Interrupt function
void onEncoderPulse() {
  pulse_count++;
}


void setup() {
  Serial.begin(9600);


  // Motor setup
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);


  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);


  analogWrite(MOTOR_ENA, current_pwm);


  // Encoder
  pinMode(ENCODER_PIN, INPUT_PULLUP);


  attachInterrupt(
    digitalPinToInterrupt(ENCODER_PIN),
    onEncoderPulse,
    RISING
  );


  // Radio setup
  if (!radio.begin()) {
    Serial.println("ERROR: nRF24L01+ not found");
    while (true);
  }


  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(3, 5);


  radio.openWritingPipe(TX_PIPE);
  radio.openReadingPipe(1, RX_PIPE);


  radio.startListening();


  last_calc_ms = millis();
  last_send_ms = millis();
}


float calculateVelocity() {
  noInterrupts();


  uint32_t count = pulse_count;
  pulse_count = 0;


  interrupts();


  float rotations = (float)count / ENCODER_SLOTS;
  float distance = rotations * WHEEL_CIRCUMFERENCE_CM;


  float velocity =
    distance / (CALC_INTERVAL_MS / 1000.0);


  return velocity;
}


void loop() {
  uint32_t now = millis();


  // Recalculate velocity
  if (now - last_calc_ms >= CALC_INTERVAL_MS) {


    current_velocity = calculateVelocity();


    last_calc_ms = now;


    Serial.print("v: ");
    Serial.print(current_velocity, 1);
    Serial.print(" cm/s | PWM: ");
    Serial.println(current_pwm);
  }


  // Send telemetry
  if (now - last_send_ms >= 100) {


    radio.stopListening();


    TelemetryPacket pkt;


    pkt.car_id = CAR_ID;
    pkt.velocity_cms = current_velocity;
    pkt.timestamp_ms = now;


    bool ok = radio.write(&pkt, sizeof(pkt));


    if (!ok) {
      Serial.println("Warning: TX failed");
    }


    radio.startListening();


    last_send_ms = now;
  }


  // Receive commands
  if (radio.available()) {


    CommandPacket cmd;


    radio.read(&cmd, sizeof(cmd));


    if (cmd.car_id == CAR_ID) {


      current_pwm = cmd.target_pwm;


      analogWrite(MOTOR_ENA, current_pwm);


      Serial.print("New PWM: ");
      Serial.println(current_pwm);
    }
  }
}
