#ifndef DEEPSLEEP_H
#define DEEPSLEEP_H

#include <stdint.h>

/// @brief Enters deep sleep. The chip wakes when the sensor pin changes level or optionally after a timeout, whichever happens first.
/// @param currentPinState The current level of the sensor pin. The wake is armed for the opposite level, so any change wakes the chip.
/// @param timerWakeUs Wake after this many microseconds. 0 means no timer wake.
void enterDeepSleep(bool currentPinState, uint64_t timerWakeUs);

#endif // DEEPSLEEP_H
