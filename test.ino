/*
 * WebSocketClient.ino
 *
 *  Created on: 12.10.2023
 *
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>
#define LED_awning 17
#define LED_kitchen 19
#define LED_Fan 16
#define BUTTON_kitchen 13
#define BUTTON_awning 18
#define gasSensor 34
#define lightSensor 32
#define motionSensor 27
unsigned long interval = 1000;
unsigned long interval1 = 5000;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
int led_state1 = 0;

WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
#define USE_SERIAL Serial
JsonDocument doc;
// call back send ping to server
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
		// convert uint8_t to string
		
		break;
	case WStype_TEXT:{
		// USE_SERIAL.printf("[WSc] get text: %s\n", payload);

		String json = String((char *)payload);
		// send message to server when Connected
		DeserializationError error = deserializeJson(doc, json);

		// Test if parsing succeeds.
		if (error)
		{
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}
		// Fetch values.
		const char *type = doc["type"];
		const char *id = doc["id"];
		const char *device1 = doc["device1"];
		const char *device2 = doc["device2"];
		const char *device3 = doc["device3"];
		const char *device4 = doc["device4"];
		const char *device5 = doc["device5"];
		const char *device6 = doc["device6"];
		USE_SERIAL.printf("type: %s\n", type);
		USE_SERIAL.printf("id: %s\n", id);
		USE_SERIAL.printf("device1: %s\n", device1);
		USE_SERIAL.printf("device2: %s\n", device2);
		USE_SERIAL.printf("device3: %s\n", device3);
		USE_SERIAL.printf("device4: %s\n", device4);
		USE_SERIAL.printf("device5: %s\n", device5);
		USE_SERIAL.printf("device6: %s\n", device6);
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

void setup()
{
	// USE_SERIAL.begin(921600);
	USE_SERIAL.begin(115200); // Start serial communication
	USE_SERIAL.print("Connecting to WiFi");
	WiFi.begin("36 lau 2", "0345616001");
	while (WiFi.status() != WL_CONNECTED)
	{
		USE_SERIAL.print(".");
	}
	USE_SERIAL.println(" Connected!");

	pinMode(LED_Fan, OUTPUT);

	for (uint8_t t = 4; t > 0; t--)
	{
		USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
		USE_SERIAL.flush();
		delay(1000);
	};

	// server address, port and URL
	webSocket.begin("159.223.71.166", 8120, "/", "x5nr9");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	// webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
}

void loop()
{
	int sensorValue = analogRead(gasSensor);
	float voltage = (float)sensorValue / 1024 * 5.0;

	int gasValue = map(voltage, 0, 5, 0, 30);

	Serial.print("gasValue: ");
	Serial.println(gasValue); // Print the voltage value
	Serial.print("voltage: ");
	Serial.println(voltage);
	Serial.print("sensorValue: ");
	Serial.println(sensorValue);

	if (gasValue >= 100)
	{
		digitalWrite(LED_Fan, HIGH); // Turn on the LED if gas concentration is high
		led_state1 = ~led_state1;
		Serial.println("Gas detected!");
		webSocket.sendTXT("{\"type\":\"message\",\"id\":\"x5nr9\",\"device1\":1}");
	}
	else
	{
		digitalWrite(LED_Fan, LOW);
		webSocket.sendTXT("{\"type\":\"message\",\"id\":\"x5nr9\",\"device1\":0}");
		//	 // Turn off the LED if gas concentration is low
	}
	timerIsr();
	// ping server every 1000 milliseconds
	//	webSocket.sendTXT("{\"type\":\"ping\"}\n");

	webSocket.loop();
}