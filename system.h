#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>
#include "threshold.h"

typedef enum {
    NORMAL,
    TEMP_HIGH,
    TEMP_LOW,
    LIGHT_HIGH,
    LIGHT_LOW,
    MULTIPLE_ALERT
} SystemStatus;

typedef struct {
    float temperature;
    float light;
    SystemStatus status;
    volatile bool sampleNow;
} SystemState;

extern SystemState systemValue;

SystemStatus processSensors(THRESHOLDS* thresh, float temp, float light);

#endif