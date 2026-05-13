/*
 * FreeRTOS V202112.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * This project provides a POSIX demo using a blinking LED.
 *
 * A POSIX thread controls the blinking LED.
 *
 * This file implements the code that is not demo specific.
 */

/* Standard includes */
#include <stdio.h>
#include <stdarg.h>

/* POSIX header files */
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>

/* RTOS header files */
#include <FreeRTOS.h>
#include <task.h>

/* Project headers */
#include "system.h"
#include "threshold.h"
#include "adc_sensor.h"
#include "timer_module.h"
#include "led_button.h"

/* TI includes */
#include "ti_msp_dl_config.h"

#define THREADSTACKSIZE 1024

//  UART configuration
static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_0Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

void UART_sendChar(char c)
{
    while (DL_UART_isTXFIFOFull(UART0));
    DL_UART_Main_transmitData(UART0, c);
}

void UART_sendString(char *str)
{
    int i = 0;
    while (str[i] != '\0') {
        UART_sendChar(str[i]);
        i++;
    }
}

void UART_print(char *msg, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    UART_sendString(buffer);
}

//  Hardware setup 
static void prvSetupHardware(void)
{
    SYSCFG_DL_init();
    ADC_Sensor_init();
    LED_Button_Init();
    initThresholds(&THRESH, TEMP_LOW_C, TEMP_HIGH_C, LIGHT_LOW_L, LIGHT_HIGH_L);
}

// Processing task 
struct processing_config_t {
    SystemState  *state;
    THRESHOLDS   *thresh;
};

void *process_temp_light(void *args)
{
    struct processing_config_t *config = (struct processing_config_t *) args;

    while (1) {
        config->state->status = processSensors(
            config->thresh,
            config->state->temperature,
            config->state->light
        );
        usleep(100000);     // 100 ms — fast enough to catch state changes
    }

    return NULL;
}

// UART task 
void *UARTTask(void *arg0)
{
    // Initialize UART hardware
    DL_UART_Main_reset(UART0);
    DL_UART_Main_enablePower(UART0);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_UART_Main_setClockConfig(UART0, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);
    DL_UART_Main_init(UART0, (DL_UART_Main_Config *) &gUART_0Config);
    DL_UART_Main_setOversampling(UART0, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART0, 17, 23);
    DL_UART_Main_enable(UART0);

    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM21, IOMUX_PINCM21_PF_UART0_TX);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM22, IOMUX_PINCM22_PF_UART0_RX);

    UART_sendString("System Started\r\n");

    while (1)
    {
        // Handle button presses every loop
        handleButtons();

        // Only read sensors and print when timer fires
        if (systemValue.sampleNow)
        {
            timer_clearFlag();  // clear immediately before any work

            // Read sensors
            systemValue.temperature = readTemperatureC();
            systemValue.light       = readLightPercent();   // 5000 lux (I2C stub)

            // Update LEDs based on current status and mode
            updateLEDs(systemValue.status, getMode());

            // Format temperature for UART (no %f — UART only handles int)
            int temp_int     = (int) systemValue.temperature;
            int temp_decimal = (int) ((systemValue.temperature - temp_int) * 100.0f);
            if (temp_decimal < 0) temp_decimal = -temp_decimal;

            int light_int = (int) systemValue.light;

            UART_print(
                "Temp: %d.%02d C | Light: %d lux | Status: %d | Mode: %d\r\n",
                temp_int, temp_decimal,
                light_int,
                systemValue.status,
                getMode()
            );
        }

        vTaskDelay(pdMS_TO_TICKS(50));  // 50 ms poll — keeps CPU free
    }

    return NULL;
}

//  Main
int main(void)
{
    pthread_t      thread_UART, thread_Timer, thread_Processing;
    pthread_attr_t attrs;
    struct sched_param priParam;
    int retc;

#ifdef __ICCARM__
    __iar_Initlocks();
#endif

    prvSetupHardware();

    pthread_attr_init(&attrs);
    priParam.sched_priority = 1;
    retc  = pthread_attr_setschedparam(&attrs, &priParam);
    retc |= pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
    if (retc != 0) { while (1); }

    // UART task — handles printing, button reads, LED updates
    retc = pthread_create(&thread_UART, &attrs, UARTTask, NULL);
    if (retc != 0) { while (1); }

    // Timer task — sets sampleNow every 500 ms
    retc = pthread_create(&thread_Timer, &attrs, timerTask, NULL);
    if (retc != 0) { while (1); }

    // Processing task — evaluates thresholds, updates systemValue.status
    static struct processing_config_t processing_config = {
        .state  = &systemValue,
        .thresh = &THRESH
    };
    retc = pthread_create(&thread_Processing, &attrs, process_temp_light, &processing_config);
    if (retc != 0) { while (1); }

    vTaskStartScheduler();

    return 0;
}

#if (configCHECK_FOR_STACK_OVERFLOW)
#if defined(__TI_COMPILER_VERSION__)
#pragma WEAK(vApplicationStackOverflowHook)
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
#elif (defined(__GNUC__) || defined(__ti_version__))
void __attribute__((weak))
vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
#endif
{
    while (1) {}
}
#endif
