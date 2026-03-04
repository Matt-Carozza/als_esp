#include <stdint.h>

#define DAYLIGHT_BUFFER_SIZE 6U

typedef struct {    
    uint32_t magic;
    uint8_t head;
    uint8_t count;
    uint32_t data[DAYLIGHT_BUFFER_SIZE];
} DaylightBuffer;


void daylight_buffer_append(uint32_t voltage);
uint32_t daylight_buffer_average(void);
uint8_t daylight_buffer_get_count(void);