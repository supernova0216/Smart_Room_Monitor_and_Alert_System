#ifndef LED_BUTTON_H
#define LED_BUTTON_H

#include "system.h"
#include <stdbool.h>

typedef enum {
    MODE_MONITOR,
    MODE_THRESHOLD_VIEW,
    MODE_THRESHOLD_ADJUST,
    MODE_ALERT_DISPLAY
} OperatingMode;

extern OperatingMode currentMode;

void LED_Button_Init(void);
void updateLEDs(SystemStatus status, OperatingMode mode);
void handleButtons(void);
OperatingMode getMode(void);

#endif