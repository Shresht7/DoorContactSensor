// Library
#include <stdio.h>
#include <string.h>

#include "discovery.h"

// Device-based Discovery:
// The discovery message that declares the device and all of its entities.
static const char DISCOVERY_PAYLOAD_TEMPLATE[] = R"json({
"dev":{"ids":["@ID@"],"name":"@NAME@","mf":"DIY","mdl":"ESP32-C3 reed sensor","sw":"@FW@"},
"o":{"name":"door-contact-sensor firmware","sw":"@FW@"},
"state_topic":"@TOPIC@",
"cmps":{
"door":{"p":"binary_sensor","unique_id":"@ID@_door","name":"Door","device_class":"door","value_template":"{{ value_json.state }}","payload_on":"OPEN","payload_off":"CLOSED","expire_after":@EXPIRE@},
"rssi":{"p":"sensor","unique_id":"@ID@_rssi","name":"Signal Strength","device_class":"signal_strength","unit_of_measurement":"dBm","state_class":"measurement","entity_category":"diagnostic","value_template":"{{ value_json.rssi }}"},
"awake":{"p":"sensor","unique_id":"@ID@_awake","name":"Awake Time","device_class":"duration","unit_of_measurement":"ms","state_class":"measurement","entity_category":"diagnostic","value_template":"{{ value_json.awake_ms }}"},
"failed":{"p":"sensor","unique_id":"@ID@_failed","name":"Failed Publishes","state_class":"measurement","entity_category":"diagnostic","value_template":"{{ value_json.failed }}"},
"wakes":{"p":"sensor","unique_id":"@ID@_wakes","name":"Wake Count","state_class":"total_increasing","entity_category":"diagnostic","value_template":"{{ value_json.wake_count }}"},
"wake":{"p":"sensor","unique_id":"@ID@_wake","name":"Wake Reason","entity_category":"diagnostic","value_template":"{{ value_json.wake }}"},
"wifi_ms":{"p":"sensor","unique_id":"@ID@_wifi_ms","name":"WiFi Connect Time","device_class":"duration","unit_of_measurement":"ms","state_class":"measurement","entity_category":"diagnostic","value_template":"{{ value_json.wifi_ms }}"},
"mqtt_ms":{"p":"sensor","unique_id":"@ID@_mqtt_ms","name":"MQTT Connect Time","device_class":"duration","unit_of_measurement":"ms","state_class":"measurement","entity_category":"diagnostic","value_template":"{{ value_json.mqtt_ms }}"},
"report_ms":{"p":"sensor","unique_id":"@ID@_report_ms","name":"Report Time","device_class":"duration","unit_of_measurement":"ms","state_class":"measurement","entity_category":"diagnostic","value_template":"{{ value_json.report_ms }}"},
"stayawake":{"p":"switch","unique_id":"@ID@_stay_awake","name":"Stay Awake","command_topic":"@CMD@","state_topic":"@SW@","payload_on":"ON","payload_off":"OFF","state_on":"ON","state_off":"OFF","retain":true}
}})json";

/// @brief Structure representing a token for template substitution.
struct Token
{
    const char *key;   // Including the @ markers. e.g. "@ID@"
    const char *value; // The value to substitute for the token
};

/// @brief Fills a buffer with a template string, substituting tokens with their corresponding values.
/// @param buffer The buffer to fill with the resulting string.
/// @param size The size of the buffer.
/// @param tmpl The template string containing placeholders for tokens.
/// @param tokens An array of Token structures representing the substitutions to make.
/// @param tokenCount The number of tokens in the tokens array.
/// @return The number of characters written to the buffer, or 0 if an error occurred (e.g., not enough space or missing token).
static size_t fillTemplate(char *buffer, size_t size, const char *tmpl, const Token *tokens, size_t tokenCount)
{
    size_t n = 0;
    for (const char *p = tmpl; *p;)
    {
        if (*p == '@')
        {
            bool matched = false;
            for (size_t i = 0; i < tokenCount; i++)
            {
                size_t keyLen = strlen(tokens[i].key);
                if (strncmp(p, tokens[i].key, keyLen) != 0)
                    continue;
                size_t valueLen = strlen(tokens[i].value);
                if (n + valueLen > size)
                    return 0; // Not enough space in the buffer to insert the value
                memcpy(buffer + n, tokens[i].value, valueLen);
                n += valueLen;
                p += keyLen;
                matched = true;
            }
            if (!matched)
                return 0; // No matching token found for the current placeholder
        }
        else
        {
            if (n + 1 > size)
                return 0; // Not enough space in the buffer to insert the character
            buffer[n++] = *p++;
        }
    }
    if (n < size)
        buffer[n] = '\0';
    return n;
}

size_t buildDiscoveryTopic(char *buffer, size_t size, const char *prefix, const char *deviceId)
{
    int n = snprintf(buffer, size, "%s/device/%s/config", prefix, deviceId);
    return (n < 0 || (size_t)n >= size) ? 0 : (size_t)n;
}

size_t buildDiscoveryPayload(char *buffer, size_t size, const DiscoveryInfo &info)
{
    char expire[12];
    snprintf(expire, sizeof(expire), "%lu", (unsigned long)info.expireAfterS);

    const Token tokens[] = {
        {"@ID@", info.deviceId},
        {"@NAME@", info.deviceName},
        {"@TOPIC@", info.stateTopic},
        {"@FW@", info.firmware},
        {"@EXPIRE@", expire},
        {"@CMD@", info.cmdTopic},
        {"@SW@", info.stayAwakeTopic},
    };
    size_t n = fillTemplate(buffer, size, DISCOVERY_PAYLOAD_TEMPLATE, tokens, sizeof(tokens) / sizeof(tokens[0]));

    if (n == 0 && size > 0)
        buffer[0] = '\0';
    return n;
}
