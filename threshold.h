#pragma once

#include <stdio.h>
#include "ti_msp_dl_config.h"
#include "ti/driverlib/driverlib.h"

/* Default threshold values */
#define TEMP_LOW_F      61.0
#define TEMP_LOW_C      16.0
#define TEMP_HIGH_F     85.0
#define TEMP_HIGH_C     29.0
#define LIGHT_LOW_L     250.0
#define LIGHT_HIGH_L    25000.0

/* Threshold type enumerations */
typedef enum {
    THRESHOLD_TEMP_LOW      = 0,
    THRESHOLD_TEMP_HIGH     = 1,
    THRESHOLD_LIGHT_LOW     = 2,
    THRESHOLD_LIGHT_HIGH    = 3,
} THRESH_TYPE;

/* Structure for defining system thresholds */
typedef struct thresholds_t{
    float temp_low;
    float temp_high;
    float light_low;
    float light_high;
} THRESHOLDS;

extern THRESHOLDS THRESH;

/* =============================================================================;
    initThresholds() - Initialize temperature/light threshold values.
    
    THRESH_VALS:
    TEMP_LOW_F, TEMP_LOW_C, TEMP_HIGH_F, TEMP_HIGH_C, LIGHT_LOW, LIGHT_HIGH, 
    
    USAGE:
    initThresholds(THRESH, TEMP_LOW_F, TEMP_HIGH_F, LIGHT_LOW, LIGHT_HIGH);
============================================================================== */
void initThresholds(THRESHOLDS* t, float tl, float th, float ll, float lh);


/* =============================================================================;
    updateThreshold() - Update a threshold value. Returns (1) on success.
    
    THRESH_TYPE:
    THRESHOLD_TEMP_LOW, THRESHOLD_TEMP_HIGH, THRESHOLD_LIGHT_LOW, THRESHOLD_LIGHT_HIGH,

    USAGE:
    if(!updateThreshold(THRESH, THRESHOLD_TEMP_HIGH, 100)) {
        // Handle error
    }
============================================================================== */
uint8_t updateThreshold(THRESHOLDS* t, THRESH_TYPE type, float val);