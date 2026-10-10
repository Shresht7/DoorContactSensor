#include <Arduino.h>

#include "indicator.h"
#include "config.h"

static bool lit = false;
static unsigned long offAt = 0; // millis() timestamp when the current pulse ends

static void setLed(bool on)
{
    // Maps "on/off" to the pin level for this board's polarity
    digitalWrite(PIN_LED, (on == LED_ACTIVE_HIGH) ? HIGH : LOW);
    lit = on;
}

void indicatorBegin()
{
    pinMode(PIN_LED, OUTPUT);
    setLed(false);
}

void indicatorPulse(unsigned int durationMs)
{
    if (durationMs == 0)
        return;
    setLed(true);
    offAt = millis() + durationMs;
}

void indicatorUpdate()
{
    if (lit && (long)(millis() - offAt) >= 0)
        setLed(false);
}

void indicatorOff()
{
    setLed(false);
}
