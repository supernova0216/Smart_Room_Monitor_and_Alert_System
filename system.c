#include "system.h"

SystemState systemValue = {
    .temperature = 0.0,
    .light = 0.0,
    .status = NORMAL,       //=0
    .sampleNow = false
};

SystemStatus processTemp(float temp) {
    if (temp > THRESH->temp_high) return TEMP_HIGH;
    if (temp < THRESH->temp_low) return TEMP_LOW;
    return NORMAL
}

SystemStatus processLight(float light) {
    if (light > THRESH->light_high) return LIGHT_HIGH;
    if (light < THRESH->light_low) return LIGHT_LOW;
    return NORMAL
}

SystemStatus processSensors(float temp, float light) {

    SystemStatus temp_status = processTemp(temp);
    SystemStatus light_status = processLight(light);

    if (temp_status != NORMAL && light_status != NORMAL) return MULTIPLE_ALERT;
    if (temp_status != NORMAL) return temp_status;
    if (light_status != NORMAL) return light_status;

    return NORMAL;
}