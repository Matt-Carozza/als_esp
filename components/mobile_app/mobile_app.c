#include <stdio.h>
#include "mobile_app.h"
#include "transport_mqtt.h"

static const char* TAG = "MOBILE_APP";

void mobile_app_handle(const QueueMessage *msg) {
    switch (msg->app.action)
    {
        case APP_STATUS:        
            /*
                Though not done here, any logic needed for this action can be
                put within this case statement here.
            */ 
            if (msg->app.payload.connected_to_broker == true) {
                char json_buf[256];
                
                if(!serialize_message(msg, json_buf, sizeof(json_buf))) {
                    ESP_LOGE(TAG, "Serialization Error");
                    return;
                }

                int msg_id = mqtt_transport_publish("/status", json_buf); 
            }
            break;
        
        default:
            break;
    }

    
}
