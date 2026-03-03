#include <stdio.h>
#include "daylight.h"
#include "transport_mqtt.h"

static const char* TAG = "DAYLIGHT_HANDLE";

void daylight_handle(const DayMessage *msg) {
    char json_buf[256];
    char topic_buf[64];
    const char* topic = NULL;
    
    if(!day_serialize_message(msg, json_buf, sizeof(json_buf))) {
        ESP_LOGE(TAG, "Serization Error");
        return;
    } 
    
    switch (msg->action)
    {
        case HEARTBEAT_UPDATE:
            topic = "/als/status/heartbeat";
            break;
        
        case DAY_UPDATE:
            topic = "/als/status/heartbeat";
            snprintf(topic_buf, 
                sizeof(topic_buf),
                "/als/status/day/%u",
                msg->payload.day_update.room_id);
            topic = topic_buf;
            break;
            
        default:
            break;
    }
    
    if (topic == NULL) {
       ESP_LOGE(TAG, "No MQTT topic resolved");
       return;
    }
    
    int msg_id = mqtt_transport_publish(topic, json_buf);
}