#include <Arduino.h>
#include "esp_sleep.h"

#include "deepsleep.h"
#include "config.h"

void enterDeepSleep(bool currentPinState, uint64_t timerWakeUs)
{
    // If a timer wake is requested, configure it. A value of 0 means no timer wake.
    if (timerWakeUs > 0)
        esp_sleep_enable_timer_wakeup(timerWakeUs);

    // Wake when the pin goes to the opposite state of the current door state,
    // so any change (open->close or close->open) trigger an external wake.
    esp_deepsleep_gpio_wake_up_mode_t wakeMode = currentPinState
                                                     ? ESP_GPIO_WAKEUP_GPIO_LOW
                                                     : ESP_GPIO_WAKEUP_GPIO_HIGH;
    esp_deep_sleep_enable_gpio_wakeup(BIT(PIN_MAG_REED_SENSOR), wakeMode);

    Serial.println("[DEEPSLEEP]: Going to sleep...");
    Serial.flush(); // Ensure all serial output is sent before entering deep sleep

    esp_deep_sleep_start();
}
