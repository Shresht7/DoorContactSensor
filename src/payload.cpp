// Library
#include <stdio.h>

#include "payload.h"

size_t buildStatusReportPayload(char *buffer, size_t size, const StatusReport &r)
{
    // Build the JSON payload for the status report
    int n = snprintf(
        buffer,
        size,
        "{\"state\":\"%s\",\"rssi\":%d,\"wake\":\"%s\",\"wake_count\":%lu,\"failed\":%u,\"awake_ms\":%lu,\"fw\":\"%s\"}",
        r.doorClosed ? "CLOSED" : "OPEN",
        r.rssi,
        r.wakeReason,
        (unsigned long)r.wakeCount,
        (unsigned)r.failedPublishes,
        (unsigned long)r.awakeMs,
        r.firmwareVersion);

    // snprintf returns the length it *wanted* to write, so n >= size means truncation
    if (n < 0 || (size_t)n >= size)
    {
        if (size > 0)
            buffer[0] = '\0';
        return 0;
    }

    // Return the number of characters written, excluding the null terminator
    return (size_t)n;
}
