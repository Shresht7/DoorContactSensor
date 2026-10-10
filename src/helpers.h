#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>

// Maximum allowed delay time for exponential backoff in microseconds (default: 8 hours)
#define EXPONENTIAL_BACKOFF_CAP (8ULL * 60 * 60 * 1000 * 1000)

/// @brief Calculates the exponential backoff time based on the number of attempts
/// @param base The base delay time in microseconds
/// @param attempt The current attempt number
/// @param cap The maximum allowed delay time in microseconds
/// @return The calculated backoff time in microseconds
uint64_t exponentialBackoff(uint64_t base, unsigned int attempt, uint64_t cap = EXPONENTIAL_BACKOFF_CAP)
{
    uint64_t result = base;
    while (attempt-- > 0 && result < cap)
    {
        result <<= 1;
    }
    return result > cap ? cap : result;
}

#endif // HELPERS_H
