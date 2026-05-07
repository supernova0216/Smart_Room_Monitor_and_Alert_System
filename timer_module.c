#include "timer_module.h"
#include <unistd.h>   /* usleep() */
#include <stdbool.h>


volatile bool sampleNow = false;

//  Timer task
//  This will run as its own FreeRTOS thread.
//  Sets sampleNow = true every SAMPLE_INTERVAL_MS ms.
//  Uses usleep() — no hardware timer, no delay loops.
//  Has zero knowledge of sensors, LEDs, or UART.
void *timerTask(void *arg0)
{
    // Convert ms -> microseconds for usleep()
    const uint32_t interval_us = SAMPLE_INTERVAL_MS * 1000;

    while (1)
    {
        // Wait one full sample interval
        usleep(interval_us);

        // Signal that a sample should be taken
        // Only set — never cleared here. The consumer clears it.
        sampleNow = true;
    }

    return NULL;  // Never reached
}
