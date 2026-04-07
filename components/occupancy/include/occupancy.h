#pragma once
#include "protocol.h"

#define OccSensorInput_PIN (GPIO_NUM_18) // to recieve occupancy from sensor
#define TXD_PIN (GPIO_NUM_17) // uart Tx
#define RXD_PIN (GPIO_NUM_16) // uart Rx
#define RX_BUF_SIZE 256

void occupancy_handle(const OccMessage *msg);