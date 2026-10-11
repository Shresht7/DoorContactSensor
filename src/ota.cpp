#include <ArduinoOTA.h>

#include "ota.h"
#include "identity.h"
#include "config.h"
#include "secrets.h"

/// @brief Indicates whether an OTA (Over-the-Air) update is currently in progress
static bool otaActive = false;

void otaBegin()
{
    // Initialize the ArduinoOTA library with the device hostname, port, and password
    ArduinoOTA.setHostname(deviceId());
    ArduinoOTA.setPort(OTA_PORT);
#ifdef OTA_PASSWORD
    ArduinoOTA.setPassword(OTA_PASSWORD);
#endif

    // Set up OTA event handlers for start, end, and error events
    ArduinoOTA.onStart([]()
                       { otaActive = true; Serial.println("[OTA] Update Starting"); });
    ArduinoOTA.onEnd([]()
                     { otaActive = false; Serial.println("[OTA] Update Complete. Rebooting..."); });
    ArduinoOTA.onError([](ota_error_t e)
                       { otaActive = false; Serial.printf("[OTA] Update Error: [%u]\n", e); });

    ArduinoOTA.begin();
    Serial.println("[OTA]: Service Started");
}

void otaUpdate()
{
    ArduinoOTA.handle();
}

bool otaInProgress()
{
    return otaActive;
}
