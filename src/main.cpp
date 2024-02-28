#include <Arduino.h>
#include "motor.h"
#include "HX711.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <time.h>

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

int run_benchmark = 0;
int benchmark_duration;
time_t benchmark_start;
time_t current_time;

WebSocketsServer webSocket = WebSocketsServer(81);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
  }
  break;
  case WStype_TEXT:
  {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    if (doc.containsKey("command"))
    {
      const char *command = doc["command"];
      if (strcmp(command, "stop") == 0)
      {
        run_benchmark = 0;
      }
      else if (strcmp(command, "start") == 0)
      {
        run_benchmark = 1;
      }
      Serial.printf("[%u] get command: %s\n", num, command);
    }

    if (doc.containsKey("motor1_speed") && doc.containsKey("motor2_speed"))
    {
      motor1_speed = doc["motor1_speed"];
      motor2_speed = doc["motor2_speed"];
      Serial.printf("[%u] get motorspeed: %d\n", num, motor1_speed);
    }

    if (doc.containsKey("benchmark_duration"))
    {
      benchmark_duration = doc["benchmark_duration"];
      Serial.printf("[%u] get duration: %d\n", num, benchmark_duration);
    }

    // webSocket.sendTXT(num, "message here");
    benchmark_start = time(NULL);
  }
  break;
  case WStype_BIN:
    Serial.printf("[%u] get binary length: %u\n", num, length);

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
  delay(5000);
  scale.set_scale(scale.get_units(10) / 1000);
  printf("\nCalibration complete.\n");

  motorInit(motor1);
  motorInit(motor2);

  setMotorSpeed(0, motor1);
  setMotorSpeed(0, motor2);

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

int iteration = 0;
float force_measurements[5];

void loop()
{
  webSocket.loop();
  current_time = time(NULL);
  if (current_time - benchmark_start > benchmark_duration && run_benchmark == 1)
  {
    run_benchmark = 0;
  }
  if (run_benchmark == 0)
  {
    setMotorSpeed(0, motor1);
    setMotorSpeed(0, motor2);
    return;
  }

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

  if (iteration == 5)
  {
    Serial.printf("one reading:\t");
    Serial.printf("%lf", scale.get_units(), 1);
    Serial.printf("\t| average:\t");
    Serial.printf("%lf\n\r", scale.get_units(10), 1);
  }

  scale.power_down(); // put the ADC in sleep mode
  delay(500);
  scale.power_up();

  if (iteration > sizeof(force_measurements) / sizeof(force_measurements[0]) - 1)
  {
    iteration = 0;
    JsonDocument doc;
    // Add values in the document
    doc["i"] = iteration;
    // Add an array
    JsonArray measurement_array = doc["force_measurements"].to<JsonArray>();
    for (size_t i = 0; i < 5; i++)
    {
      measurement_array.add(force_measurements[i]);
    }

    String serialized_json;
    serializeJson(doc, serialized_json);
    webSocket.broadcastTXT(serialized_json);
  }
  else
  {
    // force_measurements[iteration] = scale.get_units(10);
    force_measurements[iteration] = random(0, 60);
    iteration++;
  }
}
