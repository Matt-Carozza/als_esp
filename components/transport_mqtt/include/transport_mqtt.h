#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void mqtt_transport_start(void);
bool mqtt_transport_is_connected(void);
int mqtt_transport_publish(const char* topic, const char* paylaod);
void mqtt_transport_set_app_task(TaskHandle_t handle);