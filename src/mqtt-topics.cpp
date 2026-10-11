#include <Arduino.h>
#include <stdio.h>

#include "config.h"
#include "identity.h"
#include "mqtt-topics.h"

namespace
{
    /// @brief The size of the buffer used to store MQTT topics
    constexpr size_t TOPIC_BUFFER_SIZE = 128;

    /// @brief The buffer storing the retained state topic
    char stateTopicValue[TOPIC_BUFFER_SIZE] = {};
    /// @brief The buffer storing the command topic
    char cmdTopicValue[TOPIC_BUFFER_SIZE] = {};
    /// @brief The buffer storing the stay-awake topic
    char stayAwakeTopicValue[TOPIC_BUFFER_SIZE] = {};

    /// @brief Indicates whether the MQTT topics have been initialized
    bool initialized = false;

    bool buildTopic(char *buffer, size_t size, const char *suffix)
    {
        const int written = snprintf(
            buffer,
            size,
            "%s/%s/%s",
            MQTT_BASE_TOPIC,
            deviceId(),
            suffix);
        return written > 0 && static_cast<size_t>(written) < size;
    }
}

void mqttTopicsInit()
{
    // Skip if already initialized
    if (initialized)
        return;

    const bool stateOk = buildTopic(stateTopicValue, sizeof(stateTopicValue), "state");
    const bool cmdOk = buildTopic(cmdTopicValue, sizeof(cmdTopicValue), "cmd");
    const bool stayAwakeOk = buildTopic(stayAwakeTopicValue, sizeof(stayAwakeTopicValue), "stay-awake");

    initialized = stateOk && cmdOk && stayAwakeOk;

    if (!initialized)
    {
        // Handle initialization failure, e.g., log an error
        Serial.println("[MQTT]: Failed to construct MQTT topics");
        return;
    }
}

const char *mqttTopicState()
{
    return stateTopicValue;
}

const char *mqttTopicCmd()
{
    return cmdTopicValue;
}

const char *mqttTopicStayAwake()
{
    return stayAwakeTopicValue;
}
