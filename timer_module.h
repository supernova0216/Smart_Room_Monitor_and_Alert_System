#ifndef TIMER_MODULE_H
#define TIMER_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "ti_msp_dl_config.h"
#include "ti/driverlib/driverlib.h"
#include <FreeRTOS.h>
#include <task.h>
#include "system.h"

// Sampling interval in milliseconds — change here to adjust rate
#define SAMPLE_INTERVAL_MS  500

// Thread stack size (matches rest of project)
#define THREADSTACKSIZE     1024

/**
 * timerTask() — FreeRTOS/POSIX thread function.
 * Sets systemValue.sampleNow = true every SAMPLE_INTERVAL_MS ms.
 * Does NOT touch any hardware — timing only.
 *
 * Usage in main():
 *   pthread_create(&thread, &attrs, timerTask, NULL);
 */
void *timerTask(void *arg0);

/**
 * timer_clearFlag() — call this after consuming a sample.
 * Call immediately after checking systemValue.sampleNow == true,
 * before doing any sensor reads, so the flag doesn't fire twice.
 */
static inline void timer_clearFlag(void)
{
    systemValue.sampleNow = false;
}

#endif /* TIMER_MODULE_H */
