#include <Arduino.h>

#define BAUD_RATE 115200

#define LED_BUILTIN 8

void setup()
{
    Serial.begin(BAUD_RATE);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    Serial.println("Hello World\n");

    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
}
