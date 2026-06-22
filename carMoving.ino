const byte MOTOR_1 = 5; // This is a PWM pin
const byte MOTOR_2 = 6;
int PWM_speed = 100; 

void setup() 
{
  pinMode(MOTOR_1, OUTPUT);
  pinMode(MOTOR_2, OUTPUT);
}

void loop() 
{
  analogWrite(MOTOR_1, PWM_speed);
  analogWrite(MOTOR_2, PWM_speed);
}

