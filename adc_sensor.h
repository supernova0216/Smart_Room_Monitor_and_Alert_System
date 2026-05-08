#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "ti/driverlib/driverlib.h"

/* ---------- User-adjustable defaults ---------- */

/* Basic software averaging */
#define ADC_SENSOR_AVG_SAMPLES      8U

/* Assume VDDA reference = 3.3V */
#define ADC_SENSOR_VREF_VOLTS       3.3f
#define ADC_SENSOR_MAX_COUNTS       4095.0f   /* 12-bit ADC */

/*
 * Default sensor channel assumptions:
 * - Temp sensor: ADC0.5-ish
 * - Light sensor: ADC0.7-ish
 *
 * If board/jumpers show different channels, only change these 2 lines.
 */
#define ADC_TEMP_CHANNEL            DL_ADC12_INPUT_CHAN_5
#define ADC_LIGHT_CHANNEL           DL_ADC12_INPUT_CHAN_7

/* Thermistor model assumptions for simple Celsius conversion */
#define THERM_FIXED_RESISTOR_OHMS   10000.0f
#define THERM_NOMINAL_RES_OHMS      10000.0f
#define THERM_BETA                  3950.0f
#define THERM_T0_KELVIN             298.15f   /* 25 C */

/* ---------- Public API ---------- */

void ADC_Sensor_init(void);

uint16_t readTemperatureRaw(void);
uint16_t readLightRaw(void);

float convertTemperatureRawToC(uint16_t raw);
float convertLightRawToPercent(uint16_t raw);

float readTemperatureC(void);
float readLightPercent(void);

#endif /* ADC_SENSOR_H */