#include <stdio.h>
#include "protocol.h"
#include "cJSON.h"

static const char *TAG = "PROTOCOL";

static bool get_string_ref(cJSON *obj, const char *key, const char** out);
static bool get_u8(cJSON *obj, const char* key, uint8_t *out);

// bool parse_broker_message(const char* json, OccMessage *out) {
//     if (!out) return false;

//     cJSON *root = cJSON_Parse(json);
//    
//     const char *device_from_message, *origin, *action_from_message;

//     bool ok = false;

//     if (!root) goto fail;

//     if (!get_string_ref(root, "origin", &origin)) goto fail;
//     if (!get_string_ref(root, "device", &device_from_message)) goto fail;
//     if (!get_string_ref(root, "action", &action_from_message)) goto fail;
//     
//     out->origin = origin_from_string(origin);
//     out->device = device_from_string(device_from_message);
//     out->action = occ_action_from_string(action_from_message);
//     
//     if (out->device != DEVICE_OCC_SENSOR) {
//         ESP_LOGI(TAG, "Wrong Device Type: %s", device_from_message);
//         goto fail;
//     }
//     
//     switch (out->device)
//     {
//         case DEVICE_APP:
//             /* code */
//             break;
//         case DEVICE_LIGHT:
//             ok = parse_light_message(root, out);
//             /* code */
//             break;
//         case DEVICE_OCC_SENSOR:
//             /* code */
//             break;
//         case DEVICE_UNKNOWN:
//             ESP_LOGI(TAG, "Unknown Device Type: %s", device_from_message);
//             break;
//         default:
//             break;
//     }
//     
// fail:
//     if (root) cJSON_Delete(root);
//     return ok;
// }


// bool parse_app_message(cJSON *root, QueueMessage *out) {
//     // Code here
//     bool ok = false;
//     return ok;
// }

bool occ_serialize_message(const OccMessage *msg, char* out, size_t out_len) {
    if (!msg || !out || out_len == 0) return false;
    cJSON *root = cJSON_CreateObject();
    
    if (!cJSON_AddStringToObject(root, "origin", 
            origin_to_string(msg->origin))  ||
        !cJSON_AddStringToObject(root, "device", 
            device_to_string(msg->device)) ||
        !cJSON_AddStringToObject(root, "action",
            occ_action_to_string(msg->action))) {
        cJSON_Delete(root);
        return false;
    }

    cJSON *payload = cJSON_CreateObject();
    if (!payload) {
        cJSON_Delete(root);
        return false;
    }

    bool ok = false;

    switch (msg->action)
    {
        case HEARTBEAT_UPDATE:
            ok = cJSON_AddBoolToObject(payload, "connected_to_broker", 
                                       msg->payload.heartbeat_update.connected_to_broker);
            break;
        case OCC_UPDATE:
            ok = cJSON_AddBoolToObject(payload, "occupied", 
                                       msg->payload.occ_update.occupied)&&
                 cJSON_AddNumberToObject(payload, "room_id", 
                                       msg->payload.occ_update.room_id);
            break;
        default:
            cJSON_Delete(root);
            return false;
    }
    
    if (!ok) {
        cJSON_Delete(payload);
        cJSON_Delete(root);
        return false;
    }
    
    cJSON_AddItemToObject(root, "payload", payload);
    
    char *tmp = cJSON_PrintUnformatted(root);

    if (!tmp) {
       cJSON_Delete(root);
       return false;
    }
    
    strncpy(out, tmp, out_len);
    out[out_len - 1] = '\0';
    
    free(tmp);
    cJSON_Delete(root);
    return true;
}

MessageOrigin origin_from_string(const char *s) {
    if (!strcmp(s, "MAIN")) return ORIGIN_MAIN;
    if (!strcmp(s, "APP")) return ORIGIN_APP;
    if (!strcmp(s, "LIGHT")) return ORIGIN_LIGHT;
    if (!strcmp(s, "OCC")) return ORIGIN_OCC_SENSOR;
    if (!strcmp(s, "DAYLIGHT")) return ORIGIN_DAYLIGHT_SENSOR;
    return ORIGIN_UKNOWN;
}

DeviceType device_from_string(const char *s) {
    if (!strcmp(s, "MAIN")) return DEVICE_MAIN;
    if (!strcmp(s, "APP")) return DEVICE_APP;
    if (!strcmp(s, "LIGHT")) return DEVICE_LIGHT;
    if (!strcmp(s, "OCC")) return DEVICE_OCC_SENSOR;
    if (!strcmp(s, "DAYLIGHT")) return DEVICE_DAYLIGHT_SENSOR;
    return DEVICE_UNKNOWN;
}

OccAction occ_action_from_string(const char *s) {
    if (!strcmp(s, "OCC_UPDATE")) return OCC_UPDATE;
    if (!strcmp(s, "HEARTBEAT_UPDATE")) return HEARTBEAT_UPDATE;
    return OCC_ACTION_UNKNOWN;
}

const char* origin_to_string(MessageOrigin origin) {
    switch (origin)
    {
        case ORIGIN_MAIN:
            return "MAIN";
        case ORIGIN_APP:
            return "APP";
        case ORIGIN_LIGHT:
            return "LIGHT";
        case ORIGIN_OCC_SENSOR:
            return "OCC";
        case ORIGIN_DAYLIGHT_SENSOR:
            return "DAYLIGHT";
        case ORIGIN_UKNOWN:
            return "UNKNOWN";
        default:
            return NULL;
    }
}
const char* device_to_string(DeviceType device) {
    switch (device)
    {
        case DEVICE_MAIN:
            return "MAIN";
        case DEVICE_APP:
            return "APP";
        case DEVICE_LIGHT:
            return "LIGHT";
        case DEVICE_OCC_SENSOR:
            return "OCC";
        case DEVICE_DAYLIGHT_SENSOR:
            return "DAYLIGHT";
        case DEVICE_UNKNOWN:
            return "UNKNOWN";
        default:
            return NULL;
    }
}

const char* occ_action_to_string(OccAction occ_action) {
    switch (occ_action)
    {
        case OCC_UPDATE:
            return "OCC_UPDATE";
        case HEARTBEAT_UPDATE:
            return "HEARTBEAT_UPDATE";
        default:
            return NULL;
    }
}

static bool get_string_ref(cJSON *obj, const char *key, const char** out) {
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (!cJSON_IsString(item)) return false;
    *out = item->valuestring;
    return true;
}

static bool get_u8(cJSON *obj, const char* key, uint8_t *out) {
   cJSON *item = cJSON_GetObjectItem(obj, key); 
   if (!cJSON_IsNumber(item)) return false;

   double v = item->valuedouble;
   if (v < 0 || v > 255) return false;
   
   *out = (uint8_t)v;
   return true;
}

static bool get_bool(cJSON *obj, const char* key, bool *out) {
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (!cJSON_IsBool(item)) return false;
    
    bool b = item->valueint;
    *out = b;
    return true;
}