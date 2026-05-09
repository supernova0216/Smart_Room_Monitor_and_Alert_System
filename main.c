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

/* POSIX header files */
#include <pthread.h>

/* RTOS header files */
#include <FreeRTOS.h>
#include <task.h>

#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <system.h>
#include <stdarg.h>
#include <timer_module.h>
#include "led_button.h"

/* TI includes for driver configuration */
#include "ti_msp_dl_config.h"

extern void *Thread(void *arg0);

#define THREADSTACKSIZE 1024

static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1};

static const DL_UART_Main_Config gUART_0Config = {
    .mode = DL_UART_MAIN_MODE_NORMAL,
    .direction = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity = DL_UART_MAIN_PARITY_NONE,
    .wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits = DL_UART_MAIN_STOP_BITS_ONE};

void UART_sendChar(char c)
{
    while (DL_UART_isTXFIFOFull(UART0))
        ;
    DL_UART_Main_transmitData(UART0, c);
}

void UART_sendString(char *str)
{
    int i = 0;
    while (str[i] != '\0')
    {
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

static void prvSetupHardware(void)
{
    SYSCFG_DL_init();
}

void *UARTTask(void *arg0)
{
    DL_UART_Main_reset(UART0);
    DL_UART_Main_enablePower(UART0);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_UART_Main_setClockConfig(UART0, (DL_UART_Main_ClockConfig *)&gUART_0ClockConfig);
    DL_UART_Main_init(UART0, (DL_UART_Main_Config *)&gUART_0Config);
    DL_UART_Main_setOversampling(UART0, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART0, 17, 23);
    DL_UART_Main_enable(UART0);

    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM21, IOMUX_PINCM21_PF_UART0_TX);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM22, IOMUX_PINCM22_PF_UART0_RX);

    UART_sendString("System Started\r\n");

    // --- LED TEST ---
    LED_Button_Init();

    UART_sendString("Testing LEDs...\r\n");

    updateLEDs(NORMAL, MODE_MONITOR);
    UART_sendString("GREEN - Normal\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    updateLEDs(TEMP_HIGH, MODE_MONITOR);
    UART_sendString("RED - Alert\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    updateLEDs(MULTIPLE_ALERT, MODE_MONITOR);
    UART_sendString("YELLOW - Multiple Alert\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    updateLEDs(NORMAL, MODE_THRESHOLD_ADJUST);
    UART_sendString("BLUE - Config Mode\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    UART_sendString("LED test done! Now testing buttons...\r\n");
    // --- END LED TEST ---

    while (1)
    {
        // Debug: read raw button states
        // S1 on board = GPIO_BUTTONS_S2, S2 on board = GPIO_BUTTONS_S3
        bool s1 = DL_GPIO_readPins(GPIO_BUTTONS_S2_PORT, GPIO_BUTTONS_S2_PIN) != 0;
        bool s2 = DL_GPIO_readPins(GPIO_BUTTONS_S3_PORT, GPIO_BUTTONS_S3_PIN) != 0;

        UART_print("S1=%d S2=%d Mode=%d\r\n", s1, s2, getMode());

        handleButtons();
        updateLEDs(systemValue.status, getMode());

        if (systemValue.sampleNow)
        {
            systemValue.sampleNow = false;
            int temp_int = (int)systemValue.temperature;
            int temp_decimal = (int)((systemValue.temperature - temp_int) * 100);
            UART_print("Temperature: %d.%02d C | Light: %d %% | Status: %d\r\n",
                       temp_int, temp_decimal, (int)(systemValue.light), systemValue.status);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
    return NULL;
}

int main(void)
{
    // // Threshold Testing
    // prvSetupHardware();
    // testProcessing();
    // while(1);

    pthread_t thread_processing;
    pthread_t thread_UART, thread_Timer;
    pthread_attr_t attrs;
    struct sched_param priParam;
    int retc;

#ifdef __ICCARM__
    __iar_Initlocks();
#endif

    prvSetupHardware();
    pthread_attr_init(&attrs);

    priParam.sched_priority = 1;
    retc = pthread_attr_setschedparam(&attrs, &priParam);
    retc |= pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
    if (retc != 0)
    {
        printf("Failed to set thread attributes\n");
        while (1)
        {
        }
    }

    retc = pthread_create(&thread_UART, &attrs, UARTTask, NULL);
    if (retc != 0)
    {
        printf("Failed to create UART task\n");
        while (1)
        {
        }
    }

    retc = pthread_create(&thread_Timer, &attrs, timerTask, NULL);
    if (retc != 0)
    {
        printf("Failed to create Timer task\n");
        while (1)
        {
        }
    }

    vTaskStartScheduler();

    return (0);
}