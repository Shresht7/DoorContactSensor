// LIBRARY
// -------

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_sleep.h"

#include "secrets.h"

// DEFINITIONS
// -----------

#define BAUD_RATE 115200

#define MAG_REED_SENSOR 4
#define LED 8

#define FIRMWARE_UPDATE_GRACE_PERIOD 10000

// WiFi AND MQTT CONFIGURATION
// ---------------------------

#define MQTT_TOPIC "door-sensor/state"
#define MQTT_CLIENT_ID "door-sensor-client-01"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

#ifdef STAY_AWAKE
bool connectedToNetwork = false;
#endif

/// @brief Connects to the WiFi network and MQTT broker
/// @return True if both WiFi and MQTT connections are successful, false otherwise
bool connectNetwork()
{

    // Return early if already connected to the MQTT broker
    if (mqttClient.connected())
        return true;

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000)
    {
        delay(100);
    }
    if (WiFi.status() != WL_CONNECTED)
        return false;

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setSocketTimeout(3);
    start = millis();
    while (!mqttClient.connected() && (millis() - start) < 10000)
    {
        mqttClient.connect(MQTT_CLIENT_ID);
        delay(100);
    }

#ifdef STAY_AWAKE
    connectedToNetwork = mqttClient.connected();
#endif

    return mqttClient.connected();
}

/// @brief Publishes the current state of the door to the MQTT broker
/// @param doorOpened True if the door is opened, false if closed
void publishState(bool doorOpened)
{
    if (!connectNetwork())
    {
        Serial.println("[NET]: Connection Failed, skipping publish");
        return;
    }
    bool ok = mqttClient.publish(MQTT_TOPIC, doorOpened ? "OPEN" : "CLOSED", true);
    mqttClient.loop(); // Ensure the MQTT client processes the publish request
    delay(100);        // Give the MQTT client time to send the message
    Serial.println("[NET]: State published: " + String(doorOpened ? "OPEN" : "CLOSED") + " (success: " + String(ok) + ")");
}

// STATE
// -----

// Variables marked RTC_DATA_ATTR survive deep sleep (but reset on power cycle)
RTC_DATA_ATTR uint32_t wakeCount = 0;
RTC_DATA_ATTR int lastKnownState = -1; // -1 marks no previous state yet

#ifdef STAY_AWAKE
// Last known state of the door in stay awake mode
bool lastDoorState;
#endif

// FUNCTIONS
// ---------

/// @brief Reports the current state of the door (opened or closed)
/// @param doorOpened True if the door is opened (no magnet), false if closed (magnet)
void reportState(bool doorOpened)
{
    Serial.println(doorOpened ? "[DOOR]: OPEN" : "[DOOR]: CLOSED");
    publishState(doorOpened);
}

/// @brief Puts the device into deep sleep, waiting for the door state to change
/// @param currentlyOpen True if the door is currently open, false if closed
void goToSleep(bool currentlyOpen)
{
#ifdef STAY_AWAKE
    Serial.println("[DEV]: STAYING AWAKE!");
    return;
#else
    Serial.println("[DEEPSLEEP]: Going to sleep...");
    Serial.flush();

    // Disconnect from the network before going to deep sleep
    if (mqttClient.connected())
        mqttClient.disconnect();
    if (WiFi.isConnected())
        WiFi.disconnect();
    connectedToNetwork = false;

    // Wake when the pin goes to the opposite state of the current door state,
    // so any change (open->close or close->open) trigger an external wake.
    esp_deepsleep_gpio_wake_up_mode_t wakeMode = currentlyOpen
                                                     ? ESP_GPIO_WAKEUP_GPIO_LOW
                                                     : ESP_GPIO_WAKEUP_GPIO_HIGH;
    esp_deep_sleep_enable_gpio_wakeup(BIT(MAG_REED_SENSOR), wakeMode);
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
    pinMode(LED, OUTPUT);

#ifdef DEBUG
    // Blink the LED a few times to indicate debug mode is active
    for (int i = 0; i < 5; i++)
    {
        digitalWrite(LED, HIGH);
        delay(500);
        digitalWrite(LED, LOW);
        delay(500);
    }
#endif

    // Initialize serial communication
    Serial.begin(BAUD_RATE);

#if defined(DEBUG) && !defined(STAY_AWAKE)
    delay(2000); // give the host time to reattach to the USB port
#endif

    // Configure the pins for the LED and magnetic reed sensor
    pinMode(MAG_REED_SENSOR, INPUT_PULLUP);

    Serial.println("[BOOT]: Starting up...");

    delay(50); // Short delay to allow the sensor to stabilize

    // Read the current state of the door sensor (1 = open / no magnet, 0 = closed / magnet)
    bool doorOpened = digitalRead(MAG_REED_SENSOR);

#ifdef STAY_AWAKE
    // In stay awake mode, remember the last door state
    lastDoorState = doorOpened;
#endif

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

    if (wakeReason == ESP_SLEEP_WAKEUP_GPIO)
    {
        // Woke up from external wakeup (door sensor)
        Serial.println("[WAKE]: Woke up from external wakeup (door sensor)");
        reportState(doorOpened); // Report current state of door
    }
    else
    {
        // Woke up from other reasons (e.g., power-on reset)
        Serial.println("[WAKE]: First Boot!");
#if defined(DEBUG) && !defined(STAY_AWAKE)
        Serial.println("[WAKE]: Halting for firmware update grace period...");
        delay(FIRMWARE_UPDATE_GRACE_PERIOD); // Halt for firmware update grace period
#endif
        reportState(doorOpened); // Report initial state
    }

    // Go to deep sleep, waiting for the door state to change...
    goToSleep(doorOpened);
}

// ----
// LOOP
// ----

/// @brief Runs indefinitely after setup() has completed
void loop()
{
    // Nothing to do in normal deep sleep mode
#ifdef STAY_AWAKE
    // Stay awake mode: continuously monitor the door sensor
    bool doorOpened = digitalRead(MAG_REED_SENSOR);
    if (doorOpened != lastDoorState)
    {
        lastDoorState = doorOpened;
        reportState(doorOpened);
    }

    mqttClient.loop();

    delay(50);
#endif
}
