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
#include <system.h>         //shared set up SystemState
#include <stdarg.h>         //UART_print
#include "timer_module.h"  
#include "led_button.h"
#include "adc_sensor.h"


/* TI includes for driver configuration */
#include "ti_msp_dl_config.h"

extern void *Thread(void *arg0);

/* Stack size in bytes */
#define THREADSTACKSIZE 1024
//#define configTOTAL_HEAP_SIZE (12 * 1024)

struct processing_config_t {
    SystemState* state;
    THRESHOLDS* thresh;
};

/*set up ADC & Sensor Reading Module
float readTemperatureC(void);
float readLightPercent(void);
void *adcTask(void *arg);
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

    ADC_Sensor_init();
    // Init temp thresh with Celsius. Change to F variants if Farenheit is desired.
    initThresholds(&THRESH, TEMP_LOW_C, TEMP_HIGH_C, LIGHT_LOW_L, LIGHT_HIGH_L);
};

//task function: ADC & Sensor Reading Module


//task function: Processing, Threshold Logic & Alert Detection
void *process_temp_light(void* args) {

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

//task function: UART Communication
void *UARTTask (void *arg0) {
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

    //string
    UART_sendString("System Started\r\n");

    //LED
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

    updateLEDs(NORMAL, MODE_THRESHOLD_VIEW);
    UART_sendString("BLUE - Config Mode\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    UART_sendString("LED test done! Now testing buttons...\r\n");

    char rxBuf[16];
    int rxIndex = 0;
    bool started = false;

    while (1) {
        //for testing purposes - delete once all tasks combined
        //systemValue.temperature += 0.5;
        //systemValue.light += 10.0;
        //systemValue.status = NORMAL;
        //systemValue.sampleNow = true; //timer trigger

        
        // Wait for START command
        if (!started) {
            if (!DL_UART_isRXFIFOEmpty(UART0)) {
                char c = DL_UART_Main_receiveData(UART0);   //read the received character  
                while (DL_UART_isTXFIFOFull(UART0));
                DL_UART_Main_transmitData(UART0, c);    //echo; send the same character back 
                
                // end of command
                if (c == '\r' || c == '\n') {
                    rxBuf[rxIndex] = '\0';
                    if (strcmp(rxBuf, "start") == 0) {
                        UART_print("\r\nSYSTEM STARTED\r\n");
                        started = true;
                    } else {
                        UART_print("\r\nType 'start' only\r\n");
                    }
                    rxIndex = 0;
                }
                else if (rxIndex < sizeof(rxBuf) - 1) {
                    rxBuf[rxIndex++] = c;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        //system runs
        // Debug: read raw button states
        bool s1 = DL_GPIO_readPins(GPIOA, DL_GPIO_PIN_18) != 0;
        bool s2 = DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21) != 0;
        UART_print("S1=%d S2=%d Mode=%d\r\n", s1, s2, getMode());
        handleButtons();

        //only print when new sample is ready
        if(systemValue.sampleNow) {
            systemValue.sampleNow = false;
            UART_print("Timer tick\r\n");       //testing timer - remove once finished

            //read sensor
            /*uint16_t tempRaw = readTemperatureRaw();
            uint16_t lightRaw = readLightRaw();
            systemValue.temperature = convertTemperatureRawToC(tempRaw);            
            systemValue.light = convertLightRawToPercent(lightRaw);*/
            float tempC = readTemperatureC();
            systemValue.temperature = tempC;

            
            //for testing threshold 
            //systemValue.temperature += 0.5;
            //systemValue.light += 10.0;

            //process logic
            systemValue.status = processSensors(&THRESH,systemValue.temperature, systemValue.light);

            //update LEDs
            //updateLEDs(systemValue.status);
            //systemValue.status = NORMAL;            //testing convert adc 
            updateLEDs(systemValue.status, getMode());

            //alternative to print float since UART can only send int
            int temp_int =  (int) systemValue.temperature;
            int temp_decimal = (int) ((systemValue.temperature - temp_int) * 100);
            if (temp_decimal < 0) {
                temp_decimal = -temp_decimal;
            }

            int light_int = (int) systemValue.light;
            int light_decimal = (int) ((systemValue.light - light_int) * 100);
            if (light_decimal < 0) {
                light_decimal = -light_decimal;
            }
            
            //UART_print("Test int: %d\r\n", 123);      //test to check UART_print
            //UART_print("Temperature: %d.%02d C  (Raw:%u) | Light: %d.%02d %%  (Raw:%u) | Status: %d\r\n",
            //            temp_int, temp_decimal, tempRaw, light_int, light_decimal, lightRaw, systemValue.status);
            UART_print("Temperature: %d.%02d C  | Light: %d.%02d %%  | Status: %d\r\n",
                        temp_int, temp_decimal, light_int, light_decimal, systemValue.status);
        }
        
        vTaskDelay(pdMS_TO_TICKS(900));     //prevent CPU hogging, printing speed
    }
    return NULL;
}


int main(void)
{
    // // Threshold Testing
    // prvSetupHardware();
    // testProcessing();
    // while(1);

    pthread_t thread_UART, thread_Timer, thread_processing;
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
    pthread_attr_setstacksize(&attrs, 1024);
    retc = pthread_create(&thread_UART, &attrs, UARTTask, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        printf("Falied to create UART task\n");
        while (1) {
        }
    }

    //create Timer task
    pthread_attr_setstacksize(&attrs, 512);
    retc = pthread_create(&thread_Timer, &attrs, timerTask, NULL);
    if (retc != 0) {
        printf("Failed to create Timer task\n");
        while (1) {
        }
    }


    //adcTask
    //processingTask
    pthread_attr_setstacksize(&attrs, 512);
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

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    return (0);
}