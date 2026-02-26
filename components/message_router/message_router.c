#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "protocol.h"
#include "message_router.h"

static QueueHandle_t message_queue;
static const char *TAG = "ROUTER";

void message_router_init(void) {
    message_queue = xQueueCreate(10, sizeof(OccMessage));
}

/*
Pushes string data to the wire
*/
// bool message_router_push_wire(const char *json) {
//     OccMessage msg;
//     
//     if(!parse_broker_message(json, &msg)) {
//         ESP_LOGE(TAG, "Parse Failed");
//         return false;
//     }

//     return xQueueSend(message_queue, &msg, portMAX_DELAY);
// }

/*
Pushes OccMessage struct data to the wire
*/
bool message_router_push_local(const OccMessage* msg) {
    return xQueueSend(message_queue, msg, 0);
}

bool message_router_receive(OccMessage* msg) {
   return xQueueReceive(message_queue, msg, portMAX_DELAY); 
}
