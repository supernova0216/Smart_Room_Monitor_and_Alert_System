#include "system.h"

SystemState systemValue = {
    .temperature = 0.0,
    .light = 0.0,
    .status = NORMAL,       //=0
    .sampleNow = false
};

SystemStatus processTemp(THRESHOLDS* thresh, float temp) {
    if (temp > thresh->temp_high) return TEMP_HIGH;
    if (temp < thresh->temp_low) return TEMP_LOW;
    return NORMAL;
}

SystemStatus processLight(THRESHOLDS* thresh, float light) {
    if (light > thresh->light_high) return LIGHT_HIGH;
    if (light < thresh->light_low) return LIGHT_LOW;
    return NORMAL;
}

SystemStatus processSensors(THRESHOLDS* thresh, float temp, float light) {

    SystemStatus temp_status = processTemp(thresh, temp);
    SystemStatus light_status = processLight(thresh, light);
    
    // printf("[Internal] Temp (%.2f) Status: ", temp);
    // printStatus(temp_status);
    // printf(" | [Internal] Light (%.2f) Status: ", light);
    // printStatus(light_status);
    // printf("\n");

    if (temp_status != NORMAL && light_status != NORMAL) return MULTIPLE_ALERT;
    if (temp_status != NORMAL) return temp_status;
    if (light_status != NORMAL) return light_status;

    return NORMAL;
}

void printStatus(SystemStatus status) {
    switch (status) {
        case NORMAL:
            printf("NORMAL");
            break;
        case TEMP_HIGH:
            printf("TEMP_HIGH");
            break;
        case TEMP_LOW:
            printf("TEMP_LOW");
            break;
        case LIGHT_HIGH:
            printf("LIGHT_HIGH");
            break;
        case LIGHT_LOW:
            printf("LIGHT_LOW");
            break;
        case MULTIPLE_ALERT:
            printf("MULTIPLE_ALERT");
            break;
        default:
            printf("INVALID STATUS");
    }
}