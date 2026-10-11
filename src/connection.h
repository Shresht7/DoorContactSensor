#ifndef NETWORK_H
#define NETWORK_H

// CONNECT
// -------

/// @brief Checks if the device is currently connected to the network and MQTT broker
/// @return True if connected, false otherwise
bool isConnected();

/// @brief Checks if the device is currently connected to the WiFi network
/// @return True if connected to WiFi, false otherwise
bool isWiFiConnected();

/// @brief Connects to the specified WiFi network
/// @param ssid The SSID of the WiFi network
/// @param password The password of the WiFi network
/// @return True if the connection is successful, false otherwise
bool connectWiFi(const char *ssid, const char *password);

/// @brief Gets the duration of the most recent WiFi connect attempt in ms
uint32_t getLastWiFiConnectMs();

/// @brief Gets the duration of the most recent MQTT connect attempt in ms
uint32_t getLastMQTTConnectMs();

/// @brief Gets the Received Signal Strength Indicator (RSSI) of the current WiFi connection
/// @return The RSSI value in dBm, or 0 if not connected
int getRSSI();

/// @brief Connects to the specified MQTT broker
/// @param broker The address of the MQTT broker
/// @param port The port of the MQTT broker
/// @param clientId The client ID to use for the MQTT connection
/// @param username The username for the MQTT connection
/// @param password The password for the MQTT connection
/// @return True if the connection is successful, false otherwise
bool connectMQTT(const char *broker, int port, const char *clientId, const char *username, const char *password);

/// @brief Connects to the WiFi network and MQTT broker
/// @return True if both WiFi and MQTT connections are successful, false otherwise
bool connectNetwork();

// SUBSCRIBE
// ---------

/// @brief Handler for incoming MQTT messages (routed from the client loop)
typedef void (*MqttMessageCallback)(const char *topic, const uint8_t *payload, unsigned int length);

/// @brief Registers the handler for subscribed MQTT messages
/// @param callback The callback function to handle incoming MQTT messages
void setMqttMessageCallback(MqttMessageCallback callback);

/// @brief Callback function invoked when an MQTT message is received
/// @param topic The topic of the received message
/// @param payload The payload of the received message
/// @param length The length of the payload
void onMqttMessage(char *topic, uint8_t *payload, unsigned int length);

// PUBLISH
// -------

/// @brief Publishes a message to the specified MQTT topic
/// @param topic The MQTT topic to publish to
/// @param payload The message payload
/// @param retained Whether the message should be retained by the broker
/// @return True if the message was successfully published, false otherwise
bool publish(const char *topic, const char *payload, bool retained = true);

// KEEP ALIVE
// ----------

/// @brief Keeps the network connection alive by processing the MQTT client loop
void networkLoop();

// DISCONNECT
// ----------

/// @brief Disconnects from the WiFi network
void disconnectWiFi();

/// @brief Disconnects from the MQTT broker
void disconnectMQTT();

/// @brief Disconnects from the WiFi network and MQTT broker
void disconnectNetwork();

#endif // NETWORK_H
