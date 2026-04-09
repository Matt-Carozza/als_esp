#pragma once 

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_log.h"
#include "cJSON.h"

#define CAM_RESOLUTION 64

typedef enum {
    ORIGIN_MAIN,
    ORIGIN_APP,
    ORIGIN_LIGHT,
    ORIGIN_OCC_SENSOR,
    ORIGIN_DAYLIGHT_SENSOR,
    ORIGIN_CAMERA,
    ORIGIN_UKNOWN
} MessageOrigin;

typedef enum {
    DEVICE_MAIN,
    DEVICE_APP,
    DEVICE_LIGHT,
    DEVICE_OCC_SENSOR,
    DEVICE_DAYLIGHT_SENSOR,
    DEVICE_CAMERA,
    DEVICE_UNKNOWN 
} DeviceType;

typedef enum {
    SEND_FRAME,
    CAMERA_UNKNOWN
} CameraAction;

typedef struct {
    uint8_t pixel_data[CAM_RESOLUTION];
    uint8_t room_id;
} CameraPayload;

typedef struct {
    MessageOrigin origin;
    DeviceType device;
    CameraAction action;
    CameraPayload payload;
} CameraMessage;


/*
    Parsers
*/

// bool parse_broker_message(const char* json, CameraMessage *msg);

/*
    serializers 
*/

bool serialize_message(const CameraMessage *msg, char* out, size_t out_len);

/*
    Turns keys (string) found within JSON objects into corresponding enums
*/

MessageOrigin origin_from_string(const char *s);
DeviceType device_from_string(const char *s);
CameraAction camera_action_from_string(const char *s);

/*
    Turns enumerations found within QueueMessage struct into corresponding strings
*/

const char* origin_to_string(MessageOrigin origin);
const char* device_to_string(DeviceType device);
const char* camera_action_to_string(CameraAction camera_action);