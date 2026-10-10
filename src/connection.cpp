// LIBRARY
// -------

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "connection.h"
#include "config.h"
#include "secrets.h"

// WiFi AND MQTT CONFIGURATION
// ---------------------------

// The WiFi client instance used by the MQTT client
static WiFiClient wifiClient;

// The MQTT client instance used to communicate with the MQTT broker
static PubSubClient mqttClient(wifiClient);

// CONNECT WIFI
// ------------

bool isWiFiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

bool connectWiFi(const char *ssid, const char *password)
{
    Serial.println("[NET]: Connecting to WiFi...");
    WiFi.mode(WIFI_STA);

    // Configure the static IP address, if provided
    // This ensures the device uses a static IP address instead of obtaining one via DHCP
    // cutting down the time it takes to negotiate a DHCP lease and improving connection reliability
#ifdef STATIC_IP_ADDRESS
    WiFi.config(STATIC_IP_ADDRESS);
#endif

    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (!isWiFiConnected() && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS)
        delay(WIFI_POLL_INTERVAL_MS);

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

bool connectMQTT(const char *broker, int port, const char *clientId, const char *username, const char *password)
{
    Serial.println("[NET]: Connecting to MQTT broker...");

    // Set the MQTT server and port for the client to connect to
    mqttClient.setServer(broker, port);

    // Set the buffer size for the MQTT client to accommodate the payloads
    if (!mqttClient.setBufferSize(PAYLOAD_BUFFER_SIZE))
    {
        Serial.println("[NET]: Failed to allocate MQTT buffer");
        return false;
    }

    // Set the socket timeout for the MQTT client (in seconds)
    mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);

    // Attempt to connect to the MQTT broker within the specified connection timeout
    unsigned long start = millis();
    while (!mqttClient.connected() && (millis() - start) < MQTT_CONNECTION_TIMEOUT_MS)
    {
        mqttClient.connect(clientId, username, password);
        delay(MQTT_POLL_INTERVAL_MS);
    }

    // Check if the MQTT client successfully connected to the broker
    bool connected = mqttClient.connected();
    if (!connected)
        Serial.println("[NET]: MQTT connection failed");
    else
        Serial.println("[NET]: MQTT connection successful");
    return connected;
}

// CONNECT NETWORK
// ---------------

bool isConnected()
{
    return mqttClient.connected();
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
    if (!connectMQTT(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
        return false;

    return true;
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
    delay(MQTT_SPACE_MS);

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
