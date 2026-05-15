#pragma once

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>

#include "system.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "ti_msp_dl_config.h"

#define ALERT_LOG_SIZE 10
#define STATUS_TEST_DELAY_US 250000

/* UART Menu Index Enums */
typedef enum {
    MENU_UPDATE_THRESH = 0,
    MENU_CHANGE_LIGHT,
    MENU_VIEW_ALERT_LOG,
    MENU_COUNT
} MenuOption;

typedef enum {
    MENU_EVENT_NONE = 0,
    MENU_EVENT_UP,
    MENU_EVENT_DOWN,
    MENU_EVENT_SELECT
} MenuEvent;

typedef enum {
    LIGHT_DARK = 0,
    LIGHT_DIM,
    LIGHT_NORMAL,
    LIGHT_BRIGHT,
    LIGHT_COUNT
} LightOption;

typedef enum {
    THRESH_MENU_TEMP_LOW = 0,
    THRESH_MENU_TEMP_HIGH,
    THRESH_MENU_BACK,
    THRESH_MENU_COUNT
} ThresholdMenuOptions;

typedef enum {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_S1_PRESS,
    BTN_EVENT_S2_PRESS,
    BTN_EVENT_S2_RELEASE
} ButtonEvent;

typedef struct {
    float temperature;
    float light;
    SystemStatus status;
    uint32_t sampleNumber;
} AlertLogEntry;

extern sem_t menu_sem;

extern volatile bool s2Pressed;
extern volatile bool s2Released;

extern QueueHandle_t menuQueue;
extern QueueHandle_t buttonQueue;

extern volatile MenuOption currentMenu;
extern volatile MenuEvent menuEvent;

extern volatile LightOption currentLightMenu;
extern volatile bool inLightMenu;

extern volatile bool inThresholdMenu;
extern volatile bool editingThreshold;
extern volatile ThresholdMenuOptions currentThresholdMenu;

extern char thresholdInputBuf[16];
extern uint8_t thresholdInputIndex;

extern AlertLogEntry alertLog[ALERT_LOG_SIZE];
extern volatile uint8_t alertLogHead;
extern volatile uint8_t alertLogCount;
extern volatile uint32_t alertSampleCounter;
extern volatile bool inAlertLogMenu;


/* UART helper funtions */
void UART_sendChar(char c);

void UART_sendString(char* str);

/* UART public functions */

void init_UART();

void UART_print(char* msg, ...);

void run_status_test();

/* UART UI Functions */

void clear_screen();

void show_main_menu();

void show_threshold_menu();

void begin_threshold_edit();

void show_light_menu();

void show_alert_log_menu();