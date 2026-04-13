#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"

#include "message_router.h"
#include "transport_mqtt.h"
#include "protocol.h"
#include "camera.h"
#include "driver/i2c_master.h"
//#include "freertos/task.h"


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
    #pragma region I2C config
            i2c_master_bus_config_t bus_config = {
                .clk_source = I2C_CLK_SRC_DEFAULT,
                .i2c_port = I2C_NUM_0,
                .scl_io_num = GPIO_NUM_22, 
                .sda_io_num = GPIO_NUM_21, 
                .glitch_ignore_cnt = 7,
                .flags.enable_internal_pullup = true, // Still recommended to use external ones!
            };

            i2c_master_bus_handle_t bus_handle;
            ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

            // registering ir camera as a slave
            i2c_device_config_t dev_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = 0x68, // address of camera (68 if AD0 is tied to GND, 69 is default or if tied to VDD)
                .scl_speed_hz = 100000,   // this is standard
            };

            i2c_master_dev_handle_t dev_handle;
            ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    #pragma endregion
    #pragma region adjustable variables
    float MinDetectionThreshold = 70.0;
    float step_threshold = 5;
    float MaxDetectionThreshold = MinDetectionThreshold + (step_threshold * 3); // 85
    int scan_rate = 1; // sec
    #pragma endregion    


    while (1) {
        CameraMessage msg = {
            .origin = ORIGIN_CAMERA,
            .device = DEVICE_APP,
            .action = SEND_FRAME,
            .payload =  {
                .room_id = 1
            }
        };


        #pragma region loop variables
        uint8_t raw_frame[128];
        float fahrenheit_temp[64];
        #pragma endregion


        // point to the first pixel register and read 128 bytes
        uint8_t reg = 0x80;
        esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg, 1, raw_frame, 128, -1);

        if (ret == ESP_OK) {
            for (int i = 0; i < 64; i++) {
                // combines the two bytes per pixel
                int16_t combined = ((int16_t)raw_frame[i * 2 + 1] << 8) | raw_frame[i * 2];

                // thia allows negative temps to be read correctly
                if (combined & 0x800) {
                    combined |= 0xF000;
                }

                // to celcuis
                float celsius = combined * 0.25f;
                // celcius to fahrenheit
                fahrenheit_temp[i] = (celsius * 1.8f) + 32.0f;
            }
        }

        // Convert temperature array from fahrenheit to int value
  
        for (int i = 0; i<64; i++){
            if(fahrenheit_temp[i] >= (MaxDetectionThreshold)){ // > 85
                msg.payload.pixel_data[i] = 2;
            }
            else if(fahrenheit_temp[i] >= (MinDetectionThreshold + (step_threshold *2))){ // 80 < x < 85
                msg.payload.pixel_data[i] = 1;
            }
            else{ // < 80
                msg.payload.pixel_data[i] = 0;
            }
        }

        // print msg.payload.pixel_data array in 8x8 format
        // for(int rowA = 0; rowA < 8; rowA++){
            
        //     for (int i = rowA+56; i > -1; i = i-8){
        //         printf("\033[0m"); // white color
        //         printf("%d ", msg.payload.pixel_data[i]);
        //     }
        //     printf("\n");
        // }
        // printf("\n");

        // // for colored display -- separate from top loop, because values would be printed over each other otherwise
        // for(int row = 0; row < 8; row++){
        //     for (int i = row+56; i > -1; i = i-8){
        //         // color
        //         if (fahrenheit_temp[i] >= MaxDetectionThreshold) { // > 85
        //             printf("\033[0;31m"); // Bright Red 
        //         } 
        //         else if (fahrenheit_temp[i] >= (MinDetectionThreshold + (step_threshold *2))) { // > 80
        //             printf("\033[0;33m"); // Yellow
        //         }
        //         else { // < 80
        //             printf("\033[0;34m"); // Blue 
        //         }
        //         printf("%6.1f ", fahrenheit_temp[i]);
        //     }
        //     printf("\n\n");
        // }

        if (message_router_push_local(&msg) != pdPASS) { 
            ESP_LOGE("STATUS_TASK", "Failed to send message to queue");
        } 
        vTaskDelay(pdMS_TO_TICKS(scan_rate*110));
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
    // xTaskCreate(heartbeat_task, "status_task", 4096, NULL, 4, NULL);
    
    mqtt_transport_start();
    xTaskCreate(camera_task, "camera_task", 4096, NULL, 5, NULL);
}