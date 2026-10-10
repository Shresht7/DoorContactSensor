#ifndef CONFIG_H
#define CONFIG_H

// DEFINITIONS
// -----------

// The baud rate for serial communication (in bits per second)
#define BAUD_RATE 115200

#define MAG_REED_SENSOR 4 // Magnetic reed sensor pin
#define LED 8             // LED pin

#define DOOR_STABILITY_TIME_MS 250 // Minimum time the door state must remain unchanged before considering it stable
#define DOOR_SETTLE_DELAY_MS 500   // Minimum time the door state must remain steady before considering deep sleep

// First retry after 60 seconds (in microseconds)
// This value is used as the base delay before retrying a failed operation
// Subsequent retries use exponential backoff based on this base delay
#define RETRY_BASE_US (60ULL * 1000 * 1000)

// Maximum time the device can stay awake before forcing deep sleep (in milliseconds)
#define MAX_AWAKE 60000

// Grace period for firmware updates (in milliseconds)
#define FIRMWARE_UPDATE_GRACE_PERIOD 10000

#endif // CONFIG_H
