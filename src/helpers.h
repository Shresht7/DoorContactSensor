#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include "esp_sleep.h"
#include "config.h"

/// @brief Calculates the exponential backoff time based on the number of attempts
/// @param base The base delay time in microseconds
/// @param attempt The current attempt number
/// @param cap The maximum allowed delay time in microseconds
/// @return The calculated backoff time in microseconds
inline uint64_t exponentialBackoff(uint64_t base, unsigned int attempt, uint64_t cap = EXPONENTIAL_BACKOFF_CAP)
{
    uint64_t result = base;               // Initialize the result with the base delay time
    while (attempt-- > 0 && result < cap) // Continue doubling until attempts are exhausted or cap is reached
        result <<= 1;                     // Double the result for exponential backoff
    return result > cap ? cap : result;   // Ensure the result does not exceed the cap
}

enum WakeReason
{
    WAKE_REASON_DOOR,
    WAKE_REASON_TIMER,
    WAKE_REASON_BOOT
};

inline const char *WakeReasonToString(WakeReason reason)
{
    switch (reason)
    {
    case WAKE_REASON_DOOR:
        return "door";
    case WAKE_REASON_TIMER:
        return "timer";
    default:
        return "boot";
    }
}

static WakeReason wakeReasonFromCause(esp_sleep_wakeup_cause_t cause)
{
    switch (cause)
    {
    case ESP_SLEEP_WAKEUP_GPIO:
        return WAKE_REASON_DOOR;
    case ESP_SLEEP_WAKEUP_TIMER:
        return WAKE_REASON_TIMER;
    default:
        return WAKE_REASON_BOOT;
    }
}

#endif // HELPERS_H
