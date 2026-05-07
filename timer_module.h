#ifndef TIMER_MODULE_H
#define TIMER_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "ti_msp_dl_config.h"
#include "ti/driverlib/driverlib.h"

//  Sampling interval — change here to adjust rate
//  Default: 500 ms
#define SAMPLE_INTERVAL_MS  500

//  Shared flag — set by timer task, cleared by
//  whoever processes the sample
extern volatile bool sampleNow;

//  Thread stack size (matches rest of project)
#define THREADSTACKSIZE 1024

//  Public API

/**
 * timerTask() — FreeRTOS/POSIX thread function.
 *
 * Pass to pthread_create() in main().
 * Sets sampleNow = true every SAMPLE_INTERVAL_MS milliseconds.
 * Does NOT touch any hardware — timing only.
 *
 * Usage in main():
 *   pthread_create(&thread, &attrs, timerTask, NULL);
 */
void *timerTask(void *arg0);

/**
 * timer_clearFlag() — call this after you have consumed a sample.
 *
 * Whoever reads sampleNow (Person 3 or Hoang in main loop) should
 * call this immediately after starting the ADC read so the flag
 * doesn't get acted on twice.
 */
static inline void timer_clearFlag(void)
{
    sampleNow = false;
}

#endif /* TIMER_MODULE_H */
