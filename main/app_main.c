#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"

#include "message_router.h"
#include "transport_mqtt.h"
#include "protocol.h"
#include "mobile_app.h"
#include "driver/gpio.h"

#define OccSensorInput_PIN (GPIO_NUM_18) // to recieve occupancy from sensor

// #include "string_type.h" PROB REMOVE

void queue_task(void *pvParameters);
void status_task(void *pvParameters);

static const char *TAG = "APP_MAIN";

void queue_task(void *pvParameters) {
    QueueMessage msg;
    while (1) {
        if (message_router_receive(&msg) == pdPASS) {
            switch (msg.device) {

                case DEVICE_MAIN:
                    break;
                case DEVICE_APP:
                    mobile_app_handle(&msg);
                    break;
                case DEVICE_LIGHT:
                    // Check mqtt_transport.c to see how to go from wireless broker data --> queue task
                    uint8_t r = msg.light.payload.r;
                    uint8_t g = msg.light.payload.g;
                    uint8_t b = msg.light.payload.b;
                    ESP_LOGI(TAG, "%u %u %u", r, g, b);
                    break;
                case DEVICE_OCC_SENSOR:
                    break;
                case DEVICE_DAYLIGHT_SENSOR:
                    break;
                case DEVICE_UNKNOWN:
                    ESP_LOGE(TAG, "ERROR During Queue: Device Unknown");
                    break;
            }
        }
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
void status_task(void *pvParameters) {
    while (1) {
        QueueMessage msg = {
            .origin = ORIGIN_OCC_SENSOR, 
            .device = DEVICE_APP,
            .app = {
                .action = APP_STATUS,
                .payload = {
                    .connected_to_broker = mqtt_transport_is_connected(),
                }
            }
        };

        if (message_router_push_local(&msg) != pdPASS) { 
            ESP_LOGE("STATUS_TASK", "Failed to send message to queue");
        } 
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void occ_task(void *pvParameters) {
    
    gpio_set_direction(OccSensorInput_PIN, GPIO_MODE_INPUT);

    bool PreviousSensorStatus = 0;      

    while(1) {
        int CurrentSensorStatus = gpio_get_level(OccSensorInput_PIN);
            // if occupancy changes, then print the change
            if (CurrentSensorStatus != PreviousSensorStatus) {
                if (CurrentSensorStatus == 1) {
                    ESP_LOGI("Occupancy status: Occupied\n");
                } else {
                    ESP_LOGI("Occupancy status: Unoccupied\n");
                }
            
                // update previous status
                PreviousSensorStatus = CurrentSensorStatus;
            }   

            vTaskDelay(pdMS_TO_TICKS(200)); // task is paused every 200 ms, to allow other tasks to be priortized
        }
    };


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

    xTaskCreate(queue_task, "queue_task", 4096, NULL, 5, NULL);
    xTaskCreate(status_task, "status_task", 4096, NULL, 4, NULL);
    xTaskCreate(occ_task, "occ_task", 4096, NULL, 5, NULL);
    
    mqtt_transport_start();
}
