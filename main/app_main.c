#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_sleep.h"

#include "message_router.h"
#include "transport_mqtt.h"
#include "protocol.h"
#include "mobile_app.h"
#define Daylight_PIN 2
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
            .origin = ORIGIN_DAYLIGHT_SENSOR, 
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

RTC_DATA_ATTR size_t counter = 0; //A value that carries over during deep sleeps GLOBAL

void app_main(void)
{
    const int wakeup_time_sec = 10;
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
    ESP_LOGI(TAG, "[APP] Startup..");

    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << Daylight_PIN),     // Select GPIO 2
        .mode = GPIO_MODE_OUTPUT,              // Set as output
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE         // Disable interrupts
    };
    gpio_config(&io_conf);

    static esp_adc_cal_characteristics_t adc2_chars;
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_DEFAULT, 0, &adc2_chars);
    //adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
    adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_12);
    uint32_t voltage;
    int adc_value = 0;
    

    // Turn Sensor On
    printf("Daylight Sensor ON\n");
    gpio_set_level(Daylight_PIN, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay half second
        
    //Read Daylight voltage value
    adc2_get_raw(ADC2_CHANNEL_8, ADC_WIDTH_BIT_DEFAULT,&adc_value);
    voltage = esp_adc_cal_raw_to_voltage(adc_value, &adc2_chars);
    printf("Voltage: %ld mV", voltage);
    printf("\n");

    //Turn Sensor Off
    printf("Daylight Sensor OFF\n");
    gpio_set_level(Daylight_PIN, 0);

    counter++; 
    if (counter < 6){
        printf("Entering deep sleep for %d seconds\n", wakeup_time_sec);
        esp_deep_sleep_start();
    }
    counter = 0;
    

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

    mqtt_transport_start();
}
