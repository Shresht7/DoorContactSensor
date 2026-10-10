// LIBRARY
// -------

#include <Arduino.h>
#include "esp_sleep.h"

#include "connection.h"
#include "helpers.h"
#include "config.h"

// STATE
// -----

// Variables marked RTC_DATA_ATTR survive deep sleep (but reset on power cycle)
RTC_DATA_ATTR uint32_t wakeCount = 0;
RTC_DATA_ATTR int lastKnownState = -1; // -1 marks no previous state yet

// Counts consecutive failures; reset when a publish succeeds.
RTC_DATA_ATTR uint8_t failedPublishes = 0;

bool lastDoorState;
int lastPublishedState = -1;
unsigned long lastActivityTime = 0;

/// @brief Publishes the current state of the door to the MQTT broker
/// @param doorOpened True if the door is opened, false if closed
/// @return True if the state was successfully published, false otherwise
bool publishState(bool doorOpened)
{
    // Construct the JSON payload
    String payload = "{\"state\":\"" + String(doorOpened ? "OPEN" : "CLOSED") + "\",\"runtime\":" + String(millis()) + "}";

    // Attempt to publish the state to the MQTT broker
    bool ok = publish(MQTT_TOPIC, payload.c_str(), true);
    if (ok)
    {
        lastPublishedState = doorOpened;
        failedPublishes = 0;
    }

    Serial.println("[NET]: State published: " + String(doorOpened ? "OPEN" : "CLOSED") + " (success: " + String(ok) + ")");
    return ok;
}

// FUNCTIONS
// ---------

/// @brief Reports the current state of the door (opened or closed)
/// @param doorOpened True if the door is opened (no magnet), false if closed (magnet)
/// @return True if the state was successfully reported, false otherwise
bool reportState(bool doorOpened)
{
    Serial.println(doorOpened ? "[DOOR]: OPEN" : "[DOOR]: CLOSED");
    return publishState(doorOpened);
}

/// @brief Puts the device into deep sleep, waiting for the door state to change
/// @param currentlyOpen True if the door is currently open, false if closed
void goToSleep(bool currentlyOpen)
{
#ifdef STAY_AWAKE
    Serial.println("[DEV]: STAYING AWAKE!");
    return;
#else
    // If the current state never reached the broker, also wake on a timer
    // to retry. The delay grows with each consecutive failure.
    if (lastPublishedState != (int)currentlyOpen)
    {
        uint64_t retryUs = exponentialBackoff(RETRY_BASE_US, failedPublishes);
        if (failedPublishes < UINT8_MAX)
            failedPublishes++;
        esp_sleep_enable_timer_wakeup(retryUs);
        Serial.printf("[DEEPSLEEP]: State not published, retrying in %llu s\n",
                      (unsigned long long)(retryUs / 1000000ULL));
    }

    Serial.println("[DEEPSLEEP]: Going to sleep...");
    Serial.flush();

    // Disconnect from the network before going to deep sleep
    disconnectNetwork();

    // Wake when the pin goes to the opposite state of the current door state,
    // so any change (open->close or close->open) trigger an external wake.
    esp_deepsleep_gpio_wake_up_mode_t wakeMode = currentlyOpen
                                                     ? ESP_GPIO_WAKEUP_GPIO_LOW
                                                     : ESP_GPIO_WAKEUP_GPIO_HIGH;
    esp_deep_sleep_enable_gpio_wakeup(BIT(PIN_MAG_REED_SENSOR), wakeMode);
    esp_deep_sleep_start();
#endif
}

// -----
// SETUP
// -----

/// @brief Runs at startup (and on every wake from deep sleep)
void setup()
{
    // Initialize the LED pin immediately
    pinMode(PIN_LED, OUTPUT);

#ifdef DEBUG
    // Blink the LED a few times to indicate debug mode is active
    for (int i = 0; i < BLINK_COUNT; i++)
    {
        digitalWrite(PIN_LED, HIGH);
        delay(BLINK_MS);
        digitalWrite(PIN_LED, LOW);
        delay(BLINK_MS);
    }
#endif

    // Initialize serial communication
    Serial.begin(BAUD_RATE);

#if defined(DEBUG) && !defined(STAY_AWAKE)
    // give the host time to reattach to the USB port
    delay(USB_REATTACH_GRACE_MS);
#endif

    // Configure the pins for the LED and magnetic reed sensor
    pinMode(PIN_MAG_REED_SENSOR, INPUT_PULLUP);

    Serial.println("[BOOT]: Starting up...");

    // Short delay to allow the sensor to stabilize
    delay(MAG_REED_STABILIZATION_DELAY_MS);

    // Read the current state of the door sensor (1 = open / no magnet, 0 = closed / magnet)
    bool doorOpened = digitalRead(PIN_MAG_REED_SENSOR);

    lastDoorState = doorOpened;

    // Determine the wakeup cause
    esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();

    // Log Wake events
    wakeCount++;
#ifdef DEBUG
    Serial.printf("[WAKE]: #%lu, reason %d, door %d (previous %d)\n",
                  (unsigned long)wakeCount, (int)wakeReason, (int)doorOpened, lastKnownState);
    if (lastKnownState == (int)doorOpened)
    {
        Serial.println("[WAKE]: !! Wake with unchanged state (bounce or floating pin)");
    }
#endif
    lastKnownState = (int)doorOpened;

    bool ok;
    if (wakeReason == ESP_SLEEP_WAKEUP_GPIO)
    {
        // Woke up from external wakeup (door sensor)
        Serial.println("[WAKE]: Woke up from external wakeup (door sensor)");
        ok = reportState(doorOpened); // Report current state of door
    }
    else if (wakeReason == ESP_SLEEP_WAKEUP_TIMER)
    {
        // Woke up from timer (retry)
        Serial.println("[WAKE]: Woke up from timer (retry)");
        ok = reportState(doorOpened); // Report current state of door
    }
    else
    {
        // Woke up from other reasons (e.g., power-on reset)
        Serial.println("[WAKE]: First Boot!");
        ok = reportState(doorOpened); // Report initial state
    }

    // The publish above counts as activity: start the quiet window from now,
    // not from boot, so the device waits STEADY_MS for the door to settle
    lastActivityTime = millis();

    // If reporting the state failed, go to sleep to retry later
    if (!ok)
        goToSleep(doorOpened);
}

// ----
// LOOP
// ----

/// @brief Runs indefinitely after setup() has completed
void loop()
{
    // Read the current state of the door sensor
    bool doorOpened = digitalRead(PIN_MAG_REED_SENSOR);
    if (doorOpened != lastDoorState)
    {
        lastDoorState = doorOpened;
        lastActivityTime = millis();
    }

    // Check if the door has been stable for the hold time/threshold before publishing the state
    if ((millis() - lastActivityTime) > DOOR_STABILITY_TIME_MS && doorOpened != lastPublishedState)
        if (!publishState(doorOpened))
            goToSleep(doorOpened); // Go to sleep if publishing fails

    // Allow the network client to process any pending messages
    networkLoop();

    // Check if the door has been in a steady state for long enough to go to sleep
    if ((millis() - lastActivityTime) > DOOR_SETTLE_TIME_MS && doorOpened == lastPublishedState)
        goToSleep(doorOpened);

    // Force deep sleep if the device has been awake for too long
    if (millis() > MAX_AWAKE_TIME_MS)
        goToSleep(doorOpened);

    // Throttle the loop to avoid busy-waiting
    delay(LOOP_DELAY_MS);
}
