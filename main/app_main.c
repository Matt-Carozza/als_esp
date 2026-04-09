#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"

#include "message_router.h"
#include "transport_mqtt.h"
#include "protocol.h"
#include "camera.h"

// #include "string_type.h" PROB REMOVE

void queue_task(void *pvParameters);
void status_task(void *pvParameters);

static const char *TAG = "APP_MAIN";



void queue_task(void *pvParameters) {
    CameraMessage msg;
    while (1) {
        if (message_router_receive(&msg) == pdPASS) {
            camera_handle(&msg);
        }
    } 
}

void camera_task(void *pvParameters) {
    size_t translucent_pixel_location = 0;
    size_t opaque_pixel_location = 1;
    while (1) {
        CameraMessage msg = {
            .origin = ORIGIN_CAMERA,
            .device = DEVICE_APP,
            .action = SEND_FRAME,
            .payload =  {
                .room_id = 1
            }
        };
        for (size_t i = 0; i < CAM_RESOLUTION; ++i) {
            msg.payload.pixel_data[i] = 0;
            if (i == (opaque_pixel_location - 1)) 
                msg.payload.pixel_data[i] = 1;
            if (i == opaque_pixel_location) 
                msg.payload.pixel_data[i] = 2;
        }

        translucent_pixel_location = (translucent_pixel_location  + 1) % CAM_RESOLUTION;
        opaque_pixel_location = (opaque_pixel_location + 1) % CAM_RESOLUTION;

        if (message_router_push_local(&msg) != pdPASS) { 
            ESP_LOGE("STATUS_TASK", "Failed to send message to queue");
        } 
        vTaskDelay(pdMS_TO_TICKS(1000));
    } 
}

/*
    Good example of how data is sent out to the broker from your device.
    ctrl + click on functions to see implemenation details
    
    General route of data:

    1. app_main.c runs task every 10 seconds
    2. message_router.c pushes QueueMessage struct to the queue
    3. app_main.c queue_task gets message
    4. mobile_app.c handles messages for the app from the handler (handlers for other devices 
    can be created with the similar file structures)
    5. The handler then takes the message and serializes it 
    (QueueMessage struct --> string) using protocol.c functions
    6. The output created from serialization is then called within,
    mqtt_transport_publish which will finally publish the data to the broker
*/
// void heartbeat_task(void *pvParameters) {
//     while (1) {
//         QueueMessage msg = {
//             .origin = ORIGIN_MAIN, // TODO: REPLACE WITH YOUR DEVICE 
//             .device = DEVICE_APP,
//             .app = {
//                 .action = APP_STATUS,
//                 .payload = {
//                     .connected_to_broker = mqtt_transport_is_connected(),
//                 }
//             }
//         };

//         if (message_router_push_local(&msg) != pdPASS) { 
//             ESP_LOGE("STATUS_TASK", "Failed to send message to queue");
//         } 
//         vTaskDelay(60000 / portTICK_PERIOD_MS);
//     }
// }

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    
    message_router_init();

    xTaskCreate(queue_task, "queue_task", 4096, NULL, 4, NULL);
    xTaskCreate(camera_task, "camera_task", 4096, NULL, 5, NULL);
    // xTaskCreate(heartbeat_task, "status_task", 4096, NULL, 4, NULL);
    
    mqtt_transport_start();
}
