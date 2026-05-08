#pragma once

#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "ti/driverlib/driverlib.h"

/* Default threshold values */
typedef enum {
    TEMP_LOW_F  = 61,       // Farenheit
    TEMP_LOW_C  = 16,       // Celsuis
    TEMP_HIGH_F = 85,       // Farenheit
    TEMP_HIGH_C = 29,       // Celsuis
    LIGHT_LOW   = 250,      // Lux
    LIGHT_HIGH  = 25000,    // Lux
} THRESH_STATUS;

/* Threshold type enumerations */
typedef enum {
    THRESHOLD_TEMP_LOW      = 0,
    THRESHOLD_TEMP_HIGH     = 1,
    THRESHOLD_LIGHT_LOW     = 2,
    THRESHOLD_LIGHT_HIGH    = 3,
} THRESH_TYPE

/* Structure for defining system thresholds */
typedef struct {
    int temp_low;
    int temp_high;
    int light_low;
    int light_high;
} Thresholds;


/* =============================================================================;
    initThresholds() - Initialize temperature/light threshold values.
    
    STANDARD VALUES:
    TEMP_LOW_F, TEMP_LOW_C, TEMP_HIGH_F, TEMP_HIGH_C, LIGHT_LOW, LIGHT_HIGH, 
    
    USAGE:
    Thresholds thresh; 
    initThresholds(&thresh, TEMP_LOW_F, TEMP_HIGH_F, LIGHT_LOW, LIGHT_HIGH);
============================================================================== */
void initThresholds(Threshholds* t, int tl, int th, int ll, int lh);


/* =============================================================================;
    updateThreshold() - Update a threshold value. Returns (1) on success.
    
    THRESH_TYPE:
    THRESHOLD_TEMP_LOW,
    THRESHOLD_TEMP_HIGH,
    THRESHOLD_LIGHT_LOW,
    THRESHOLD_LIGHT_HIGH,

    USAGE:
    Thresholds thresh; 
    if(!updateThreshold(&thresh, THRESHOLD_TEMP_HIGH, 100)) {
        // Handle error
    }
============================================================================== */
int updateThreshold(Thresholds* t, THRESH_TYPE type, int val);