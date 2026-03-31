#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"

#include "message_router.h"
#include "transport_mqtt.h"
#include "protocol.h"
#include "occupancy.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define OccSensorInput_PIN (GPIO_NUM_18) // to recieve occupancy from sensor
#define TXD_PIN (GPIO_NUM_17) // uart Tx
#define RXD_PIN (GPIO_NUM_16) // uart Rx
#define RX_BUF_SIZE 256

static const char *TAG = "APP_MAIN";

void uart_init() {

    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0); // uart1 (pins)
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

bool configure_off_delay(uint16_t off_delay) {
    // Command structures
    uint8_t EnableConfig[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01}; // Enable Configurarion Command
    uint8_t ReadBaseParameters[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x12, 0x00, 0x04, 0x03, 0x02, 0x01};
    uint8_t ConfigureBaseParameters[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
    uint8_t EndConfiguration[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};

    // Expected response lengths
    const int EnableConfig_Response_Length = 18;
    const int ReadBaseParameters_Response_Length = 19;
    const int ConfigBaseParameters_Response_Length = 14;
    const int EndConfig_Repsonse_Length = 14;
    // int sequenceSTEP = 5; // initialize sequence to step
                         
    // Transmit Enable Config command
    uart_write_bytes(UART_NUM_1, EnableConfig, sizeof(EnableConfig));
    //printf("Sent 'Enable Configuration' Command\n");
    printf("OFF-Delay message received\n");
   
    // Read response
    // uint8_t data[RX_BUF_SIZE];
    // int len = 0;
    // Check that the correct message has been recieved (len = 18)
    // while(len != EnableConfig_Response_Length) {
    //     int len = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 20 / portTICK_PERIOD_MS); // Message is read and the # of bytes is saved to variable len
    //     vTaskDelay(100 / portTICK_PERIOD_MS);
    // }
    uint8_t enable_config_data[RX_BUF_SIZE];
    int len = uart_read_bytes(UART_NUM_1, enable_config_data, RX_BUF_SIZE, 20 / portTICK_PERIOD_MS); // Message is read and the # of bytes is saved to variable len
    if (len != EnableConfig_Response_Length) {
        ESP_LOGE(TAG, "Enable Config Length Does Not Match");
        return false;
    }
   
    // Transmit "Read Base Parameters" command
    uart_write_bytes(UART_NUM_1, ReadBaseParameters, sizeof(ReadBaseParameters));
   
    uint8_t base_parameters_data[RX_BUF_SIZE];
    len = uart_read_bytes(UART_NUM_1, base_parameters_data, RX_BUF_SIZE, 20 / portTICK_PERIOD_MS); // Message is read and the # of bytes is saved to variable len
    // Check that the correct message has been recieved (len = 19)
    if (len != ReadBaseParameters_Response_Length) {
        ESP_LOGE(TAG, "Read Base Parameters Length Does Not Match");
        return false;
    }
    ConfigureBaseParameters[8] = base_parameters_data[10]; // Min Gate
    ConfigureBaseParameters[9] = base_parameters_data[11]; // Max Gate
    // minimum distance gate
    printf("Prev. Minimum distance gate: %d\n", base_parameters_data[10]);
    // maximum distance gate
    printf("Prev. Maximum distance gate: %d\n", base_parameters_data[11]-1);

    // split the off delay bytes
    uint8_t OFFdelay_low = (uint8_t)(off_delay & 0xFF);
    uint8_t OFFdelay_high = (uint8_t)(off_delay >> 8);

    // finish configuring base parameter command from off delay received from app
    ConfigureBaseParameters[10] = OFFdelay_low; // off delay (low)
    ConfigureBaseParameters[11] = OFFdelay_high; // off delay (high)

    // Transmit "Configure Base Parameters" command
    uart_write_bytes(UART_NUM_1, ConfigureBaseParameters, sizeof(ConfigureBaseParameters));

    uint8_t config_parameters_data[RX_BUF_SIZE];
    len = uart_read_bytes(UART_NUM_1, config_parameters_data, RX_BUF_SIZE, 20 / portTICK_PERIOD_MS); // Message is read and the # of bytes is saved to variable len
    // Check that the correct message has been recieved (len = 19)
    if (len != ConfigBaseParameters_Response_Length) {
        ESP_LOGE(TAG, "Config Base Length Does Not Match");
        return false;
    }
   
    // Transmit "end configuration" command
    uart_write_bytes(UART_NUM_1, EndConfiguration, sizeof(EndConfiguration));

    uint8_t end_config_data[RX_BUF_SIZE];
    len = uart_read_bytes(UART_NUM_1, end_config_data, RX_BUF_SIZE, 20 / portTICK_PERIOD_MS); // Message is read and the # of bytes is saved to variable len
    if (len != EndConfig_Repsonse_Length) {
        ESP_LOGE(TAG, "End Config Length Does Not Match");
        return false;
    }
    return true;
}

// #include "string_type.h" PROB REMOVE

//void queue_task(void *pvParameters);
//void heartbeat_task(void *pvParameters);

void queue_task(void *pvParameters) {
    OccMessage msg;
    while (1) {
        if (message_router_receive(&msg) == pdPASS) {
            occupancy_handle(&msg);
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
void heartbeat_task(void *pvParameters) {
    while (1) {
        OccMessage msg = {
            .origin = ORIGIN_OCC_SENSOR, 
            .device = DEVICE_MAIN,
            .action = HEARTBEAT_UPDATE,
            .payload = {
                .heartbeat_update = {
                    .connected_to_broker = mqtt_transport_is_connected(),
                }
            }
        };

        if (message_router_push_local(&msg) != pdPASS) { 
            ESP_LOGE("STATUS_TASK", "Failed to send message to queue");
        } 
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}

void occ_task(void *pvParameters) {
    
    gpio_set_direction(OccSensorInput_PIN, GPIO_MODE_INPUT);

    bool previous_sensor_status = 0;      

    while(1) {
        int current_sensor_status = gpio_get_level(OccSensorInput_PIN);
        OccMessage msg;
        // if occupancy changes, then print the change
        if (current_sensor_status != previous_sensor_status) {
            msg = (OccMessage) {
                .origin = ORIGIN_OCC_SENSOR,
                .device = DEVICE_MAIN,
                .action = OCC_UPDATE,
                .payload = {
                    .occ_update = {
                        .occupied = current_sensor_status == 1,
                        .room_id = 1,
                    }
                }
            };
            if (current_sensor_status == 1) {
                ESP_LOGI("OCC_TASK", "Room is now occupied");
            } else {
                ESP_LOGI("OCC_TASK", "Room is now unoccupied");
            }
            if (message_router_push_local(&msg) != pdPASS) { 
                ESP_LOGE("OCC_TASK", "Failed to send message to queue");
            } 
            // update previous status
            previous_sensor_status = current_sensor_status;
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
    uart_init();
    bool ok = false;
    int retries = 0;
    while (!ok && retries < 5) {
        ok = configure_off_delay(5);
        retries++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    message_router_init();

    xTaskCreate(queue_task, "queue_task", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_task, "status_task", 4096, NULL, 4, NULL);
    xTaskCreate(occ_task, "occ_task", 4096, NULL, 5, NULL);
    
    mqtt_transport_start();
}
