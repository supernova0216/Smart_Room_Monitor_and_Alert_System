#include "led_button.h"
#include "ti_msp_dl_config.h"
#include <FreeRTOS.h>
#include <task.h>
#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCE_MS 50

OperatingMode currentMode = MODE_MONITOR;

static void setRGB(int r, int g, int b)
{
    // For your RGB LED test:
    // If colors are inverted, swap setPins and clearPins here.

    if (r)
        DL_GPIO_setPins(GPIO_LEDS_RED_PORT, GPIO_LEDS_RED_PIN);
    else
        DL_GPIO_clearPins(GPIO_LEDS_RED_PORT, GPIO_LEDS_RED_PIN);

    if (g)
        DL_GPIO_setPins(GPIO_LEDS_GREEN_PORT, GPIO_LEDS_GREEN_PIN);
    else
        DL_GPIO_clearPins(GPIO_LEDS_GREEN_PORT, GPIO_LEDS_GREEN_PIN);

    if (b)
        DL_GPIO_setPins(GPIO_LEDS_BLUE_PORT, GPIO_LEDS_BLUE_PIN);
    else
        DL_GPIO_clearPins(GPIO_LEDS_BLUE_PORT, GPIO_LEDS_BLUE_PIN);
}

void LED_Button_Init(void)
{
    DL_GPIO_initDigitalOutput(GPIO_LEDS_RED_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_LEDS_GREEN_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_LEDS_BLUE_IOMUX);

    DL_GPIO_enableOutput(GPIO_LEDS_RED_PORT, GPIO_LEDS_RED_PIN);
    DL_GPIO_enableOutput(GPIO_LEDS_GREEN_PORT, GPIO_LEDS_GREEN_PIN);
    DL_GPIO_enableOutput(GPIO_LEDS_BLUE_PORT, GPIO_LEDS_BLUE_PIN);

    // Keep S2 like your original code: pull-down, pressed = 1
    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_S2_IOMUX,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    // Keep S3 pull-up: pressed = 0
    DL_GPIO_initDigitalInputFeatures(GPIO_BUTTONS_S3_IOMUX,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    setRGB(0, 0, 0);
}

void updateLEDs(SystemStatus status, OperatingMode mode)
{
    switch (mode)
    {
        case MODE_MONITOR:
            setRGB(0, 1, 0);   // Green
            break;

        case MODE_THRESHOLD_VIEW:
            setRGB(0, 0, 1);   // Blue
            break;

        case MODE_THRESHOLD_ADJUST:
            setRGB(1, 1, 0);   // Yellow
            break;

        case MODE_ALERT_DISPLAY:
            setRGB(1, 0, 0);   // Red
            break;

        default:
            setRGB(0, 0, 0);
            break;
    }
}

static bool btn1LastPressed = false;
static bool btn2LastPressed = false;
static TickType_t btn1LastTime = 0;
static TickType_t btn2LastTime = 0;

void handleButtons(void)
{
    TickType_t now = xTaskGetTickCount();

    // S2 uses pull-down:
    // unpressed = 0
    // pressed   = 1
    bool btn1Pressed =
        (DL_GPIO_readPins(GPIO_BUTTONS_S2_PORT, GPIO_BUTTONS_S2_PIN) != 0);

    // S3 uses pull-up:
    // unpressed = 1
    // pressed   = 0
    bool btn2Pressed =
        (DL_GPIO_readPins(GPIO_BUTTONS_S3_PORT, GPIO_BUTTONS_S3_PIN) == 0);

    // S2: change mode only once when button is first pressed
    if (btn1Pressed && !btn1LastPressed)
    {
        if ((now - btn1LastTime) >= pdMS_TO_TICKS(DEBOUNCE_MS))
        {
            btn1LastTime = now;
            currentMode = (OperatingMode)((currentMode + 1) % 4);
        }
    }

    // S3: reset back to monitor mode
    if (btn2Pressed && !btn2LastPressed)
    {
        if ((now - btn2LastTime) >= pdMS_TO_TICKS(DEBOUNCE_MS))
        {
            btn2LastTime = now;
            currentMode = MODE_MONITOR;
        }
    }

    btn1LastPressed = btn1Pressed;
    btn2LastPressed = btn2Pressed;
}

OperatingMode getMode(void)
{
    return currentMode;
}