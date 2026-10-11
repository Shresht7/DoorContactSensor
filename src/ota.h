#ifndef OTA_H
#define OTA_H

/// @brief Sets up the OTA (Over-the-Air) update functionality
void otaBegin();

/// @brief Handles OTA (Over-the-Air) update events
void otaUpdate();

/// @brief Checks if an OTA (Over-the-Air) update is currently in progress, used to block sleep
bool otaInProgress();

#endif
