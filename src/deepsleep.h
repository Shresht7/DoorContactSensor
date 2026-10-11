#ifndef DEEPSLEEP_H
#define DEEPSLEEP_H

#include <stdint.h>
#include <esp_sleep.h>

// DEEP SLEEP
// ----------

/// @brief Enters deep sleep. The chip wakes when the sensor pin changes level or optionally after a timeout, whichever happens first.
/// @param currentPinState The current level of the sensor pin. The wake is armed for the opposite level, so any change wakes the chip.
/// @param timerWakeUs Wake after this many microseconds. 0 means no timer wake.
void enterDeepSleep(bool currentPinState, uint64_t timerWakeUs);

// WAKE
// ----

enum class WakeReason
{
    Door,
    Timer,
    Boot
};

inline const char *WakeReasonToString(WakeReason reason)
{
    switch (reason)
    {
    case WakeReason::Door:
        return "door";
    case WakeReason::Timer:
        return "timer";
    case WakeReason::Boot:
        return "boot";
    default:
        return "unknown";
    }
}

inline WakeReason wakeReasonFromCause(esp_sleep_wakeup_cause_t cause)
{
    switch (cause)
    {
    case ESP_SLEEP_WAKEUP_GPIO:
        return WakeReason::Door;
    case ESP_SLEEP_WAKEUP_TIMER:
        return WakeReason::Timer;
    default:
        return WakeReason::Boot;
    }
}

#endif // DEEPSLEEP_H
