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
    OCC_UPDATE,
    OCC_ACTION_UNKNOWN
} OccAction;

typedef struct {
    union {
        struct {
            bool occupied; 
            uint8_t room_id; 
        } occ_update;
        bool occupied; 
        struct {
            bool connected_to_broker; 
        } heartbeat_update;
    };
} OccPayload;

typedef struct {
    MessageOrigin origin;
    DeviceType device;
    OccAction action;
    OccPayload payload;
} OccMessage;

/*
    Parsers
*/

// bool parse_broker_message(const char* json, OccMessage *msg);
// bool parse_app_message(cJSON *root, OccMessage *msg); // Unfinished
// bool parse_light_message(cJSON *root, OccMessage *msg);

/*
    serializers 
*/

bool occ_serialize_message(const OccMessage *msg, char* out, size_t out_len);

/*
    Turns keys (string) found within JSON objects into corresponding enums
*/

MessageOrigin origin_from_string(const char *s);
DeviceType device_from_string(const char *s);
OccAction occ_action_from_string(const char *s);

/*
    Turns enumerations found within QueueMessage struct into corresponding strings
*/

const char* origin_to_string(MessageOrigin origin);
const char* device_to_string(DeviceType device);
const char* occ_action_to_string(OccAction occ_action);