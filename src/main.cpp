#include <Arduino.h>
#include "motor.h"
#include "HX711.h"
#include <WiFi.h>
#include <WebSocketsServer.h>

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
Motor motor1{BLDC1_PIN, 0, BLDC1_CHAN, 0, BLDC_FREQ, MOTOR_TYPE_BLDC};
Motor motor2{BLDC2_PIN, 0, BLDC2_CHAN, 0, BLDC_FREQ, MOTOR_TYPE_BLDC};

// Variables to store motor speeds
int motor1_speed = 0;
int motor2_speed = 0;

WebSocketsServer webSocket = WebSocketsServer(81);
#define USE_SERIAL Serial1

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16)
{
  const uint8_t *src = (const uint8_t *)mem;
  USE_SERIAL.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for (uint32_t i = 0; i < len; i++)
  {
    if (i % cols == 0)
    {
      USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    USE_SERIAL.printf("%02X ", *src);
    src++;
  }
  USE_SERIAL.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    USE_SERIAL.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    webSocket.sendTXT(num, "Connected");
  }
  break;
  case WStype_TEXT:
    USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

    // send message to client
    // webSocket.sendTXT(num, "message here");

    // send data to all connected clients
    // webSocket.broadcastTXT("message here");
    break;
  case WStype_BIN:
    USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
    hexdump(payload, length);

    // send message to client
    // webSocket.sendBIN(num, payload, length);
    break;
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
}

void setup()
{
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

  scale.set_scale(); // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();      // reset the scale to 0
  printf("\nCALIBRATE NOW\n");
  delay(4000);
  scale.set_scale(scale.get_units(10) / 1000);
  printf("\nCalibration complete.\n");

  motorInit(motor1);
  motorInit(motor2);

  setMotorSpeed(0, motor1);
  setMotorSpeed(0, motor1);

  const char *ssid = "MATEBOOK5667";
  const char *password = "8gB341?4";

  WiFi.mode(WIFI_STA); // Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
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
  Serial.printf("%lf", scale.get_units(), 1);
  Serial.printf("\t| average:\t");
  Serial.printf("%lf\n\r", scale.get_units(10), 1);

  scale.power_down(); // put the ADC in sleep mode
  delay(500);
  scale.power_up();

  webSocket.loop();
}
