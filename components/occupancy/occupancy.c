#include <stdio.h>
#include "occupancy.h"
#include "transport_mqtt.h"

static const char* TAG = "OCCUPANCY_HANDLE";

void occupancy_handle(const OccMessage *msg) {
    char json_buf[256];
    char topic_buf[64];
    const char* topic = NULL;

    
    if(!occ_serialize_message(msg, json_buf, sizeof(json_buf))) {
        ESP_LOGE(TAG, "Serialization Error");
        return;
    }
    
    switch (msg->action)
    {
        case HEARTBEAT_UPDATE:
            topic = "/als/status/heartbeat"; 
            break;
        case OCC_UPDATE:
            snprintf(topic_buf, 
                sizeof(topic_buf), 
                "/als/status/occ/%u", 
                msg->payload.occ_update.room_id);
            topic = topic_buf;

            break;
        default:
            ESP_LOGW(TAG, "Unhandled OCC action: %d", msg->action);
            break;
    }

    // TODO: Change design document from "occ_sensor" to "occ"
    if (topic == NULL) {
        ESP_LOGE(TAG, "No MQTT topic resolved");
        return;
    }
    
    int msg_id = mqtt_transport_publish(topic, json_buf);  
    ESP_LOGI(TAG, "Published status message sent, msg_id=%d", msg_id);
}
