#include "adc_sensor.h"
#include <math.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

#if defined(ADC12_0_INST)
    #define ROOMMON_ADC_INST ADC12_0_INST
#else
    #error "ADC instance macro not found. Check your local ti_msp_dl_config.h."
#endif

#define ROOMMON_ADC_MEM_INDEX   DL_ADC12_MEM_IDX_0

static bool gADCInitialized = false;

void ADC_Sensor_init(void)
{
    if (gADCInitialized) {
        return;
    }

    /* Voltage Reference Module Setup */
    static const DL_VREF_ClockConfig gVREFClockConfig = {
        .clockSel    = DL_VREF_CLOCK_BUSCLK,
        .divideRatio = DL_VREF_CLOCK_DIVIDE_1
    };
    static const DL_VREF_Config gVREFConfig = {
        .vrefEnable     = DL_VREF_ENABLE_ENABLE,
        .bufConfig      = DL_VREF_BUFCONFIG_OUTPUT_1_4V,
        .shModeEnable   = DL_VREF_SHMODE_DISABLE,
        .holdCycleCount = DL_VREF_HOLD_MIN,
        .shCycleCount   = DL_VREF_SH_MIN
    };

    DL_VREF_reset(VREF);
    DL_VREF_enablePower(VREF);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_VREF_setClockConfig(VREF, (DL_VREF_ClockConfig *) &gVREFClockConfig);
    DL_VREF_configReference(VREF, (DL_VREF_Config *) &gVREFConfig);

    /* ADC clock configuration */
    static const DL_ADC12_ClockConfig gADCClockConfig = {
        .clockSel    = DL_ADC12_CLOCK_SYSOSC,
        .divideRatio = DL_ADC12_CLOCK_DIVIDE_4,
        .freqRange   = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32
    };

    DL_ADC12_reset(ROOMMON_ADC_INST);
    DL_ADC12_enablePower(ROOMMON_ADC_INST);
    DL_ADC12_setClockConfig(ROOMMON_ADC_INST, (DL_ADC12_ClockConfig *) &gADCClockConfig);
    DL_ADC12_initSingleSample(
        ROOMMON_ADC_INST,
        DL_ADC12_REPEAT_MODE_ENABLED,
        DL_ADC12_SAMPLING_SOURCE_MANUAL,
        DL_ADC12_TRIG_SRC_SOFTWARE,
        DL_ADC12_SAMP_CONV_RES_12_BIT,
        DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED
    );
    DL_ADC12_configConversionMem(
        ROOMMON_ADC_INST,
        ROOMMON_ADC_MEM_INDEX,
        ADC_TEMP_CHANNEL,
        DL_ADC12_REFERENCE_VOLTAGE_INTREF,
        DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
        DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED,
        DL_ADC12_TRIGGER_MODE_TRIGGER_NEXT,
        DL_ADC12_WINDOWS_COMP_MODE_DISABLED
    );
    DL_ADC12_setPowerDownMode(ROOMMON_ADC_INST, DL_ADC12_POWER_DOWN_MODE_MANUAL);
    DL_ADC12_enableConversions(ROOMMON_ADC_INST);

    gADCInitialized = true;
}

float readTemperatureC(void)
{
    DL_ADC12_startConversion(ROOMMON_ADC_INST);
    delay_cycles(TEMP_SAMPLE_TIME);
    DL_ADC12_stopConversion(ROOMMON_ADC_INST);

    uint32_t timeout = ADC_TIMEOUT;
    while (DL_ADC12_isConversionStarted(ROOMMON_ADC_INST)) {
        if (--timeout == 0) {
            return -1.0f;   // timeout — return sentinel value
        }
    }

    float adc_code  = (float) DL_ADC12_getMemResult(ROOMMON_ADC_INST, ROOMMON_ADC_MEM_INDEX);
    float v_sample  = VREF_1V4 * (adc_code - 0.5f);
    float v_trim    = VREF_3V3 * ((float) DL_SYSCTL_getTempCalibrationConstant() - 0.5f);
    float t_sample  = ((1.0f / TYP_TSC) * (v_sample - v_trim)) + TYP_TSTRIM;

    return t_sample;
}

float readLightPercent(void)
{
 
    return 5000.0f;
}
