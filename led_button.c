#include "led_button.h"
#include "UART_UI.h"

#define DEBOUNCE_MS 50

OperatingMode currentMode = MODE_MONITOR;

static void setRGB(int r, int g, int b)
{
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
    DL_GPIO_initPeripheralInputFunctionFeatures(IOMUX_PINCM40,
        IOMUX_PINCM40_PF_GPIOA_DIO18,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    // Keep S2 pull-up: pressed = 0, PB21
    DL_GPIO_initPeripheralInputFunctionFeatures(IOMUX_PINCM49,
        IOMUX_PINCM49_PF_GPIOB_DIO21,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_setUpperPinsPolarity(GPIOA, DL_GPIO_PIN_18_EDGE_RISE_FALL);
    DL_GPIO_setUpperPinsPolarity(GPIOB, DL_GPIO_PIN_21_EDGE_RISE_FALL);

    // Clear stale interrupt flags first
    DL_GPIO_clearInterruptStatus(GPIOA, DL_GPIO_PIN_18); // S1
    DL_GPIO_clearInterruptStatus(GPIOB, DL_GPIO_PIN_21); // S2

    // Enable GPIO pin interrupts
    DL_GPIO_enableInterrupt(GPIOA, DL_GPIO_PIN_18);
    DL_GPIO_enableInterrupt(GPIOB, DL_GPIO_PIN_21);

    // Enable interrupt group in NVIC
    NVIC_ClearPendingIRQ(GPIOA_INT_IRQn);
    NVIC_ClearPendingIRQ(GPIOB_INT_IRQn);

    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);

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

OperatingMode getMode(void)
{
    return currentMode;
}

void GROUP1_IRQHandler(void)
{
    BaseType_t wake = pdFALSE;
    ButtonEvent event;

    static TickType_t lastS1Tick = 0;
    static TickType_t lastS2Tick = 0;

    TickType_t now = xTaskGetTickCountFromISR();

    uint32_t s1 = DL_GPIO_getEnabledInterruptStatus(GPIOA, DL_GPIO_PIN_18);
    uint32_t s2 = DL_GPIO_getEnabledInterruptStatus(GPIOB, DL_GPIO_PIN_21);

    if (s1 & DL_GPIO_PIN_18) {
        DL_GPIO_clearInterruptStatus(GPIOA, DL_GPIO_PIN_18);

        if ((now - lastS1Tick) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
            lastS1Tick = now;

            if (DL_GPIO_readPins(GPIOA, DL_GPIO_PIN_18) != 0) {
                event = BTN_EVENT_S1_PRESS;
                xQueueSendFromISR(buttonQueue, &event, &wake);
            }
        }
    }

    if (s2 & DL_GPIO_PIN_21) {
        DL_GPIO_clearInterruptStatus(GPIOB, DL_GPIO_PIN_21);

        if ((now - lastS2Tick) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
            lastS2Tick = now;

            if (DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) == 0) {
                event = BTN_EVENT_S2_PRESS;
            } else {
                event = BTN_EVENT_S2_RELEASE;
            }

            xQueueSendFromISR(buttonQueue, &event, &wake);
        }
    }

    portYIELD_FROM_ISR(wake);
}