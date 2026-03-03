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
#include "daylight.h"
#define Daylight_PIN 2

void queue_task(void *pvParameters);
void heartbeat_task(void *pvParameters);

static const char *TAG = "APP_MAIN";
TaskHandle_t app_main_task_handle = NULL; // Pass data between tasks
static esp_adc_cal_characteristics_t adc2_chars;
RTC_DATA_ATTR size_t counter = 0; // Carries through deep sleep

static void init_sleep_timer(void) {
    const int wakeup_time_sec = 10;
    ESP_ERROR_CHECK(
        esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000)
    );
}

static void init_sensor_gpio(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << Daylight_PIN), // Select GPIO 2
        .mode = GPIO_MODE_OUTPUT,               // Set as output
        .pull_up_en = GPIO_PULLUP_DISABLE,      // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE          // Disable interrupts
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static void init_sensor_adc(void) {

    esp_adc_cal_characterize(ADC_UNIT_2, 
        ADC_ATTEN_DB_12, 
        ADC_WIDTH_BIT_DEFAULT, 
        0, 
        &adc2_chars
    );

    adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_12);
}

static uint32_t read_daylight_voltage(void) {
    uint32_t voltage;
    int adc_value = 0;

    // Turn Sensor On
    ESP_LOGD(TAG, "Daylight Sensor ON\n");
    gpio_set_level(Daylight_PIN, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay half second
        
    //Read Daylight voltage value
    adc2_get_raw(ADC2_CHANNEL_8, ADC_WIDTH_BIT_DEFAULT,&adc_value);
    voltage = esp_adc_cal_raw_to_voltage(adc_value, &adc2_chars);
    ESP_LOGI(TAG, "Daylight Voltage: %ld mV", voltage);

    //Turn Sensor Off
    ESP_LOGD(TAG, "Daylight Sensor OFF\n");
    gpio_set_level(Daylight_PIN, 0);

    return voltage;
}

static bool should_init_wifi(uint32_t voltage) {
    counter++;

    if (counter < 6) {
        return false;
    }
    
    counter = 0;
    return true;
}

void queue_task(void *pvParameters) {
    DayMessage msg;
    while (1) {
        if (message_router_receive(&msg) == pdPASS) {
            daylight_handle(&msg);
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
        DayMessage msg = {
            .origin = ORIGIN_DAYLIGHT_SENSOR, 
            .device = DEVICE_MAIN,
            .action = HEARTBEAT_UPDATE,
            .payload.heartbeat_update = {
                .connected_to_broker = mqtt_transport_is_connected(),
            }
        };

        if (message_router_push_local(&msg) != pdPASS) { 
            ESP_LOGE("STATUS_TASK", "Failed to send message to queue");
        } 
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    app_main_task_handle = xTaskGetCurrentTaskHandle();
    ESP_LOGI(TAG, "[APP] Startup..");

    init_sleep_timer();
    init_sensor_gpio();
    init_sensor_adc();
    
    uint32_t voltage = read_daylight_voltage();

    if (!should_init_wifi(voltage)) {
        ESP_LOGI(TAG, "Entering deep sleep...");
        esp_deep_sleep_start();
    }
    

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
    // xTaskCreate(heartbeat_task, "heartbeat_task", 4096, NULL, 4, NULL);
    
    mqtt_transport_start();
    mqtt_transport_set_app_task(app_main_task_handle);

    DayMessage msg = {
        .origin = ORIGIN_DAYLIGHT_SENSOR,
        .device = ORIGIN_MAIN,
        .action = DAY_UPDATE,
        .payload.day_update = {
            .room_id = 1,
            .voltage = voltage
        }
    };
    
    message_router_push_local(&msg);
    
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    esp_deep_sleep_start();
}
