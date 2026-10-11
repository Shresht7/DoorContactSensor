#ifndef IDENTITY_H
#define IDENTITY_H

#include <stddef.h>

/// Initializes the runtime device identity module
void identityBegin();

/// Generates and returns the stable MAC-derived device ID
/// @returns the stable MAC-derived device ID
const char *deviceId();

#endif // IDENTITY_H
