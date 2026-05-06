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

#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>

/* TI includes for driver configuration */
#include "ti_msp_dl_config.h"

extern void *Thread(void *arg0);

/* Stack size in bytes */
#define THREADSTACKSIZE 1024

//set up ADC & Sensor Reading Module


//set up Timer & Periodic Sampling


//set up Processing, Threshold Logic & Alert Detection


//set up LED Control + Button Handling


//set up UART controller 
//UART clock configuration
static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};
//UART communication parameters
static const DL_UART_Main_Config gUART_0Config = {
    .mode = DL_UART_MAIN_MODE_NORMAL,
    .direction = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity = DL_UART_MAIN_PARITY_NONE,
    .wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits = DL_UART_MAIN_STOP_BITS_ONE
};


/* Set up the hardware ready to run this demo */
static void prvSetupHardware(void) {
    SYSCFG_DL_init();
};

//task function: ADC & Sensor Reading Module


//task function: Timer & Periodic Sampling


//task function: Processing, Threshold Logic & Alert Detection


//task function: LED Control + Button Handling


//task function: UART Communication
void *echoUARTTask (void *arg0) {
    //initalize UART hardware
    DL_UART_Main_reset(UART0);
    DL_UART_Main_enablePower(UART0);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_UART_Main_setClockConfig(UART0, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);      //choose clock signal to be used on the controller
    DL_UART_Main_init(UART0, (DL_UART_Main_Config *) &gUART_0Config);       //sets communication parameters
    
    //determine baud rate to be used
    DL_UART_Main_setOversampling(UART0, DL_UART_OVERSAMPLING_RATE_16X);     
    DL_UART_Main_setBaudRateDivisor(UART0, 17, 23);
    
    DL_UART_Main_enable(UART0);     //enable UART

    //pins configuration
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM21, IOMUX_PINCM21_PF_UART0_TX);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM22, IOMUX_PINCM22_PF_UART0_RX);

    while (1) {
        if (!DL_UART_isRXFIFOEmpty(UART0)) {     //if UART received something in RX
            char c;
            c = DL_UART_Main_receiveData(UART0);     //read the received character   
            while (DL_UART_isTXFIFOFull(UART0));    //wait until TX is ready
            DL_UART_Main_transmitData(UART0, c);    //echo  
        }
    }
    return NULL;
}


int main(void)
{
    pthread_t thread_echoUART;
    pthread_attr_t attrs;
    struct sched_param priParam;
    int retc;

    /* Initialize the system locks */
#ifdef __ICCARM__
    __iar_Initlocks();
#endif

    /* Prepare the hardware to run this demo. */
    prvSetupHardware();

    /* Initialize the attributes structure with default values */
    pthread_attr_init(&attrs);

    /* Set priority, detach state, and stack size attributes */
    priParam.sched_priority = 1;
    retc                    = pthread_attr_setschedparam(&attrs, &priParam);
    retc |= pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
    if (retc != 0) {
        /* failed to set attributes */
        printf("Failed to set thread attributes\n");
        while (1) {
        }
    }

    retc = pthread_create(&thread_echoUART, &attrs, echoUARTTask, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        printf("Falied to create echo UART task\n");
        while (1) {
        }
    }

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    return (0);
}

