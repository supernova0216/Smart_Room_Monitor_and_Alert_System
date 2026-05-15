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
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_26);
    else
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_26);

    if (g)
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_27);
    else
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_27);

    if (b)
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_22);
    else
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_22);
}

void LED_Button_Init(void)
{
    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);    
    
    DL_GPIO_initDigitalOutput(IOMUX_PINCM57); // PB26 - red LED2 
    DL_GPIO_initDigitalOutput(IOMUX_PINCM58); // PB27 - green LED2 
    DL_GPIO_initDigitalOutput(IOMUX_PINCM50); // PB22 - blue LED2 

    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_26); 
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_27); 
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_22); 

    // Keep S1 like your original code: pull-down, pressed = 1, PA18
    DL_GPIO_initDigitalInputFeatures(IOMUX_PINCM40,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    // Keep S2 pull-up: pressed = 0, PB21
    DL_GPIO_initDigitalInputFeatures(IOMUX_PINCM49,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    setRGB(0, 0, 0);
}

void updateLEDs(SystemStatus status, OperatingMode mode)
{
    //uncommented after done integrating adc for alert overide
    if (status != NORMAL) {
        switch (status) {
            case TEMP_HIGH:
                setRGB(1, 0, 0);    // red
                break;
            
            case TEMP_LOW:
                setRGB(0, 0, 1);    // blue
                break;

            case LIGHT_HIGH:
                setRGB(1, 1, 0);    // yellow
                break;

            case LIGHT_LOW:
                setRGB(0, 1, 1);    // cyan
                break;

            case MULTIPLE_ALERT:
                setRGB(1, 0, 1);    // purple
                break;

            default:
                break;
        }
        return;
    }
    //normal mode display
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

    // S1 uses pull-down:
    // unpressed = 0
    // pressed   = 1
    bool btn1Pressed =
        (DL_GPIO_readPins(GPIOA, DL_GPIO_PIN_18) != 0);

    // S2 uses pull-up:
    // unpressed = 1
    // pressed   = 0
    bool btn2Pressed =
        (DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) == 0);

    // S1: change mode only once when button is first pressed
    if (btn1Pressed && !btn1LastPressed)
    {
        if ((now - btn1LastTime) >= pdMS_TO_TICKS(DEBOUNCE_MS))
        {
            btn1LastTime = now;
            currentMode = (OperatingMode)((currentMode + 1) % 4);
        }
    }

    // S2: reset back to monitor mode
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