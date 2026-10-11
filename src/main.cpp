// LIBRARY
// -------

#include <Arduino.h>
#include "esp_sleep.h"
#include "string.h"

#include "connection.h"
#include "payload.h"
#include "deepsleep.h"
#include "identity.h"
#include "discovery.h"
#include "mqtt-topics.h"
#include "ota.h"
#include "indicator.h"
#include "helpers.h"
#include "config.h"

// STATE
// -----

// Variables marked RTC_DATA_ATTR survive deep sleep (but reset on power cycle)
RTC_DATA_ATTR uint32_t wakeCount = 0;
RTC_DATA_ATTR int lastKnownState = -1; // -1 marks no previous state yet

// Counts consecutive failures; reset when a publish succeeds.
RTC_DATA_ATTR uint8_t failedPublishes = 0;

// Stay awake instead of deep sleeping. Preseeded from env default;
// survives deep sleep; resets on power cycle; toggleable via MQTT
RTC_DATA_ATTR bool stayAwake = STAY_AWAKE_INITIAL;

// Set when stayAwake changed this wake cycle, so loop() can publish the switch state
static bool stayAwakeDirty = false;

bool lastDoorState;
int lastPublishedState = -1;
unsigned long lastActivityTime = 0;

/// @brief Indicates whether the OTA (Over-the-Air) update service has been started
static bool otaStarted = false;

// Why the device woke up this cycle
static WakeReason wakeReason = WakeReason::WAKE_REASON_BOOT;

/// @brief Publishes the current state of the door to the MQTT broker
/// @param doorOpened True if the door is opened, false if closed
/// @return True if the state was successfully published, false otherwise
bool publishState(bool doorOpened)
{
    // Ensure the network is connected before attempting to publish
    if (!connectNetwork())
    {
        Serial.println("[NET]: Connection failed, skipping publish");
        return false;
    }

    // Prepare the status report structure with the current state and metadata
    StatusReport report = {
        doorClosed : !doorOpened,
        rssi : getRSSI(),
        wakeReason : WakeReasonToString(wakeReason),
        wakeCount : wakeCount,
        failedPublishes : failedPublishes,
        awakeMs : (uint32_t)millis(),
        wifiMs : getLastWiFiConnectMs(),
        mqttMs : getLastMQTTConnectMs(),
        reportMs : (uint32_t)millis(),
        firmwareVersion : FIRMWARE_VERSION
    };

    // Construct the JSON payload
    char payload[PAYLOAD_BUFFER_SIZE];
    if (buildStatusReportPayload(payload, sizeof(payload), report) == 0)
    {
        Serial.println("[NET]: Payload too large for buffer, not publishing");
        return false;
    }

    // Attempt to publish the state to the MQTT broker
    bool ok = publish(mqttTopicState(), payload, true);
    if (ok)
    {
        lastPublishedState = doorOpened;
        failedPublishes = 0;
        indicatorPulse(BLINK_MS);
    }

    Serial.printf("[NET]: Published %s (success: %d)\n", doorOpened ? "OPEN" : "CLOSED", ok);
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
    // Return early if the device is set to stay awake or an OTA update is in progress
    if (stayAwake || otaInProgress())
        return;

    // Turn off the indicator LED before going to deep sleep
    indicatorOff();

    // Always wake periodically to publish heartbeat
    uint64_t timerWakeUs = HEARTBEAT_INTERVAL_US;

    // If the current state never reached the broker, also wake on a timer
    // to retry. The delay grows with each consecutive failure.
    if (lastPublishedState != (int)currentlyOpen)
    {
        uint64_t retryUs = exponentialBackoff(RETRY_BASE_US, failedPublishes);
        if (failedPublishes < UINT8_MAX)
            failedPublishes++;
        if (retryUs < timerWakeUs)
            timerWakeUs = retryUs;
        Serial.printf("[DEEPSLEEP]: State not published, retrying in %llu s\n",
                      (unsigned long long)(timerWakeUs / 1000000ULL));
    }

    // Disconnect from the network before going to deep sleep
    disconnectNetwork();

    // Setup wake triggers and enter deep sleep
    enterDeepSleep(currentlyOpen, timerWakeUs);
}

// DISCOVERY
// ---------

/// @brief Announces the sensor to Home Assistant.

static void publishDiscovery()
{
    char topic[96];
    char payload[PAYLOAD_BUFFER_SIZE];

    DiscoveryInfo info = {
        deviceId(),
        DEVICE_NAME,
        mqttTopicState(),
        FIRMWARE_VERSION,
        DOOR_EXPIRE_AFTER_S,
        mqttTopicCmd(),
        mqttTopicStayAwake()};

    if (buildDiscoveryTopic(topic, sizeof(topic), DISCOVERY_PREFIX, deviceId()) == 0)
    {
        Serial.println("[DISCOVERY]: Failed to construct Discovery Topic");
        return;
    }

    if (buildDiscoveryPayload(payload, sizeof(payload), info) == 0)
    {
        Serial.println("[DISCOVERY]: Failed to construct Discovery Payload");
        return;
    }

    bool retained = true;
    bool ok = publish(topic, payload, retained);
    Serial.printf("[DISCOVERY]: Discovery Published (success: %d)\n", ok);
}

static void onMqttMessage(const char *topic, const uint8_t *payload, unsigned int length)
{
    if (strcmp(topic, mqttTopicCmd()) != 0)
        return;
    char value[4] = {0};
    size_t n = (length < sizeof(value) - 1) ? length : sizeof(value) - 1;
    memcpy(value, payload, n);

    bool newState = (strncmp(value, "ON", 2) == 0 || strncmp(value, "1", 1) == 0);
    if (newState != stayAwake)
    {
        stayAwake = newState;
        stayAwakeDirty = true;
        indicatorPulse(BLINK_MS);
    }
    lastActivityTime = millis(); // Update the last activity time whenever a command is received

    Serial.printf("[CMD]: Stay Awake -> %s\n", newState ? "ON" : "OFF");
}

// -----
// SETUP
// -----

/// @brief Runs at startup (and on every wake from deep sleep)
void setup()
{
    // Initialize the LED pin immediately
    indicatorBegin();

    // Initialize serial communication
    Serial.begin(BAUD_RATE);
    Serial.println("[BOOT]: Starting up...");

    // Initialize the device identity module
    identityBegin();
    Serial.printf("[BOOT]: Device ID: %s\n", deviceId());

    // Initialize the MQTT topics
    mqttTopicsInit();

    // Register the MQTT message callback
    setMqttMessageCallback(onMqttMessage);

#ifdef WAIT_FOR_USB
    // Give the host time to reattach to the USB port.
    delay(USB_REATTACH_GRACE_MS);
#endif

    // Configure the pins for the LED and magnetic reed sensor
    pinMode(PIN_MAG_REED_SENSOR, INPUT_PULLUP);

    // Read the current state of the door sensor (1 = open / no magnet, 0 = closed / magnet)
    bool doorOpened = digitalRead(PIN_MAG_REED_SENSOR);

    lastDoorState = doorOpened;

    // Determine the wakeup cause
    esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();

    // Log Wake events
    wakeReason = wakeReasonFromCause(wakeCause);
    wakeCount++;
#ifdef DEBUG
    Serial.printf("[WAKE]: #%lu, reason %d, door %d (previous %d)\n",
                  (unsigned long)wakeCount, (int)wakeCause, (int)doorOpened, lastKnownState);
    if (lastKnownState == (int)doorOpened)
    {
        Serial.println("[WAKE]: !! Wake with unchanged state (bounce or floating pin)");
    }
#endif
    lastKnownState = (int)doorOpened;

    bool ok;
    if (wakeCause == ESP_SLEEP_WAKEUP_GPIO)
    {
        // Woke up from external wakeup (door sensor)
        Serial.println("[WAKE]: Woke up from external wakeup (door sensor)");
        ok = reportState(doorOpened); // Report current state of door
        networkLoop();                // Process any retained command delivered on connect
        if (stayAwake)
        {
            stayAwakeDirty = true;       // Mark the stay-awake state as dirty to trigger publishing
            lastActivityTime = millis(); // Update the last activity time to now
        }
        if (stayAwakeDirty)
        {
            stayAwakeDirty = false;                                        // Clear the dirty flag after handling it
            publish(mqttTopicStayAwake(), stayAwake ? "ON" : "OFF", true); // Publish the updated stay-awake state
        }
    }
    else if (wakeCause == ESP_SLEEP_WAKEUP_TIMER)
    {
        // Woke up from timer (retry)
        Serial.println("[WAKE]: Woke up from timer (retry or heartbeat)");
        bool online = connectNetwork();
        if (online)
        {
            publishDiscovery();
            publish(mqttTopicStayAwake(), stayAwake ? "ON" : "OFF", true);
        }
        ok = online && reportState(doorOpened); // Report current state of door
    }
    else
    {
        // Woke up from other reasons (e.g., power-on reset)
        Serial.println("[WAKE]: First Boot!");
        bool online = connectNetwork();
        if (online)
        {
            publishDiscovery();
            publish(mqttTopicStayAwake(), stayAwake ? "ON" : "OFF", true);
        }
        ok = online && reportState(doorOpened); // Report current state of door
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
    // Update the indicator LED based on its current pulse state
    indicatorUpdate();

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

    // If the stay-awake state has changed, publish the new state
    if (stayAwakeDirty)
    {
        stayAwakeDirty = false;
        publish(mqttTopicStayAwake(), stayAwake ? "ON" : "OFF", true);
    }

    // Serve OTA requests while awake and connected
    if (isWiFiConnected())
    {
        if (!otaStarted)
        {
            otaBegin();
            otaStarted = true;
        }
        otaUpdate();
    }
    else
    {
        otaStarted = false;
    }

    // Check if the device should stop staying awake due to idle timeout
    if (stayAwake && !otaInProgress() && (millis() - lastActivityTime) > STAY_AWAKE_IDLE_TIMEOUT_MS)
    {
        stayAwake = false;                 // Stop staying awake due to idle timeout
        stayAwakeDirty = true;             // Mark the stay-awake state as dirty to trigger publishing
        publish(mqttTopicCmd(), "", true); // Clear the retained command in idle auto-off so it can't rearm automatically
    }

    // Check if the door has been in a steady state for long enough to go to sleep
    if (!stayAwake && (millis() - lastActivityTime) > DOOR_SETTLE_TIME_MS && doorOpened == lastPublishedState)
        goToSleep(doorOpened);

    // Force deep sleep if the device has been awake for too long
    if (!stayAwake && millis() > MAX_AWAKE_TIME_MS)
        goToSleep(doorOpened);

    // Throttle the loop to avoid busy-waiting
    delay(LOOP_DELAY_MS);
}
