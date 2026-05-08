# include "threshold.h"


void initThresholds(Thresholds* t, int tl, int th, int ll, int lh) {
    t->temp_low     = tl;
    t->temp_high    = th;
    t->light_low    = ll;
    t->light_high   = lh;
}

int updateThreshold(Thresholds* t, THRESH_TYPE type, int val) {
    switch (type) {
        case THRESHOLD_TEMP_LOW:
            t->temp_low = val;
            // printf("Threshold [temp_low] updated to: %d\n", val);
            break;
        case THRESHOLD_TEMP_HIGH:
            t->temp_high = val;
            // printf("Threshold [temp_high] updated to: %d\n", val);
            break;
        case THRESHOLD_LIGHT_LOW:
            t->light_low = val;
            // printf("Threshold [light_low] updated to: %d\n", val);
            break;
        case THRESHOLD_LIGHT_HIGH:
            t->light_high = val;
            // printf("Threshold [light_high] updated to: %d\n", val);
            break;
        default:
            printf("Error: Invalid THRESH_TYPE (%d).\n", type);
            return 0;
    }
    return 1;
}