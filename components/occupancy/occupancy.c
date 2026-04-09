#include <stdio.h>
#include "occupancy.h"
#include "transport_mqtt.h"
#include "driver/uart.h"

static const char* TAG = "OCCUPANCY_HANDLE";

static void publish_message(const OccMessage *msg, const char* topic);
static void log_message_header(const OccMessage *msg);
static bool configure_off_delay(uint16_t off_delay);

void occupancy_handle(const OccMessage *msg) {
    const char* topic = NULL;
    switch (msg->action)
    {
        case HEARTBEAT_UPDATE:
            topic = "/als/status/heartbeat"; 
            publish_message(msg, topic);
            break;

        case OCC_UPDATE:
            char topic_buf[64];
            snprintf(topic_buf, 
                sizeof(topic_buf), 
                "/als/status/occ/%u", 
                msg->payload.occ_update.room_id);
            topic = topic_buf;
            publish_message(msg, topic);
            break;

        case OCC_CONFIG_DELAY:
            ESP_LOGD(TAG, "Payload:");
            ESP_LOGD(TAG, "  Room ID: %u", msg->payload.config_delay.room_id);
            ESP_LOGD(TAG, "  Off Delay: %u", msg->payload.config_delay.off_delay);
            bool ok = false;
            int retries = 0;
            while (!ok && retries < 10) {
                ok = configure_off_delay(msg->payload.config_delay.off_delay);
                retries++;
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            if (!ok) ESP_LOGE(TAG, "Failed to configure off delay after %d retries", retries);

            break;

        default:
            ESP_LOGW(TAG, "Unhandled OCC action: %d", msg->action);
            break;
    }
}

static void publish_message(const OccMessage *msg, const char* topic) {
    char json_buf[256];

    if (topic == NULL) {
        ESP_LOGE(TAG, "No MQTT topic resolved");
        return;
    }

    if(!occ_serialize_message(msg, json_buf, sizeof(json_buf))) {
        ESP_LOGE(TAG, "Serialization Error");
        return;
    }
    int msg_id = mqtt_transport_publish(topic, json_buf);  
    ESP_LOGI(TAG, "Published status message sent, msg_id=%d", msg_id);
}

static void log_message_header(const OccMessage *msg) {
    if (msg)
    ESP_LOGI(TAG, "Message Header:");
    ESP_LOGI(TAG, "  Origin: %u", msg->origin);
    ESP_LOGI(TAG, "  Device: %u", msg->device);
    ESP_LOGI(TAG, "  Action: %u", msg->action);
}

static bool configure_off_delay(uint16_t off_delay) {
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
                         
    // Transmit Enable Config command
    uart_write_bytes(UART_NUM_1, EnableConfig, sizeof(EnableConfig));
    ESP_LOGI(TAG, "OFF-Delay message received\n");
   
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
    ESP_LOGD(TAG, "Prev. Minimum distance gate: %d\n", base_parameters_data[10]);
    // maximum distance gate
    ESP_LOGD(TAG, "Prev. Maximum distance gate: %d\n", base_parameters_data[11]-1);

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
        ESP_LOGW(TAG, "Config Base Length Does Not Match");
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