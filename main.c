/*
 *  CMPE-146 Semester
 */

/* Standard includes */
#include <stdio.h>
#include <string.h>

/* POSIX header files */
#include <pthread.h>

/* RTOS header files */
#include <FreeRTOS.h>
#include <task.h>
#include <semaphore.h>      //POSIX

#include <unistd.h>
#include <stdlib.h>
#include <system.h>         //shared set up SystemState
#include <stdarg.h>         //UART_print

#include "timer_module.h"  
#include "led_button.h"
#include "adc_sensor.h"
#include "UART_UI.h"

/* TI includes for driver configuration */
#include "ti_msp_dl_config.h"

extern void *Thread(void *arg0);

/* Stack size in bytes */
#define THREADSTACKSIZE        1024
#define UART_STACK_SIZE        4096
#define TIMER_STACK_SIZE       1024
#define PROCESSING_STACK_SIZE  2048

struct processing_config_t {
    SystemState* state;
    THRESHOLDS* thresh;
};

// Global Semaphores
sem_t temp_request_sem;   // UART -> processing
sem_t temp_done_sem;      // processing -> UART
sem_t state_mutex;        // shared systemValue protection

volatile bool s2Pressed = false;
volatile bool s2Released = false;

QueueHandle_t buttonQueue;

volatile MenuOption currentMenu = MENU_UPDATE_THRESH;
volatile MenuEvent menuEvent = MENU_EVENT_NONE;

volatile LightOption currentLightMenu = LIGHT_NORMAL;
volatile bool inLightMenu = false;

volatile bool inThresholdMenu = false;
volatile bool editingThreshold = false;
volatile ThresholdMenuOptions currentThresholdMenu = THRESH_MENU_TEMP_LOW;

char thresholdInputBuf[16];
uint8_t thresholdInputIndex = 0;

AlertLogEntry alertLog[ALERT_LOG_SIZE];

volatile uint8_t alertLogHead = 0;
volatile uint8_t alertLogCount = 0;
volatile uint32_t alertSampleCounter = 0;

volatile bool inAlertLogMenu = false;


/* Page/Event Handler Prototypes */
static MenuEvent get_button_event();
static void handle_main_menu(MenuEvent event);
static void handle_threshold_menu(MenuEvent event);
static void handle_light_menu(MenuEvent event);
static void handle_alert_log_menu(MenuEvent event);

static void handle_threshold_input();
static void append_alert_log(float temp, float light, SystemStatus status);


/* Set up the hardware ready to run this demo */
static void prvSetupHardware(void) {
    SYSCFG_DL_init();

    init_UART();
    LED_Button_Init();
    ADC_Sensor_init();
    
    // Init temp thresh with Celsius. Change to F variants if Farenheit is desired.
    initThresholds(&THRESH, TEMP_LOW_C, TEMP_HIGH_C, LIGHT_LOW_L, LIGHT_HIGH_L);
};

// Handle Sample Timing
void* timerTask(void* arg0) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));   // sample every 1 second

        sem_wait(&state_mutex);
        systemValue.sampleNow = true;
        sem_post(&state_mutex);

        // Request processing directly
        sem_post(&temp_request_sem);
    }

    return NULL;
}

// Processing, Threshold Logic & Alert Detection
void* process_temp(void* args) {
    struct processing_config_t* config = (struct processing_config_t*) args;
    SystemState* n_state = config->state;

    while (1) {
        // Wait until UART says a sample should be processed
        sem_wait(&temp_request_sem);

        sem_wait(&state_mutex);

        n_state->sampleNow = false;

        float tempC = readTemperatureC();
        n_state->temperature = tempC;

        n_state->status = processSensors(
            config->thresh,
            n_state->temperature,
            n_state->light
        );

        append_alert_log(
            n_state->temperature,
            n_state->light,
            n_state->status
        );

        updateLEDs(n_state->status, getMode());

        sem_post(&state_mutex);

        // Tell UART processing is complete
        sem_post(&temp_done_sem);
    }

    return NULL;
}

//task function: UART Communication
void *UARTTask (void *arg0) {

    run_status_test();

    bool started = false;

    while (1) {

        if (!started) {
            UART_print("\r\nSYSTEM STARTED\r\n");
            show_main_menu();
            started = true;
        }

        MenuEvent event = get_button_event();

        /* Alert page first */
        if (inAlertLogMenu) {
            handle_alert_log_menu(event);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* Threshold page second */
        if (inThresholdMenu) {
            handle_threshold_menu(event);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        /* Light page third */
        if (inLightMenu) {
            handle_light_menu(event);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        /* Main menu last */
        handle_main_menu(event);

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return NULL;
}


int main(void)
{

    pthread_t thread_UART, thread_Timer, thread_processing;
    pthread_attr_t attrs;
    struct sched_param priParam;
    int retc;

    /* Initialize the system locks */
#ifdef __ICCARM__
    __iar_Initlocks();
#endif

    buttonQueue = xQueueCreate(16, sizeof(ButtonEvent));
    if (buttonQueue == NULL) {
        printf("Button Queue failed to create.\n");
        while (1);
    }

    /* Init semaphores */
    sem_init(&temp_request_sem, 0, 0);
    sem_init(&temp_done_sem, 0, 0);
    sem_init(&state_mutex, 0, 1);

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
        printf("Failed to set thread attributes. Code: %d\n", retc);
        while (1) {
        }
    }
    
    //create UART task 
    pthread_attr_setstacksize(&attrs, UART_STACK_SIZE);
    retc = pthread_create(&thread_UART, &attrs, UARTTask, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        printf("Falied to create UART task. Code: %d\n", retc);
        while (1) {
        }
    }

    //create Timer task
    pthread_attr_setstacksize(&attrs, TIMER_STACK_SIZE);
    retc = pthread_create(&thread_Timer, &attrs, timerTask, NULL);
    if (retc != 0) {
        printf("Failed to create Timer task. Code: %d\n", retc);
        while (1) {
        }
    }

    //processingTask
    pthread_attr_setstacksize(&attrs, PROCESSING_STACK_SIZE);
    static struct processing_config_t processing_config = {
        .state = &systemValue,
        .thresh = &THRESH
    };
    retc = pthread_create(&thread_processing, &attrs, process_temp, &processing_config);
    if (retc != 0) {
        printf("Failed to create processing task. Code: %d\n", retc);
        while (1) {
        }
    }

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    return (0);
}


/* Event Handlers */

static MenuEvent get_button_event() {
    static TickType_t s2PressTick = 0;
    static bool s2PressedValid = false;

    ButtonEvent btnEvent;

    if (xQueueReceive(buttonQueue, &btnEvent, 0) != pdTRUE) {
        return MENU_EVENT_NONE;
    }

    switch (btnEvent) {
        case BTN_EVENT_S1_PRESS:
            return MENU_EVENT_UP;

        case BTN_EVENT_S2_PRESS:
            s2PressTick = xTaskGetTickCount();
            s2PressedValid = true;
            return MENU_EVENT_NONE;

        case BTN_EVENT_S2_RELEASE:
            if (!s2PressedValid) {
                return MENU_EVENT_NONE;
            }

            s2PressedValid = false;

            if ((xTaskGetTickCount() - s2PressTick) >= pdMS_TO_TICKS(S2_HOLD_MS)) {
                return MENU_EVENT_SELECT;
            }

            return MENU_EVENT_DOWN;

        default:
            return MENU_EVENT_NONE;
    }
}

static void handle_main_menu(MenuEvent event) {
    if (event == MENU_EVENT_UP) {
        currentMenu = (currentMenu == 0) ? MENU_COUNT - 1 : currentMenu - 1;
        show_main_menu();
    }
    else if (event == MENU_EVENT_DOWN) {
        currentMenu = (currentMenu + 1) % MENU_COUNT;
        show_main_menu();
    }
    else if (event == MENU_EVENT_SELECT) {
        switch (currentMenu) {
            case MENU_UPDATE_THRESH:
                xQueueReset(buttonQueue);
                inThresholdMenu = true;
                editingThreshold = false;
                currentThresholdMenu = THRESH_MENU_TEMP_LOW;
                show_threshold_menu();
                break;

            case MENU_CHANGE_LIGHT:
                inLightMenu = true;
                show_light_menu();
                break;

            case MENU_VIEW_ALERT_LOG:
                inAlertLogMenu = true;
                show_alert_log_menu();
                break;

            default:
                break;
        }
    }
}

static void handle_threshold_menu(MenuEvent event)
{
    if (editingThreshold) {
        handle_threshold_input();

        if (event == MENU_EVENT_SELECT) {
            editingThreshold = false;
            thresholdInputIndex = 0;
            thresholdInputBuf[0] = '\0';
            show_threshold_menu();
        }

        return;
    }

    if (!DL_UART_isRXFIFOEmpty(UART0)) {
        char c = (char)DL_UART_Main_receiveData(UART0);

        if (c == 'b' || c == 'B') {
            inThresholdMenu = false;
            editingThreshold = false;
            show_main_menu();
            return;
        }
    }

    if (event == MENU_EVENT_UP) {
        currentThresholdMenu =
            (currentThresholdMenu == 0)
            ? THRESH_MENU_COUNT - 1
            : currentThresholdMenu - 1;

        show_threshold_menu();
    }
    else if (event == MENU_EVENT_DOWN) {
        currentThresholdMenu =
            (currentThresholdMenu + 1) % THRESH_MENU_COUNT;

        show_threshold_menu();
    }
    else if (event == MENU_EVENT_SELECT) {
        if (currentThresholdMenu == THRESH_MENU_BACK) {
            inThresholdMenu = false;
            editingThreshold = false;
            show_main_menu();
            return;
        }

        begin_threshold_edit();
    }
}

static void handle_light_menu(MenuEvent event) {
    if (event == MENU_EVENT_UP) {
        currentLightMenu =
            (currentLightMenu == 0)
            ? LIGHT_COUNT - 1
            : currentLightMenu - 1;

        show_light_menu();
    }
    else if (event == MENU_EVENT_DOWN) {
        currentLightMenu = (currentLightMenu + 1) % LIGHT_COUNT;
        show_light_menu();
    }
    else if (event == MENU_EVENT_SELECT) {
        sem_wait(&state_mutex);

        switch (currentLightMenu) {
            case LIGHT_DARK:   systemValue.light = 10.0; break;
            case LIGHT_DIM:    systemValue.light = 40.0; break;
            case LIGHT_NORMAL: systemValue.light = 60.0; break;
            case LIGHT_BRIGHT: systemValue.light = 90.0; break;
            default:           systemValue.light = 60.0; break;
        }

        sem_post(&state_mutex);

        inLightMenu = false;
        UART_print("\r\nLight level updated.\r\n");
        show_main_menu();
    }
}

static void handle_alert_log_menu(MenuEvent event) {
    if (sem_trywait(&temp_done_sem) == 0) {
        show_alert_log_menu();
    }

    if (event == MENU_EVENT_SELECT) {
        inAlertLogMenu = false;
        show_main_menu();
        return;
    }

    if (event == MENU_EVENT_UP || event == MENU_EVENT_DOWN) {
        show_alert_log_menu();
    }
}

static void handle_threshold_input() {
    if (!DL_UART_isRXFIFOEmpty(UART0)) {
        char c = (char)DL_UART_Main_receiveData(UART0);

        // Enter: commit value
        if (c == '\r' || c == '\n') {
            thresholdInputBuf[thresholdInputIndex] = '\0';

            float newVal = atof(thresholdInputBuf);

            if (currentThresholdMenu == THRESH_MENU_TEMP_LOW) {
                updateThreshold(&THRESH, THRESHOLD_TEMP_LOW, newVal);
            }
            else if (currentThresholdMenu == THRESH_MENU_TEMP_HIGH) {
                updateThreshold(&THRESH, THRESHOLD_TEMP_HIGH, newVal);
            }

            editingThreshold = false;
            thresholdInputIndex = 0;
            thresholdInputBuf[0] = '\0';

            show_threshold_menu();
        }

        // Backspace/delete
        else if ((c == '\b' || c == 127) && thresholdInputIndex > 0) {
            thresholdInputIndex--;
            thresholdInputBuf[thresholdInputIndex] = '\0';

            UART_print("\b \b");
        }

        // Valid numeric input
        else if (((c >= '0' && c <= '9') || c == '.') &&
                 thresholdInputIndex < sizeof(thresholdInputBuf) - 1) {
            thresholdInputBuf[thresholdInputIndex++] = c;
            thresholdInputBuf[thresholdInputIndex] = '\0';

            UART_print("%c", c);
        }
    }
}

static void append_alert_log(float temp, float light, SystemStatus status) {
    alertLog[alertLogHead].temperature = temp;
    alertLog[alertLogHead].light = light;
    alertLog[alertLogHead].status = status;
    alertLog[alertLogHead].sampleNumber = ++alertSampleCounter;

    alertLogHead = (alertLogHead + 1) % ALERT_LOG_SIZE;

    if (alertLogCount < ALERT_LOG_SIZE) {
        alertLogCount++;
    }
}