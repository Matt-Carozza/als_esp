#include <stdio.h>
#include <string.h>
#include "esp_sleep.h"
#include "esp_log.h"

#include "daylight_buffer.h"

#define MAGIC_BUFFER_VALUE (uint32_t) 0x5A5A5A5AU

static const char *TAG = "DAYLIGHT_BUFFER";
static RTC_DATA_ATTR DaylightBuffer buffer; // Carries through deep sleep

void daylight_buffer_append(uint32_t voltage) {
    if (buffer.magic != MAGIC_BUFFER_VALUE) {
        buffer.head = 0;
        buffer.count = 0;
        buffer.magic = MAGIC_BUFFER_VALUE;
    }
     
    buffer.data[buffer.head] = voltage;
    buffer.head = (buffer.head + 1) % DAYLIGHT_BUFFER_SIZE;
    if (buffer.count < DAYLIGHT_BUFFER_SIZE)
        buffer.count++;    
}

uint32_t daylight_buffer_average(void) {
    if (buffer.count == 0) {
        return 0;
    }

    uint64_t sum = 0;

    for (size_t i = 0; i < buffer.count; ++i) {
        sum += buffer.data[i];
    }
    return (uint32_t) (sum / buffer.count);
}

uint8_t daylight_buffer_get_count(void) {
    return buffer.count;
}