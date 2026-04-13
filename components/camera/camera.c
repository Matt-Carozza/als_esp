#include <stdio.h>
#include "camera.h"
#include "transport_mqtt.h"

static const char* TAG = "CAMERA_HANDLE";

static void publish_message(const CameraMessage* msg, const char* topic);

void camera_handle(const CameraMessage* msg) {
    const char* topic = NULL;
    switch (msg->action)
    {
    case SEND_FRAME:
        topic = "/als/camera";
        publish_message(msg, topic);
        break;
    
    default:

        ESP_LOGE(TAG, "Unknown Action %u", msg->action);
        break;
    }
    
}

static void publish_message(const CameraMessage* msg, const char* topic) {
    char json_buf[512];
    
    if (topic == NULL) {
        ESP_LOGE(TAG, "No MQTT topic resolved");
        return; 
    }
    
    if (!serialize_message(msg, json_buf, sizeof(json_buf))) {
        ESP_LOGE(TAG, "Serilization Error");
        return;
    }
    
    int msg_id = mqtt_transport_publish(topic, json_buf);
    //ESP_LOGI(TAG, "Published frame, msg_id=%d", msg_id);
}