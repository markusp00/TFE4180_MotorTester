#include <Arduino.h>
#include "motor.h"
#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 1;

HX711 scale;

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

  Serial.println("HX711 Demo");

  Serial.println("Initializing the scale");

  // Initialize library with data output pin, clock input pin and gain factor.
  // Channel selection is made by passing the appropriate gain:
  // - With a gain factor of 64 or 128, channel A is selected
  // - With a gain factor of 32, channel B is selected
  // By omitting the gain factor parameter, the library
  // default "128" (Channel A) is used here.
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.printf("Before setting up the scale:");
  Serial.printf("read: \t\t");
  Serial.printf("%d", scale.read());			// print a raw reading from the ADC

  Serial.printf("read average: \t\t");
  Serial.printf("%d", scale.read_average(20));  	// print the average of 20 readings from the ADC

  Serial.printf("get value: \t\t");
  Serial.printf("%d", scale.get_value(5));		// print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.printf("get units: \t\t");
  Serial.printf("%d", scale.get_units(5), 1);	// print the average of 5 readings from the ADC minus tare weight (not set) divided
						// by the SCALE parameter (not set yet)

  scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();				        // reset the scale to 0

  Serial.printf("After setting up the scale:");

  Serial.printf("read: \t\t");
  Serial.printf("%d", scale.read());                 // print a raw reading from the ADC

  Serial.printf("read average: \t\t");
  Serial.printf("%d", scale.read_average(20));       // print the average of 20 readings from the ADC

  Serial.printf("get value: \t\t");
  Serial.printf("%d", scale.get_value(5));		// print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.printf("get units: \t\t");
  Serial.printf("%d", scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
						// by the SCALE parameter set with set_scale

  Serial.println("Readings:");

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

  // Serial.printf("Motor1: %d \t Motor2: %d \n\r", motor1_speed, motor2_speed);

  Serial.printf("one reading:\t");
  Serial.printf("%d", scale.get_units(), 1);
  Serial.printf("\t| average:\t");
  Serial.printf("%d\n\r", scale.get_units(10), 1);

  scale.power_down();			        // put the ADC in sleep mode
  delay(500);
  scale.power_up();
}