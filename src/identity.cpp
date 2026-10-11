#include <Arduino.h>
#include <stdio.h>

#include "identity.h"

namespace
{
    /// @brief Prefix to use for the device ID
    constexpr char DEVICE_ID_PREFIX[] = "door-contact-sensor-";

    /// @brief Length of the MAC address in hexadecimal representation (without separators)
    /// 6-byte MAC address represented as 12 hexadecimal characters
    constexpr size_t MAC_HEX_LENGTH = 12;

    /// @brief Buffer size for storing the complete device ID (including null terminator)
    constexpr size_t DEVICE_ID_BUFFER_SIZE = sizeof(DEVICE_ID_PREFIX) - 1 + MAC_HEX_LENGTH + 1; // +1 for null terminator

    /// @brief Buffer to store the complete device ID
    char deviceIdValue[DEVICE_ID_BUFFER_SIZE] = {};

    /// @brief Indicates whether the identity module has been initialized
    bool initialized = false;
}

void identityBegin()
{
    // If already initialized, do nothing
    if (initialized)
        return;

    // Retrieve the MAC address from the ESP32's efuse
    const uint64_t mac = ESP.getEfuseMac();

    // Convert the MAC address to a hexadecimal string
    snprintf(deviceIdValue, sizeof(deviceIdValue), "%s%012llx", DEVICE_ID_PREFIX, static_cast<unsigned long long>(mac));

    // Mark the module as initialized
    initialized = true;
}

const char *deviceId()
{
    return deviceIdValue;
}
