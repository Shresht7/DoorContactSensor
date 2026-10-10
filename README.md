# Door Contact Sensor

Battery-powered door contact sensor firmware for the ESP32-C3 Super Mini. Publishes open/closed state over MQTT and auto-discovers in Home Assistant.

## Hardware

- ESP32-C3 Super Mini
- Magnetic reed sensor module (GPIO4, input pullup)
- TP4056 charger + lithium battery
- Onboard LED on GPIO8

## Requirements

- [PlatformIO](https://platformio.org/)

## Setup

⚠️ TODO

## Build & Upload

PlatformIO environments:

| Env       | Purpose                            |
| --------- | ---------------------------------- |
| `release` | Production build (deep sleep on)   |
| `debug`   | Serial over USB, no deep sleep     |
| `dev`     | Debug + stays awake (`STAY_AWAKE`) |

### Development

```sh
pio run -e dev -t upload
pio device monitor -e dev
```

### Release

```sh
pio run -e release -t upload
```

## Behavior

- Publishes `door-sensor/state` as JSON: `state`, `rssi`, `wake`, `wake_count`, `failed`, `awake_ms`, `fw`.
- Wakes from deep sleep on door state change (GPIO) or a heartbeat timer; retries failed publishes with exponential backoff.
- Publishes Home Assistant MQTT discovery on connect, so the device shows up automatically.

## References

- https://esphome.io/
- https://esphome.io/components/deep_sleep
- https://community.home-assistant.io/t/use-a-binary-sensor-to-wake-up-from-deep-sleep/898420
