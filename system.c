#include "system.h"

SystemState systemValue = {
    .temperature = 0.0,
    .light = 0.0,
    .status = NORMAL,       //=0
    .sampleNow = false
};