#ifndef LED_BUTTON_H
#define LED_BUTTON_H

#include <stdbool.h>
#include <semaphore.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdbool.h>
#include <stdint.h>

#include "system.h"

#include "ti_msp_dl_config.h"

#define S2_HOLD_MS 500
#define BUTTON_DEBOUNCE_MS 30

typedef enum {
    MODE_MONITOR,
    MODE_THRESHOLD_VIEW,
    MODE_THRESHOLD_ADJUST,
    MODE_ALERT_DISPLAY
} OperatingMode;

extern OperatingMode currentMode;

void LED_Button_Init(void);
void updateLEDs(SystemStatus status, OperatingMode mode);
OperatingMode getMode(void);

#endif