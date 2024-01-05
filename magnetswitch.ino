#include "DHT.h"
#include <Arduino.h>
#include <string>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>
#define SWITCH_PIN 22 // ESP32 pin connected to the reed switch
#define LED_Door 17
#define LED_restroom 19
#define LED_bedroom 16
#define BUTTON_bedroom 13
#define BUTTON_Door 18
#define DHTPIN 4
#define DHTTYPE DHT22
#define motionSensor 27
hw_timer_t *timer = NULL;
volatile bool ledState = false;
WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

// variables will change:
int led_state_Door = LOW;   // the current state of LED
int button_state_Door;      // the current state of button
int last_button_state_Door; // the previous state of button
int button_state_bedroom;   // the current state of button
int last_button_state_bedroom;
// the current state of LED
// int button_state_restroom;       // the current state of button
// int last_button_state_restroom;  // the previous state of button
// settle global delay for system
unsigned long previousMillis = 0;

const long interval = 1000; // interval at which to blink (milliseconds)

// constants won't change :
DHT dht(DHTPIN, DHTTYPE);

// Checks if motion was detected, sets LED HIGH and starts a timer

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
    USE_SERIAL.printf("[WSc] get text: %s\n", payload);

    // send message to server
    // webSocket.sendTXT("message here");
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
void IRAM_ATTR onTimer()
{
  // This function will be called when the timer interrupt is triggered.
  // Place your code here.
  timerAlarmEnable(timer);
  digitalWrite(LED_restroom, LOW); // turn off LED
}
void setup()
{
  pinMode(SWITCH_PIN, INPUT_PULLUP); // initialize digital pin as an input.
  Serial.begin(115200);              // initialize serial communication at 9600 bits per second
  // Door setup
  pinMode(LED_Door, OUTPUT); // set ESP32 pin to output mode
  pinMode(BUTTON_Door, INPUT_PULLUP);
  button_state_Door = digitalRead(BUTTON_Door);
  // restroom setup
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  pinMode(LED_restroom, OUTPUT); // set ESP32 pin to output mode
  digitalWrite(LED_restroom, LOW);
  // bed room setup
  dht.begin();
  pinMode(BUTTON_bedroom, INPUT_PULLUP);
  pinMode(LED_bedroom, OUTPUT);
  // timer setup
  timer = timerBegin(0, 80, true);             // Use the first timer, with a prescaler of 80.
  timerAttachInterrupt(timer, &onTimer, true); // Attach the interrupt function.
  timerAlarmWrite(timer, 10000000, true);      // Set the alarm value to 1 second (1,000,000 microseconds).
  timerAlarmEnable(timer);
  pinMode(LED_Fan, OUTPUT);
  Serial.print("Connecting to WiFi");
  WiFi.begin("Thuan", "thuan0023");
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
  webSocket.begin("159.223.71.166", 8120, "/", "6rq7g");

  // event handler
  webSocket.onEvent(webSocketEvent);

  // use HTTP Basic Authorization this is optional remove if not needed
  // webSocket.setAuthorization("user", "Password");

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);
}
void loop()
{
  // read the state of the reed switch
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int switchState = digitalRead(SWITCH_PIN);
  delay(10);
  last_button_state_Door = button_state_Door;
  button_state_Door = digitalRead(BUTTON_Door);
  last_button_state_bedroom = button_state_bedroom;
  button_state_bedroom = digitalRead(BUTTON_bedroom);
  webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device4\": " + String(switchState) "}");
  webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device5\": " + String(t) "}");
  webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device6\": " + String(digitalRead(motionSensor)) "}");

  unsigned long now = millis();
  if (now - previousMillis >= interval)
  {
    previousMillis = now;
    // Door scenario
    if (last_button_state_Door == HIGH && button_state_Door == LOW)
    {
      Serial.println("The button is pressed");
    }
    if (switchState == HIGH || button_state_Door == LOW)
    { // if door is open or button is pressed
      Serial.println("The door is open or button is pressed, turns on LED");
      webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device1\":  1}");

      digitalWrite(LED_Door, HIGH); // turn on LED
    }
    else
    {
      Serial.println("The door is closed and button is not pressed, turns off LED");
      webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device1\":  0}");

      digitalWrite(LED_Door, LOW); // turn on LED
    }
    /// End Door scenario
    /// restroom scenario
    if (digitalRead(motionSensor) == HIGH)
    {
      Serial.println("move!");
      digitalWrite(LED_restroom, HIGH);
      webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device2\":  1}");
    }

    if (digitalRead(motionSensor) == LOW)
    {
      Serial.println("dont move!");
       digitalWrite(LED_restroom, LOW);
      webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device2\":  0}");
      onTimer();
    }
    /// End restroom scenario
    /// Bedroom scenario
    if (button_state_bedroom == LOW)
    {                                  // if door is open or button is pressed
      digitalWrite(LED_bedroom, HIGH); // turn on LED
      webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device3\":  1}");
      
    }
    else
    {
      digitalWrite(LED_bedroom, LOW); // turn on LED
      webSocket.sendTXT("{ \"type\": \"message\",\"id\": \"6rq7g\",\"device3\":  0}");

    }
    if (isnan(h) || isnan(t))
    {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C");

    /// End Bedroom scenario
  }
}
