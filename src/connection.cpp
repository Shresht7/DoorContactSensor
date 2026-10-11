// LIBRARY
// -------

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "connection.h"
#include "identity.h"
#include "mqtt-topics.h"
#include "config.h"
#include "secrets.h"

// WiFi AND MQTT CONFIGURATION
// ---------------------------

// The WiFi client instance used by the MQTT client
static WiFiClient wifiClient;

// The MQTT client instance used to communicate with the MQTT broker
static PubSubClient mqttClient(wifiClient);

/// Duration of the most recent WiFi connect attempt in ms
static uint32_t lastWiFiConnectMs = 0;

/// Duration of the most recent MQTT connect attempt in ms
static uint32_t lastMQTTConnectMs = 0;

// CONNECT WIFI
// ------------

bool isWiFiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

bool connectWiFi(const char *ssid, const char *password)
{
    Serial.println("[NET]: Connecting to WiFi...");
    unsigned long start = millis();

    // Set the WiFi mode to station (client) mode
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(deviceId());

    // Disable WiFi power saver mode. This should speed up MQTT connections.
    // WiFi is shut down when the device goes to sleep anyway.
    WiFi.setSleep(false);

    // Configure the static IP address, if provided
    // This ensures the device uses a static IP address instead of obtaining one via DHCP
    // cutting down the time it takes to negotiate a DHCP lease and improving connection reliability
#ifdef STATIC_IP_ADDRESS
    WiFi.config(STATIC_IP_ADDRESS, STATIC_GATEWAY, STATIC_SUBNET, STATIC_DNS1, STATIC_DNS2);
#endif

    // Initiate the WiFi connection with the provided SSID and password
    WiFi.begin(ssid, password);

    // Wait for the WiFi connection to be established or the timeout to expire
    while (!isWiFiConnected() && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS)
        delay(WIFI_POLL_INTERVAL_MS);

    // Record how long the WiFi connect attempt took (for telemetry)
    lastWiFiConnectMs = (uint32_t)(millis() - start);

    // Check if the WiFi connection was successful and print the result
    if (!isWiFiConnected())
    {
        Serial.println("[NET]: WiFi connection failed");
        return false;
    }
    Serial.println("[NET]: WiFi connection successful");

    return true;
}

int getRSSI()
{

    if (!isWiFiConnected())
        return 0;
    return WiFi.RSSI();
}

// CONNECT MQTT
// ------------

bool connectMQTT(const IPAddress broker, const int port, const char *clientId, const char *username, const char *password)
{
    Serial.println("[NET]: Connecting to MQTT broker...");
    unsigned long start = millis();

    // Set the MQTT server and port for the client to connect to
    mqttClient.setServer(broker, port);

    // Set the MQTT packet buffer size to accomodate the largest payload and packet overhead
    if (!mqttClient.setBufferSize(MQTT_BUFFER_SIZE))
    {
        Serial.println("[NET]: Failed to allocate MQTT buffer");
        return false;
    }

    // Set the socket timeout for the MQTT client (in seconds)
    mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);

    // Set the callback function for incoming MQTT messages
    mqttClient.setCallback(onMqttMessage);

    // Retry synchronous connection attempts until connected or the time budget expires.
    while (!mqttClient.connected() && (millis() - start) < MQTT_CONNECTION_TIMEOUT_MS)
    {
        mqttClient.connect(clientId, username, password);
        delay(MQTT_RETRY_DELAY_MS);
    }

    // Record how long the MQTT connect attempt took (for telemetry)
    lastMQTTConnectMs = (uint32_t)(millis() - start);

    // Check if the MQTT client successfully connected to the broker
    bool connected = mqttClient.connected();
    if (!connected)
        Serial.println("[NET]: MQTT connection failed");
    else
        Serial.println("[NET]: MQTT connection successful");

    // Subscribe to the command topic if the connection was successful
    if (connected)
    {
        mqttClient.subscribe(mqttTopicCmd());
        Serial.println("[NET]: Subscribed to MQTT " + String(mqttTopicCmd()));
    }

    return connected;
}

// CONNECT NETWORK
// ---------------

bool isConnected()
{
    return mqttClient.connected();
}

uint32_t getLastWiFiConnectMs()
{
    return lastWiFiConnectMs;
}

uint32_t getLastMQTTConnectMs()
{
    return lastMQTTConnectMs;
}

bool connectNetwork()
{

    // Return early if already connected to network
    if (isConnected())
        return true;

    if (!isWiFiConnected())
    {
        // Attempt to connect to WiFi
        if (!connectWiFi(WIFI_SSID, WIFI_PASSWORD))
            return false;
    }

    // Attempt to connect to MQTT broker
    if (!connectMQTT(MQTT_BROKER, MQTT_PORT, deviceId(), MQTT_USERNAME, MQTT_PASSWORD))
        return false;

    return true;
}

// SUBSCRIBE
// ---------

/// @brief The callback function to handle incoming MQTT messages
static MqttMessageCallback messageCallback = nullptr;

/// @brief Registers the handler for subscribed MQTT messages
/// @param callback The callback function to handle incoming MQTT messages
void setMqttMessageCallback(MqttMessageCallback callback)
{
    messageCallback = callback;
}

/// Delegates incoming MQTT messages to the registered callback function
void onMqttMessage(char *topic, uint8_t *payload, unsigned int length)
{
    if (messageCallback)
        messageCallback(topic, payload, length);
}

// PUBLISH
// -------

bool publish(const char *topic, const char *payload, bool retained)
{
    // Ensure the device is connected to the network before attempting to publish
    if (!connectNetwork())
    {
        Serial.println("[NET]: Connection Failed, skipping publish");
        return false;
    }

    // Attempt to publish the state to the MQTT broker
    bool ok = mqttClient.publish(topic, payload, retained);

    // Process the MQTT client loop and allow time for the message to be sent
    mqttClient.loop();
    delay(MQTT_PUBLISH_SETTLE_MS);

    return ok;
}

// NETWORK LOOP
// ------------

void networkLoop()
{
    mqttClient.loop();
}

// DISCONNECT
// ----------

void disconnectWiFi()
{

    WiFi.disconnect(true);
    Serial.println("[NET]: Disconnected from WiFi network");
}

void disconnectMQTT()
{
    mqttClient.disconnect();
    Serial.println("[NET]: Disconnected from MQTT broker");
}

void disconnectNetwork()
{
    disconnectMQTT();
    disconnectWiFi();
}
