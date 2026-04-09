#pragma once

#include "esp_log.h"
#include "protocol.h"

void message_router_init(void);
// bool message_router_push_wire(const char *json);
bool message_router_push_local(CameraMessage* msg);
bool message_router_receive(CameraMessage* msg);