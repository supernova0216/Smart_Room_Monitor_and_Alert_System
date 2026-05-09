#include "led_button.h"
#include "ti_msp_dl_config.h"
#include <FreeRTOS.h>
#include <task.h>

#define DEBOUNCE_MS 50

OperatingMode currentMode = MODE_MONITOR;

static void setRGB(int r, int g, int b) {
    if (r) DL_GPIO_setPins(GPIO_LEDS_RED_PORT, GPIO_LEDS_RED_PIN);
    else   DL_GPIO_clearPins(GPIO_LEDS_RED_PORT, GPIO_LEDS_RED_PIN);

    if (g) DL_GPIO_setPins(GPIO_LEDS_GREEN_PORT, GPIO_LEDS_GREEN_PIN);
    else   DL_GPIO_clearPins(GPIO_LEDS_GREEN_PORT, GPIO_LEDS_GREEN_PIN);

    if (b) DL_GPIO_setPins(GPIO_LEDS_BLUE_PORT, GPIO_LEDS_BLUE_PIN);
    else   DL_GPIO_clearPins(GPIO_LEDS_BLUE_PORT, GPIO_LEDS_BLUE_PIN);
}

void LED_Button_Init(void) {
    DL_GPIO_initDigitalOutput(GPIO_LEDS_RED_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_LEDS_GREEN_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_LEDS_BLUE_IOMUX);

    DL_GPIO_enableOutput(GPIO_LEDS_RED_PORT, GPIO_LEDS_RED_PIN);
    DL_GPIO_enableOutput(GPIO_LEDS_GREEN_PORT, GPIO_LEDS_GREEN_PIN);
    DL_GPIO_enableOutput(GPIO_LEDS_BLUE_PORT, GPIO_LEDS_BLUE_PIN);

    // S1 on board: pull-down
    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_S2_IOMUX,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    // S2 on board: pull-up
    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_S3_IOMUX,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    setRGB(0, 0, 0);
}

void updateLEDs(SystemStatus status, OperatingMode mode) {
    if (mode == MODE_THRESHOLD_ADJUST) {
        setRGB(0, 0, 1); // Blue
        return;
    }

    switch (status) {
        case NORMAL:
            setRGB(0, 1, 0); // Green
            break;
        case TEMP_HIGH:
        case TEMP_LOW:
        case LIGHT_HIGH:
        case LIGHT_LOW:
            setRGB(1, 0, 0); // Red
            break;
        case MULTIPLE_ALERT:
            setRGB(1, 1, 0); // Yellow
            break;
        default:
            setRGB(0, 1, 0);
            break;
    }
}

static bool btn1Last = false;  // pull-down default is 0
static bool btn2Last = true;   // pull-up default is 1
static uint32_t btn1Time = 0;
static uint32_t btn2Time = 0;

void handleButtons(void) {
    uint32_t now = xTaskGetTickCount();

    // S1 on board = pull-down: pressed = 1
    bool btn1 = DL_GPIO_readPins(GPIO_BUTTONS_S2_PORT, GPIO_BUTTONS_S2_PIN) != 0;
    // S2 on board = pull-up: pressed = 0
    bool btn2 = DL_GPIO_readPins(GPIO_BUTTONS_S3_PORT, GPIO_BUTTONS_S3_PIN) != 0;

    // S1: rising edge = pressed
    if (btn1 && !btn1Last) {
        if ((now - btn1Time) > DEBOUNCE_MS) {
            btn1Time = now;
            currentMode = (OperatingMode)((currentMode + 1) % 4);
        }
    }
    btn1Last = btn1;

    // S2: falling edge = pressed
    if (!btn2 && btn2Last) {
        if ((now - btn2Time) > DEBOUNCE_MS) {
            btn2Time = now;
            currentMode = MODE_MONITOR;
        }
    }
    btn2Last = btn2;
}

OperatingMode getMode(void) {
    return currentMode;
}