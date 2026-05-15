#ifndef TIMER_MODULE_H
#define TIMER_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "ti_msp_dl_config.h"
#include "ti/driverlib/driverlib.h"

#include <FreeRTOS.h>
#include <task.h>
#include <system.h>

//  Sampling interval — change here to adjust rate
//  Default: 500 ms
#define SAMPLE_INTERVAL_MS  500

//  Thread stack size (matches rest of project)
#define THREADSTACKSIZE 1024

//  Public API
void *timerTask(void *arg0);

#endif /* TIMER_MODULE_H */