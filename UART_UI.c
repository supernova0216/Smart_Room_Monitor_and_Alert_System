#include "UART_UI.h"
#include "led_button.h"
#include "threshold.h"

/* Config Structures */

// UART Clock Configuration
static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

//UART Communication Parameters
static const DL_UART_Main_Config gUART_0Config = {
    .mode = DL_UART_MAIN_MODE_NORMAL,
    .direction = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity = DL_UART_MAIN_PARITY_NONE,
    .wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits = DL_UART_MAIN_STOP_BITS_ONE
};

/* Definitions */

void init_UART() {

    // Initalize UART Hardware
    DL_UART_Main_reset(UART0);
    DL_UART_Main_enablePower(UART0);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_UART_Main_setClockConfig(UART0, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);       //choose clock signal to be used on the controller
    DL_UART_Main_init(UART0, (DL_UART_Main_Config *) &gUART_0Config);                           //sets communication parameters
    
    // Determine Baud Rate
    DL_UART_Main_setOversampling(UART0, DL_UART_OVERSAMPLING_RATE_16X);     
    DL_UART_Main_setBaudRateDivisor(UART0, 17, 23);
    
    DL_UART_Main_enable(UART0);                                                                 //enable UART

    // Pin Configuration
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM21, IOMUX_PINCM21_PF_UART0_TX);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM22, IOMUX_PINCM22_PF_UART0_RX);
}

void UART_sendChar (char c) {
    while (DL_UART_isTXFIFOFull(UART0));            //wait until TX is ready
    DL_UART_Main_transmitData(UART0, c);            //echo  
}

void UART_sendString (char *str) {
    int i = 0;
    while (str[i] != '\0'){
        UART_sendChar(str[i]);                      //wait til TX buffer not full and send one character to UART
        i++; 
    }
}

void run_status_test() {
    clear_screen();
    UART_print("Testing LEDs...\r\n");

    updateLEDs(NORMAL, MODE_MONITOR);
    UART_print("GREEN - Normal\r\n");
    usleep(STATUS_TEST_DELAY_US);

    updateLEDs(TEMP_HIGH, MODE_MONITOR);
    UART_print("RED - Alert High Temp\r\n");
    usleep(STATUS_TEST_DELAY_US);

    updateLEDs(TEMP_LOW, MODE_MONITOR);
    UART_print("BLUE - Alert Low Temp\r\n");
    usleep(STATUS_TEST_DELAY_US);

    updateLEDs(LIGHT_HIGH, MODE_MONITOR);
    UART_print("YELLOW - Alert Bright Light\r\n");
    usleep(STATUS_TEST_DELAY_US);

    updateLEDs(LIGHT_LOW, MODE_MONITOR);
    UART_print("CYAN - Alert Low Light\r\n");
    usleep(STATUS_TEST_DELAY_US);

    updateLEDs(MULTIPLE_ALERT, MODE_MONITOR);
    UART_print("PURPLE - Multiple Alert\r\n");
    usleep(STATUS_TEST_DELAY_US);

    UART_print("LED test done!\r\n");
    sleep(1);
}

void UART_print(char *msg, ...) {
    static char buffer[128];       
    va_list args;                                   //holds var passed arguments
    va_start(args, msg);                            //initialize args to point to first variable argument
    vsnprintf(buffer, sizeof(buffer), msg, args);   //format the string into buffer, preventing overflow
    va_end(args);                                   //cleanup handling
    UART_sendString(buffer);
}


void clear_screen() {
    UART_print("\x1b[2J\x1b[H");
}

void show_main_menu() {
    clear_screen();
    UART_print("\r\n========== MAIN MENU ==========\r\n");

    UART_print("%c Update threshold values\r\n",
        currentMenu == MENU_UPDATE_THRESH ? '>' : ' ');

    UART_print("%c Change lighting level\r\n",
        currentMenu == MENU_CHANGE_LIGHT ? '>' : ' ');

    UART_print("%c View alert log\r\n",
        currentMenu == MENU_VIEW_ALERT_LOG ? '>' : ' ');

    UART_print("\r\nS1 = Up | S2 = Down | Hold S2 = Select\r\n");
}

static void print_float_2(float val)
{
    int whole = (int) val;
    int frac = (int) ((val - whole) * 100);

    if (frac < 0) {
        frac = -frac;
    }

    UART_print("%d.%02d", whole, frac);
}

void show_threshold_menu(void)
{
    clear_screen();

    UART_print("\r\n=============== THRESHOLD MENU ================\r\n");

    UART_print("%c Low Temp Threshold:  ",
        currentThresholdMenu == THRESH_MENU_TEMP_LOW ? '>' : ' ');
    print_float_2(THRESH.temp_low);
    UART_print(" C\r\n");

    UART_print("%c High Temp Threshold: ",
        currentThresholdMenu == THRESH_MENU_TEMP_HIGH ? '>' : ' ');
    print_float_2(THRESH.temp_high);
    UART_print(" C\r\n");

    UART_print("%c Back\r\n",
        currentThresholdMenu == THRESH_MENU_BACK ? '>' : ' ');

    UART_print("\r\nS1 = Up | S2 = Down | Hold S2 = Edit | b = Back\r\n");
}

void begin_threshold_edit(void)
{
    editingThreshold = true;
    thresholdInputIndex = 0;
    thresholdInputBuf[0] = '\0';

    if (currentThresholdMenu == THRESH_MENU_TEMP_LOW) {
        UART_print("\033[3;24H");      // row 3, column 24
        UART_print("          ");      // clear old value
        UART_print("\033[3;24H");      // return to input spot
    }
    else {
        UART_print("\033[4;24H");      // row 4, column 24
        UART_print("          ");
        UART_print("\033[4;24H");
    }
}

void show_light_menu() {
    clear_screen();

    UART_print("\r\n========== LIGHT MENU ==========\r\n");

    UART_print("%c Dark\r\n",
        currentLightMenu == LIGHT_DARK ? '>' : ' ');

    UART_print("%c Dim\r\n",
        currentLightMenu == LIGHT_DIM ? '>' : ' ');

    UART_print("%c Normal\r\n",
        currentLightMenu == LIGHT_NORMAL ? '>' : ' ');

    UART_print("%c Bright\r\n",
        currentLightMenu == LIGHT_BRIGHT ? '>' : ' ');

    UART_print("\r\nS1 = Up | S2 = Down | Hold S2 = Select\r\n");
}

void show_alert_log_menu() {
    clear_screen();

    UART_print("\r\n========================= ALERT LOG =========================\r\n");
    UART_print("Last %d readings:\r\n\r\n", alertLogCount);

    if (alertLogCount == 0) {
        UART_print("No readings logged yet.\r\n");
    } else {
        uint8_t oldest =
            (alertLogHead + ALERT_LOG_SIZE - alertLogCount) % ALERT_LOG_SIZE;

        for (uint8_t i = 0; i < alertLogCount; i++) {
            uint8_t index = (oldest + i) % ALERT_LOG_SIZE;

            UART_print("#%lu | Temp: ",
                alertLog[index].sampleNumber);

            print_float_2(alertLog[index].temperature);
            UART_print(" C | Light: ");

            print_float_2(alertLog[index].light);
            UART_print(" %% | Status: %s\r\n",
                getStatus(alertLog[index].status));
        }
    }

    UART_print("\r\nHold S2 = Back\r\n");
}