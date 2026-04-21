#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"

#include "message_router.h"
#include "transport_mqtt.h"
#include "protocol.h"
#include "mobile_app.h"

#include <stdlib.h>
#include "driver/ledc.h"
#include "esp_log.h"

// #include "string_type.h" PROB REMOVE

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define RLEDC_OUTPUT_IO          (17) // Define the output GPIO
#define GLEDC_OUTPUT_IO          (16) // Define the output GPIO
#define BLEDC_OUTPUT_IO          (5) // Define the output GPIO
#define RLEDC_CHANNEL           LEDC_CHANNEL_0
#define GLEDC_CHANNEL           LEDC_CHANNEL_1
#define BLEDC_CHANNEL           LEDC_CHANNEL_2
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (128) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (1000) // Frequency in Hertz. Set frequency at 4 kHz

void queue_task(void *pvParameters);
void status_task(void *pvParameters);

static const char *TAG = "APP_MAIN";

uint8_t prevr = 0;
uint8_t prevg = 0;
uint8_t prevb = 0;

static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t Rledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 833.333 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&Rledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t Rledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = RLEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = RLEDC_OUTPUT_IO,
        .duty           = 0, 
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&Rledc_channel));
        // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t Gledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 833.333 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&Gledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t Gledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = GLEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = GLEDC_OUTPUT_IO,
        .duty           = 0, 
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&Gledc_channel));
        // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t Bledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 833.333 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&Bledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t Bledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = BLEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BLEDC_OUTPUT_IO,
        .duty           = 0, 
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&Bledc_channel));
}

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

                    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, RLEDC_CHANNEL, r));
                    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, RLEDC_CHANNEL));

                    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, GLEDC_CHANNEL, g));
                    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, GLEDC_CHANNEL));

                    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, BLEDC_CHANNEL, b));
                    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, BLEDC_CHANNEL));
                    // while (prevr != r||prevg != g||prevb != b)
                    // {
                        // if (prevr<r)
                        // {
                        //     ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, RLEDC_CHANNEL, prevr));
                        //     ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, RLEDC_CHANNEL));
                        //     prevr = prevr+1;
                        //     vTaskDelay(10 / portTICK_PERIOD_MS);
                        // }
                        // else if (prevr>r)
                        // {
                        //     ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, RLEDC_CHANNEL, prevr));
                        //     ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, RLEDC_CHANNEL));
                        //     prevr = prevr-1;
                        //     vTaskDelay(10 / portTICK_PERIOD_MS);
                        // }
                        // if (prevg<g)
                        // {
                        //     ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, GLEDC_CHANNEL, prevg));
                        //     ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, GLEDC_CHANNEL));
                        //     prevg = prevg+1;
                        //     vTaskDelay(10 / portTICK_PERIOD_MS);
                        // }
                        // else if (prevg>g)
                        // {
                        //     ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, GLEDC_CHANNEL, prevg));
                        //     ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, GLEDC_CHANNEL));
                        //     prevg = prevg-1;
                        //     vTaskDelay(10 / portTICK_PERIOD_MS);
                        // }
                        // if (prevb<b)
                        // {
                        //     ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, BLEDC_CHANNEL, prevb));
                        //     ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, BLEDC_CHANNEL));
                        //     prevb = prevb+1;
                        //     vTaskDelay(10 / portTICK_PERIOD_MS);
                        // }
                        // else if (prevb>b)
                        // {
                        //     ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, BLEDC_CHANNEL, prevb));
                        //     ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, BLEDC_CHANNEL));
                        //     prevb = prevb-1;
                        //     vTaskDelay(10 / portTICK_PERIOD_MS);
                        // }
                        printf("\nRGB values : %d, %d, %d\n", r, g, b);
                    // }       
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
            .origin = ORIGIN_LIGHT,  
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

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ledc_init();
    
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
