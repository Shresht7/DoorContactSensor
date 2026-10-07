#include <Arduino.h>

#define BAUD_RATE 115200

#define MAG_REED_SENSOR 4
#define LED_BUILTIN 8

void setup()
{
    Serial.begin(BAUD_RATE);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(MAG_REED_SENSOR, INPUT);
}

void loop()
{
    int magPresence = digitalRead(MAG_REED_SENSOR);
    if (magPresence)
    {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
    Serial.println(magPresence);
    delay(50);
}
