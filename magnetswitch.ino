#include "DHT.h"
#include <Arduino.h>
#include <string>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#define timeSeconds 10
#define SWITCH_PIN 22 // ESP32 pin connected to the reed switch
#define LED_Door 17
#define LED_restroom 19
#define LED_bedroom 16
#define BUTTON_bedroom 13
#define BUTTON_Door 18
#define DHTPIN 4
#define DHTTYPE DHT22
#define motionSensor 27
#define USE_SERIAL Serial
hw_timer_t *timer = NULL;
volatile bool ledState = false;
// variables will change:
WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

int led_state_Door = LOW;   // the current state of LED
int button_state_Door;      // the current state of button
int last_button_state_Door; // the previous state of button
int button_state_bedroom;   // the current state of button
int last_button_state_bedroom;
int switchstate = false;
// the current state of LED
// int button_state_restroom;       // the current state of button
// int last_button_state_restroom;  // the previous state of button
// settle global delay for system
unsigned long previousMillis = 0;
unsigned long previousMillis_restroom = 0;
unsigned long interval = 1000; // interval at which to blink (milliseconds)
unsigned long restroom_interval = 8000;
int led_restroom_state = 0;
float pretemp = 0;
int Flag1 = 99999;
int Flag2 = 99999;

// constants won't change :
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastTrigger = 0;
boolean startTimer = false;
boolean motion = false;
void timerIsr()
{
  static unsigned long last_time = millis();
  if (millis() - last_time > 1000)
  {
    last_time = millis();
    webSocket.sendTXT("{\"type\":\"ping\"}\n");
  }
}
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
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    USE_SERIAL.printf("[WSc] Disconnected!\n");
    break;
  case WStype_CONNECTED:
    USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

    // send message to server when Connected

    break;
  case WStype_TEXT:
  {

    DynamicJsonDocument doc(1024);

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, payload);
    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    // Fetch values.

    // print all json to test
    // serializeJsonPretty(doc, Serial);
    Serial.println();
    // get type of message
    // get id of message
    const char *id = doc["id"];
    // get device1 of message
    int device1 = doc["device1"];
    // get device2 of message
    int device2 = doc["device2"];
    // get device3 of message
    int device3 = doc["device3"];
    // get device4 of message
    int device4 = doc["device4"];
    // get device5 of message
    int device5 = doc["device5"];
    // get device6 of message
    int device6 = doc["device6"];
    // Serial.println(id);
    if (device1 == 1)
    {
      digitalWrite(LED_Door, HIGH);
     // digitalWrite(SWITCH_PIN, HIGH)
    //  switchstate = true;
    }
    else if (device1 == 0)
    {

      digitalWrite(LED_Door, LOW);
     // digitalWrite(SWITCH_PIN, LOW)
    }
    if (device2 == 1)
    {
      digitalWrite(LED_bedroom, HIGH);
    }
    else if (device2 == 0)
    {
      digitalWrite(LED_bedroom, LOW);
    }
    // send message to server

    // webSocket.sendTXT("message here");
  }

  break;
  case WStype_BIN:
    // // USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
    // // // hexdump(payload, length);

    // // // send data to server
    // // // webSocket.sendBIN(payload, length);
    break;
  case WStype_PING:
    // pong will be send automatically
    USE_SERIAL.printf("[WSc] get ping\n");
    break;
  case WStype_PONG:
    // answer to a ping we send
    USE_SERIAL.printf("[WSc] get pong\n");
    break;
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
}
// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement()
{
  digitalWrite(LED_restroom, HIGH);
  // webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device3\":1}");
  startTimer = true;
  lastTrigger = millis();
}

void setup()
{
  Serial.begin(115200); // initialize serial communication at 9600 bits per second
  Serial.print("Connecting to WiFi");
  WiFi.begin("36 lau 2", "0345616001");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  for (uint8_t t = 4; t > 0; t--)
  {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  };

  // server address, port and URL
  webSocket.begin("159.223.71.166", 8120, "/", "gqlck");

  // event handler
  webSocket.onEvent(webSocketEvent);

  // use HTTP Basic Authorization this is optional remove if not needed
  // webSocket.setAuthorization("user", "Password");

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);
  // Door setup
  pinMode(SWITCH_PIN, INPUT_PULLUP); // initialize digital pin as an input.
  pinMode(LED_Door, OUTPUT);         // set ESP32 pin to output mode
  pinMode(BUTTON_Door, INPUT_PULLUP);
  button_state_Door = digitalRead(BUTTON_Door);
  // restroom setup
  pinMode(motionSensor, INPUT_PULLUP);
  //  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING); // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  pinMode(LED_restroom, OUTPUT); // set ESP32 pin to output mode
  digitalWrite(LED_restroom, LOW);
  // bed room setup
  dht.begin();
  pinMode(BUTTON_bedroom, INPUT_PULLUP);
  pinMode(LED_bedroom, OUTPUT);
  // timer setup
}
void loop()
{
  // read the state of the reed switch
  float h = dht.readHumidity();
  float T = dht.readTemperature();
  int switchState = digitalRead(SWITCH_PIN);
  delay(10);
  last_button_state_Door = button_state_Door;
  button_state_Door = digitalRead(BUTTON_Door);
  last_button_state_bedroom = button_state_bedroom;
  button_state_bedroom = digitalRead(BUTTON_bedroom);
  unsigned long now = millis();

  // Door scenario
  if (pretemp == 0)
  {
    pretemp = T;
    webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device4\": " + String(T) + "}");
  }
  else if (pretemp != T && pretemp != 0)
  {
    pretemp = T;
    webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device4\": " + String(T) + "}");
  }
  if (Flag1 == 99999)
  {
    Flag1 = switchState;
    webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device5\": " + String(switchState) + "}");
  }
  else if (Flag1 != switchState && Flag1 != 99999)
  {
    Flag1 = switchState;
    webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device5\": " + String(switchState) + "}");
  }
  if (Flag2 == 99999)
  {
    Flag2 = digitalRead(motionSensor);
    webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device6\": " + String(digitalRead(motionSensor)) + "}");
  }
  else if (Flag2 != digitalRead(motionSensor) && Flag2 != 99999)
  {
    Flag2 = digitalRead(motionSensor);
    if (digitalRead(motionSensor) == 1)
    {
      digitalWrite(LED_restroom, HIGH); // turn on LED

      webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device3\":1}");
    }
    else
    {
   digitalWrite(LED_restroom, LOW); // turn on LED

      webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device3\":0}");
    }
    webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device6\": " + String(digitalRead(motionSensor)) + "}");
  }

  // print out the state of motionSensor:
  printf("motionSensor = %d\n", digitalRead(motionSensor));

  //  if (switchState == HIGH || button_state_Door == LOW) { // if door is open or button is pressed
  //
  //    digitalWrite(LED_Door, HIGH); // turn on LED
  //
  //  } else if((switchState == HIGH || button_state_Door == HIGH)) {
  //
  //    digitalWrite(LED_Door, LOW); // turn on LED

  //
  //  }
  //  ///End Door scenario
  //
  //  ///Bedroom scenario
  //  if ( (button_state_bedroom == LOW) || (T >= 30)) { // if door is open or button is pressed
  //    digitalWrite(LED_bedroom, HIGH); // turn on LED
  //
  //
  //  } else {
  //    digitalWrite(LED_bedroom, LOW); // turn on LED
  //
  //  }
  // if (isnan(h) || isnan(T))
  // {
  //   Serial.println("Failed to read from DHT sensor!");
  //   return;
  // }

  // if ((digitalRead(LED_restroom) == HIGH) && (motion == false))
  // {
  //   Serial.println("MOTION DETECTED!!!");
  //   motion = true;
  // }
  // // Turn off the LED after the number of seconds defined in the timeSeconds variable
  // if (startTimer && (now - lastTrigger > (timeSeconds * 1000)))
  // {
  //   Serial.println("Motion stopped...");
  //   digitalWrite(LED_restroom, LOW);
  //   webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"gqlck\",\"device3\":0}");
  //   startTimer = false;
  //   motion = false;
  // }
  //    webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"s55po\",\"device5\": " + String(switchState)+ "}");
  //    webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"s55po\",\"device6\": " + String(digitalRead(motionSensor))+ "}");
  timerIsr(); //  ping server every 1000 milliseconds
  // webSocket.sendTXT("{\"type\":\"ping\"}\n");
  webSocket.loop();
}
