#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <stddef.h>
#include <stdint.h>

#include "config.h"

struct StatusReport
{
    bool doorClosed;             // True if the door is closed (magnetic reed sensor active)
    int rssi;                    // WiFi received signal strength indicator (RSSI) in dBm
    const char *wakeReason;      // Reason for the last wake-up (e.g., "heartbeat", "retry", "door movement")
    uint32_t wakeCount;          // Number of wakes since power up
    uint8_t failedPublishes;     // Consecutive failed MQTT publish attempts since last successful publish
    uint32_t awakeMs;            // Time the device has been awake since the last wake-up (in milliseconds)
    uint32_t wifiMs;             // Duration of the most recent WiFi connect attempt (ms)
    uint32_t mqttMs;             // Duration of the most recent MQTT connect attempt (ms)
    uint32_t reportMs;           // Duration from boot to publishing this report (ms)
    const char *firmwareVersion; // Current firmware version of the device
};

/// @brief Formats a status report as a JSON object
/// @param buffer Where to write the result (null-terminated)
/// @param size Size of the buffer in bytes
/// @param report The values to report
/// @return Characters written, excluding the terminator. 0 if it didn't fit.
size_t buildStatusReportPayload(char *buffer, size_t size, const StatusReport &report);

#endif // PAYLOAD_H
