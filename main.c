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
#include <semaphore.h>      //POSIX
#include <stdlib.h>
#include "system.h"         //shared set up SystemState
#include "adc_sensor.h"
#include "timer_module.h"
#include <stdarg.h>         //UART_print
#include <timer_module.h>   

/* TI includes for driver configuration */
#include "ti_msp_dl_config.h"

extern void *Thread(void *arg0);

/* Stack size in bytes */
#define THREADSTACKSIZE 1024

/*set up ADC & Sensor Reading Module
float readTemperatureC(void);
float readLightPercent(void);
void *adcTask(void *arg);
*/


/*set up Timer & Periodic Sampling
extern volatile bool sampleNow;
void *timerTask(void *arg);
void timer_clearFlag(void);
*/


/* Processing test function */
void testProcessing() {

    float test_temp = 0;
    float test_light = 500;
    for (int i = 0; i < 20; i++) {
        test_temp += (float) (i * 5);
        int status = processSensors(&THRESH, test_temp, test_light);
        printf("Temp Status: ");
        printStatus(status);
        printf("\n");
        delay_cycles(32000000 / 2);
    }

    test_temp = 25;
    test_light = 0;
    for (int i = 0; i < 30; i++) {
        test_light += (float) (i * 1000);
        int status = processSensors(&THRESH, test_temp, test_light);
        printf("Light Status: ");
        printStatus(status);
        printf("\n");
        delay_cycles(32000000 / 2);
    }
    
    test_temp = 0;
    test_light = 0;
    for (int i = 0; i < 30; i++) {
        test_temp += (float) (i * 5);
        test_light += (float) (i * 1000);
        int status = processSensors(&THRESH, test_temp, test_light);
        printf("Overall Status: ");
        printStatus(status);
        printf("\n");
        delay_cycles(32000000 / 2);
    }
}


/*set up LED Control + Button Handling
void updateLEDs(SystemStatus status);
void *ledButtonTask(void *arg);
*/


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
//UART helper funcs
void UART_sendChar (char c) {
    while (DL_UART_isTXFIFOFull(UART0));    //wait until TX is ready
    DL_UART_Main_transmitData(UART0, c);    //echo  
}
void UART_sendString (char *str) {
    int i = 0;
    while (str[i] != '\0'){
        UART_sendChar(str[i]);  //wait til TX buffer not full and send one character to UART
        i++; 
    }
}
void UART_print(char *msg, ...) {
    char buffer[128];       
    va_list args;       //holds var passed arguments
    va_start(args, msg);        //initialize args to point to first variable argument
    vsnprintf(buffer, sizeof(buffer), msg, args);       //format the string into buffer, preventing overflow
    va_end(args);       //cleanup handling
    UART_sendString(buffer);
}

/* Set up the hardware ready to run this demo */
static void prvSetupHardware(void) {
    SYSCFG_DL_init();

    // Init temp thresh with Celsius. Change to F variants if Farenheit is desired.
    initThresholds(&THRESH, TEMP_LOW_C, TEMP_HIGH_C, LIGHT_LOW_L, LIGHT_HIGH_L);
};

//task function: ADC & Sensor Reading Module


//task function: Processing, Threshold Logic & Alert Detection
struct processing_config_t {
    SystemState* state;
    THRESHOLDS* thresh;
};

void* process_temp_light(void* args) {

    struct processing_config_t* config = (struct processing_config_t*) args;
    SystemState* n_state = config->state;
    SystemStatus p_status;

    while(1) {
        // Wait for state temp and/or light to change
        // sem_wait(&state_semaphore) // Change "state_semaphore" to name of actual state semaphore being used.

        // Get new state then post change to state semaphore if status has changed
        p_status = n_state->status;
        n_state->status = processSensors(config->thresh, n_state->temperature, n_state->light);
        // if (p_status != n_state->status) sem_post(&state_semaphore);

        // Debug
        // printf("Old status: %d | New status: %d\n", p_status, n_state->status);

        usleep(100000); // Delay 0.1s to allow state change to be read by other tasks.
    }
}

//task function: LED Control + Button Handling


//task function: UART Communication
void *UARTTask(void *arg0) {
    /* initialize UART hardware */
    DL_UART_Main_reset(UART0);
    DL_UART_Main_enablePower(UART0);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_UART_Main_setClockConfig(UART0, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);
    DL_UART_Main_init(UART0, (DL_UART_Main_Config *) &gUART_0Config);

    /* determine baud rate */
    DL_UART_Main_setOversampling(UART0, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART0, 17, 23);

    DL_UART_Main_enable(UART0);

    /* pins configuration */
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM21, IOMUX_PINCM21_PF_UART0_TX);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM22, IOMUX_PINCM22_PF_UART0_RX);

    UART_sendString("System Started\r\n");

    ADC_Sensor_init();
    UART_sendString("ADC Sensor Module Initialized\r\n");

    while (1) {
        if (sampleNow) {
            uint16_t tempRaw;
            uint16_t lightRaw;
            int temp_int, temp_decimal;
            int light_int, light_decimal;

            timer_clearFlag();

            tempRaw = readTemperatureRaw();
            lightRaw = readLightRaw();

            systemValue.temperature = convertTemperatureRawToC(tempRaw);
            systemValue.light = convertLightRawToPercent(lightRaw);

            /* Temporary status for module testing only */
            systemValue.status = NORMAL;

            temp_int = (int) systemValue.temperature;
            temp_decimal = (int) ((systemValue.temperature - temp_int) * 100.0f);
            if (temp_decimal < 0) {
                temp_decimal = -temp_decimal;
            }

            light_int = (int) systemValue.light;
            light_decimal = (int) ((systemValue.light - light_int) * 100.0f);
            if (light_decimal < 0) {
                light_decimal = -light_decimal;
            }

            UART_print(
                "TempRaw: %u | Temp: %d.%02d C | LightRaw: %u | Light: %d.%02d %% | Status: %d\r\n",
                tempRaw,
                temp_int, temp_decimal,
                lightRaw,
                light_int, light_decimal,
                systemValue.status
            );
        }

        vTaskDelay(pdMS_TO_TICKS(50));
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
    //create UART task 
    retc = pthread_create(&thread_UART, &attrs, UARTTask, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        printf("Failed to create UART task\n");
        while (1) {
        }
    }

    // create Timer task
    retc = pthread_create(&thread_Timer, &attrs, timerTask, NULL);
    if (retc != 0) {
        printf("Failed to create Timer task\n");
        while (1) {
        }
    }

    //create Timer task
    retc = pthread_create(&thread_Timer, &attrs, timerTask, NULL);
    if (retc != 0) {
        printf("Failed to create Timer task\n");
        while (1) {
        }
    }


    //adcTask
    //processingTask
    struct processing_config_t processing_config = {
        .state = &systemValue,
        .thresh = &THRESH
    };
    retc = pthread_create(&thread_processing, &attrs, process_temp_light, &processing_config);
    if (retc != 0) {
        printf("Failed to create processing task\n");
        while (1) {
            
        }
    }
    //LEDTask


    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    return (0);
}