#include <Arduino.h>
#include "motor.h"

// Pins
#define BLDC1_PIN 35
#define BLDC2_PIN 36

// Frequencies
#define BLDC_FREQ 50

// LEDC channels esp-hal uses modulo 4 to select timers for different channels, therefore BLDC is the only odd channel 
#define BLDC1_CHAN 1
#define BLDC2_CHAN 2

// Motor structs setup
Motor motor1 {BLDC1_PIN, 0, BLDC1_CHAN, 0, BLDC_FREQ, MOTOR_TYPE_BLDC};
Motor motor2 {BLDC2_PIN, 0, BLDC2_CHAN, 0, BLDC_FREQ, MOTOR_TYPE_BLDC};

// Variables to store motor speeds
int motor1_speed = 0;
int motor2_speed = 0;

void setup() {
  Serial.begin(115200);

  motorInit(motor1);
  motorInit(motor2);

  setMotorSpeed(0, motor1);
  setMotorSpeed(0, motor1);
}

void loop() {
  motor1_speed++;
  motor2_speed--;
  if (motor1_speed > 127) 
  {
    motor1_speed = -127;
  }

  if (motor2_speed < -127) 
  {
    motor2_speed = 127;
  }

  setMotorSpeed(motor1_speed, motor1);
  setMotorSpeed(motor2_speed, motor2);

  Serial.printf("Motor1: %d \t Motor2: %d \n\r", motor1_speed, motor2_speed);

  delay(5);
}
