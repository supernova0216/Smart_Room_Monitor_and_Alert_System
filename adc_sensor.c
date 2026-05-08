#include "adc_sensor.h"
#include <math.h>
#include <stdbool.h>


#if defined(ADC12_0_INST)
    #define ROOMMON_ADC_INST ADC12_0_INST
#else
    #error "ADC instance macro not found. Check your local ti_msp_dl_config.h."
#endif

/* Use MEM0 for single-channel reads */
#define ROOMMON_ADC_MEM_INDEX   DL_ADC12_MEM_IDX_0

/* ADC clock configuration */
static const DL_ADC12_ClockConfig gADCClockConfig = {
    .clockSel    = DL_ADC12_CLOCK_SYSOSC,
    .divideRatio = DL_ADC12_CLOCK_DIVIDE_8,
    .freqRange   = DL_ADC12_CLOCK_FREQ_RANGE_20_TO_24
};

static bool gADCInitialized = false;

/* ---------- Private helpers ---------- */

static uint16_t adcReadSingleRaw(uint32_t inputChannel)
{
    uint32_t timeout = 1000000U;

    /* Reconfigure MEM0 for the requested input channel */
    DL_ADC12_disableConversions(ROOMMON_ADC_INST);

    DL_ADC12_configConversionMem(
        ROOMMON_ADC_INST,
        ROOMMON_ADC_MEM_INDEX,
        inputChannel,
        DL_ADC12_REFERENCE_VOLTAGE_VDDA,
        DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
        DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED,
        DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
        DL_ADC12_WINDOWS_COMP_MODE_DISABLED
    );

    DL_ADC12_enableConversions(ROOMMON_ADC_INST);

    /* Start one conversion */
    DL_ADC12_startConversion(ROOMMON_ADC_INST);

    /* Wait until conversion finishes */
    while (DL_ADC12_isConversionStarted(ROOMMON_ADC_INST) && timeout > 0U) {
        timeout--;
    }

    if (timeout == 0U) {
        /* Timeout fallback */
        return 0U;
    }

    return DL_ADC12_getMemResult(ROOMMON_ADC_INST, ROOMMON_ADC_MEM_INDEX);
}

static uint16_t adcReadAveragedRaw(uint32_t inputChannel)
{
    uint32_t sum = 0U;
    uint32_t i;

    for (i = 0U; i < ADC_SENSOR_AVG_SAMPLES; i++) {
        sum += adcReadSingleRaw(inputChannel);
    }

    return (uint16_t)(sum / ADC_SENSOR_AVG_SAMPLES);
}

/* ---------- Public functions ---------- */

void ADC_Sensor_init(void)
{
    if (gADCInitialized) {
        return;
    }

    /*
     * This function sets up ADC behavior for this module.
     */
    DL_ADC12_setClockConfig(ROOMMON_ADC_INST, (DL_ADC12_ClockConfig *) &gADCClockConfig);

    DL_ADC12_initSingleSample(
        ROOMMON_ADC_INST,
        DL_ADC12_REPEAT_MODE_DISABLED,
        DL_ADC12_SAMPLING_SOURCE_MANUAL,
        DL_ADC12_TRIG_SRC_SOFTWARE,
        DL_ADC12_SAMP_CONV_RES_12_BIT,
        DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED
    );

    /* Sample time long enough for sensor divider to settle */
    DL_ADC12_setSampleTime0(ROOMMON_ADC_INST, 80U);

    /*
     * Preload a default channel so MEM0 is valid immediately.
     * Actual reads will switch channel as needed.
     */
    DL_ADC12_configConversionMem(
        ROOMMON_ADC_INST,
        ROOMMON_ADC_MEM_INDEX,
        ADC_TEMP_CHANNEL,
        DL_ADC12_REFERENCE_VOLTAGE_VDDA,
        DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
        DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED,
        DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
        DL_ADC12_WINDOWS_COMP_MODE_DISABLED
    );

    DL_ADC12_enableConversions(ROOMMON_ADC_INST);

    gADCInitialized = true;
}

uint16_t readTemperatureRaw(void)
{
    return adcReadAveragedRaw(ADC_TEMP_CHANNEL);
}

uint16_t readLightRaw(void)
{
    return adcReadAveragedRaw(ADC_LIGHT_CHANNEL);
}

float convertTemperatureRawToC(uint16_t raw)
{
    float resistance;
    float tempK;
    float tempC;

    /*
     * Protect against divide-by-zero and saturated values.
     */
    if (raw == 0U) {
        return -100.0f;
    }
    if (raw >= (uint16_t)ADC_SENSOR_MAX_COUNTS) {
        return 150.0f;
    }

    /*
     * Assumes divider:
     *   VDDA --- thermistor --- ADC node --- fixed resistor --- GND
     *
     * If readings move backwards when warming the sensor,
     * flip the divider equation after testing.
     */
    resistance = THERM_FIXED_RESISTOR_OHMS *
                 ((ADC_SENSOR_MAX_COUNTS / (float)raw) - 1.0f);

    /*
     * Beta equation
     */
    tempK = 1.0f / (
        (1.0f / THERM_T0_KELVIN) +
        (1.0f / THERM_BETA) * logf(resistance / THERM_NOMINAL_RES_OHMS)
    );

    tempC = tempK - 273.15f;
    return tempC;
}

float convertLightRawToPercent(uint16_t raw)
{
    float percent = ((float)raw / ADC_SENSOR_MAX_COUNTS) * 100.0f;

    if (percent < 0.0f) {
        percent = 0.0f;
    }
    if (percent > 100.0f) {
        percent = 100.0f;
    }

    return percent;
}

float readTemperatureC(void)
{
    return convertTemperatureRawToC(readTemperatureRaw());
}

float readLightPercent(void)
{
    return convertLightRawToPercent(readLightRaw());
}