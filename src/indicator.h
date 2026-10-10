#ifndef INDICATOR_H
#define INDICATOR_H

/// @brief Configures the LED pin and makes sure the LED is off
void indicatorBegin();

/// @brief Lights the LED for the given time without blocking.
///        A new pulse restarts the timer. A duration of 0 does nothing.
void indicatorPulse(unsigned int durationMs);

/// @brief Turns the LED off once its pulse has run out. Call on every pass of loop()
void indicatorUpdate();

/// @brief Turns the LED off immediately. Call before sleeping
void indicatorOff();

#endif // INDICATOR_H
