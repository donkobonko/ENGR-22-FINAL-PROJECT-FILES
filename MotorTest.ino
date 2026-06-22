const byte MOTOR_1 = 5;
const byte MOTOR_2 = 6;

void setup() {
  pinMode(MOTOR_1, OUTPUT);
  pinMode(MOTOR_2, OUTPUT);
}

void loop() {
  analogWrite(MOTOR_1, 255);
  analogWrite(MOTOR_2, 255);
  delay(500);
  analogWrite(MOTOR_1, 0);
  analogWrite(MOTOR_2, 0);
  delay(500);
  analogWrite(MOTOR_1, 255);
  analogWrite(MOTOR_2, 255);
  delay(500);
  analogWrite(MOTOR_1, 0);
  analogWrite(MOTOR_2, 0);
  delay(500);
  analogWrite(MOTOR_1, 255);
  analogWrite(MOTOR_2, 255);
  delay(500);
  analogWrite(MOTOR_1, 0);
  analogWrite(MOTOR_2, 0);
  delay(500);
}
