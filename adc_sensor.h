#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "ti/driverlib/driverlib.h"

/* ---------- User-adjustable defaults ---------- */

#define ADC_SENSOR_MAX_COUNTS       4096.0f   /* 12-bit ADC */

#define VREF_1V4                    (1.4f / ADC_SENSOR_MAX_COUNTS)
#define VREF_3V3                    (3.3f / ADC_SENSOR_MAX_COUNTS)
#define TYP_TSC                     (-1.8 / 1000)
#define TYP_TSTRIM                  30.0

#define TEMP_SAMPLE_TIME            (CPUCLK_FREQ * 0.00002)
#define ADC_TIMEOUT                 10000U

/*
 * Default sensor channel assumptions:
 * - Temp sensor: ADC0.11
 * - Light sensor: ADC0.7-ish
 *
 * If board/jumpers show different channels, only change these 2 lines.
 */
#define ADC_TEMP_CHANNEL            DL_ADC12_INPUT_CHAN_11
#define ADC_LIGHT_CHANNEL           DL_ADC12_INPUT_CHAN_8


/* ---------- Public API ---------- */

void ADC_Sensor_init(void);

float readTemperatureC(void);
float readLightPercent(void);

#endif /* ADC_SENSOR_H */