// LIBRARY
// -------

#include <Arduino.h>
#include "esp_sleep.h"

// DEFINITIONS
// -----------

#define BAUD_RATE 115200

#define MAG_REED_SENSOR 4
#define LED_BUILTIN 8

/// @brief Reports the current state of the door (opened or closed)
/// @param doorOpened True if the door is opened, false if closed
void reportState(bool doorOpened)
{
    Serial.println("Door state: " + String(doorOpened));
    if (doorOpened)
    {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
}

/// @brief Puts the device into deep sleep, waiting for the door state to change
/// @param currentlyOpen True if the door is currently open, false if closed
void goToSleep(bool currentlyOpen)
{
    Serial.println("Going to sleep...");
    // Wake when the pin goes to the opposite state of the current door state,
    // so any change (open->close or close->open) trigger an external wake.
    esp_deepsleep_gpio_wake_up_mode_t wakeMode = currentlyOpen
                                                     ? ESP_GPIO_WAKEUP_GPIO_LOW
                                                     : ESP_GPIO_WAKEUP_GPIO_HIGH;
    esp_deep_sleep_enable_gpio_wakeup(BIT(MAG_REED_SENSOR), wakeMode);
    esp_deep_sleep_start();
}

// -----
// SETUP
// -----

/// @brief Runs at startup
void setup()
{
    // Initialize serial communication
    Serial.begin(BAUD_RATE);

    // Configure the pins for the LED and magnetic reed sensor
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(MAG_REED_SENSOR, INPUT_PULLUP);

    Serial.println("Starting up...");

    // Read the current state of the door sensor
    bool doorOpened = digitalRead(MAG_REED_SENSOR);

    // Determine the wakeup cause
    esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
    Serial.println("Wake reason: " + String(wakeReason));
    if (wakeReason == ESP_SLEEP_WAKEUP_GPIO)
    {
        // Woke up from external wakeup (door sensor)
        Serial.println("Woke up from external wakeup (door sensor)");
        reportState(doorOpened); // Report current state of door
    }
    else
    {
        // Woke up from other reasons (e.g., power-on reset)
        Serial.println("First Boot. Halting for firmware uploads...");
#ifdef DEBUG
        delay(5000); // Stay powered on for a while to make firmware uploads easier
#endif
        reportState(doorOpened); // Report initial state
    }

    delay(1000); // Wait a moment before going to sleep

    // Go to deep sleep, waiting for the door state to change...
    goToSleep(doorOpened);
}

// ----
// LOOP
// ----

/// @brief Runs indefinitely after setup() has completed
void loop()
{
    // Nothing to do here, as the device goes to deep sleep at the end of setup()
}
