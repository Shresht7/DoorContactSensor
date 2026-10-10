#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <stddef.h>
#include <stdint.h>

/// @brief Values substituted into the Home Assistant discovery message
struct DiscoveryInfo
{
    const char *deviceId;   // Unique identifier for the device
    const char *deviceName; // Human-readable name of the device
    const char *stateTopic; // MQTT topic for the device state
    const char *firmware;   // Firmware version of the device
    uint32_t expireAfterS;  // Time in seconds after which the discovery info expires
};

/// @brief Builds the MQTT discovery topic for a given device. <prefix>/device/<deviceId>/config
/// @param buffer The buffer to write the topic into
/// @param size The size of the buffer
/// @param prefix The MQTT topic prefix
/// @param deviceId The unique identifier of the device
/// @return The length of the written topic
size_t buildDiscoveryTopic(char *buffer, size_t size, const char *prefix, const char *deviceId);

/// @brief Builds the MQTT discovery payload for a given device
/// @param buffer The buffer to write the payload into
/// @param size The size of the buffer
/// @param info The discovery information for the device
/// @return The length of the written payload
size_t buildDiscoveryPayload(char *buffer, size_t size, const DiscoveryInfo &info);

#endif // DISCOVERY_H
