#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include "config.h"

/// @brief Calculates the exponential backoff time based on the number of attempts
/// @param base The base delay time in microseconds
/// @param attempt The current attempt number
/// @param cap The maximum allowed delay time in microseconds
/// @return The calculated backoff time in microseconds
inline uint64_t exponentialBackoff(uint64_t base, unsigned int attempt, uint64_t cap = EXPONENTIAL_BACKOFF_CAP)
{
    // If the base delay is already greater than or equal to the cap, return the cap immediately
    if (base >= cap)
        return cap;

    // Initialize the result with the base delay time
    uint64_t result = base;

    // Double the result for each attempt until the cap is reached
    while (attempt-- > 0)
    {
        if (result > cap / 2)
            return cap;
        result <<= 1; // Double the result for exponential backoff
    }

    return result;
}

#endif // HELPERS_H
