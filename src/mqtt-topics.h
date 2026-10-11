#ifndef MQTT_TOPICS_H
#define MQTT_TOPICS_H

/// Initializes device-specific MQTT topics
void mqttTopicsInit();

/// Returns the retained state topic
const char *mqttTopicState();

/// Returns the command topic
const char *mqttTopicCmd();

/// Returns the stay-awake topic
const char *mqttTopicStayAwake();

#endif // MQTT_TOPICS_H
