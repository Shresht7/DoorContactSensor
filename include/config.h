#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ===========
// DEFINITIONS
// ===========

// SERIAL COMMUNICATION
// --------------------

// The baud rate for serial communication (in bits per second)
constexpr unsigned int BAUD_RATE = 115200;

// Grace period to allow the USB host to reattach after a reset (in milliseconds)
constexpr unsigned int USB_REATTACH_GRACE_MS = 2000;

// Loop delay to avoid busy-waiting (in milliseconds)
constexpr unsigned int LOOP_DELAY_MS = 50;

// PIN CONFIGURATIONS
// ------------------

constexpr uint8_t PIN_MAG_REED_SENSOR = 4; // Magnetic reed sensor pin
constexpr uint8_t PIN_LED = 8;             // LED pin

// Delay to allow the magnetic reed sensor to stabilize
constexpr unsigned int MAG_REED_STABILIZATION_DELAY_MS = 50;

// Number of times to blink during bootup
constexpr unsigned int BLINK_COUNT = 5;
// Delay for LED blink during bootup (in milliseconds)
constexpr unsigned int BLINK_MS = 250;

// STABILITY TIMINGS
// -----------------

// Minimum time the door state must remain unchanged before considering it stable
constexpr unsigned int DOOR_STABILITY_TIME_MS = 250;

// Minimum time the door state must remain steady before considering deep sleep
constexpr unsigned int DOOR_SETTLE_TIME_MS = 500;

// WIFI
// ----

// Timeout for WiFi connection attempts (in milliseconds)
constexpr unsigned int WIFI_CONNECT_TIMEOUT_MS = 10000;

// Polling interval for checking WiFi connection status (in milliseconds)
constexpr unsigned int WIFI_POLL_INTERVAL_MS = 100;

// MQTT
// ----

// MQTT client ID for connecting to the broker
constexpr const char *MQTT_CLIENT_ID = "door-sensor-client-01";

// MQTT topic for publishing the door sensor state
constexpr const char *MQTT_TOPIC = "door-sensor/state";

// MQTT socket timeout (in seconds)
constexpr unsigned int MQTT_SOCKET_TIMEOUT_S = 3;

// MQTT connection timeout (in milliseconds)
constexpr unsigned int MQTT_CONNECTION_TIMEOUT_MS = 10000;

// Polling interval for checking MQTT connection status (in milliseconds)
constexpr unsigned int MQTT_POLL_INTERVAL_MS = 100;

// Space between MQTT connection attempts (in milliseconds)
constexpr unsigned int MQTT_SPACE_MS = 100;

// EXPONENTIAL BACKOFF
// -------------------

// First retry after 60 seconds (in microseconds)
// This value is used as the base delay before retrying a failed operation
// Subsequent retries use exponential backoff based on this base delay
constexpr uint64_t RETRY_BASE_US = (60ULL * 1000 * 1000);

// DEEP SLEEP
// ----------

// Wake this often even if the door hasn't moved, and publish the current state,
// so Home Assistant can tell "closed and fine" apart from "dead battery"
constexpr uint64_t HEARTBEAT_INTERVAL_US = (8ULL * 60 * 60 * 1000 * 1000);

// Maximum time the device can stay awake before forcing deep sleep (in milliseconds)
constexpr unsigned int MAX_AWAKE_TIME_MS = 60000;

#endif // CONFIG_H
