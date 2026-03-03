#pragma once 

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_log.h"
#include "cJSON.h"

typedef enum {
    ORIGIN_MAIN,
    ORIGIN_APP,
    ORIGIN_LIGHT,
    ORIGIN_OCC_SENSOR,
    ORIGIN_DAYLIGHT_SENSOR,
    ORIGIN_UKNOWN
} MessageOrigin;

typedef enum {
    DEVICE_MAIN,
    DEVICE_APP,
    DEVICE_LIGHT,
    DEVICE_OCC_SENSOR,
    DEVICE_DAYLIGHT_SENSOR,
    DEVICE_UNKNOWN 
} DeviceType;

typedef enum {  
    HEARTBEAT_UPDATE,
    DAY_UPDATE,
    DAY_ACTION_UKNOWN
} DayAction;

typedef struct {
    union {
        struct {
            bool connected_to_broker;
        } heartbeat_update;
        struct {
            uint8_t room_id;
            float voltage;
        } day_update;
    };
} DayPayload;

typedef struct {
    MessageOrigin origin;
    DeviceType device;
    DayAction action;
    DayPayload payload;
} DayMessage;

/*
    Parsers
*/
// bool parse_broker_message(const char* json, QueueMessage *msg);
// bool parse_app_message(cJSON *root, QueueMessage *msg); // Unfinished
// bool parse_light_message(cJSON *root, QueueMessage *msg);

/*
    serializers 
*/

bool day_serialize_message(const DayMessage *msg, char* out, size_t out_len);

/*
    Turns keys (string) found within JSON objects into corresponding enums
*/

MessageOrigin origin_from_string(const char *s);
DeviceType device_from_string(const char *s);
DayAction day_action_from_string(const char *s);

/*
    Turns enumerations found within QueueMessage struct into corresponding strings
*/

const char* origin_to_string(MessageOrigin origin);
const char* device_to_string(DeviceType device);
const char* day_action_to_string(DayAction day_action);