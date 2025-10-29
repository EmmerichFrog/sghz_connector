#pragma once
#include <HTTPClient.h>

#define FREQ 433.92F
#define BITRATE 9.996F
#define FREQ_DEV 19.042969F
#define RX_BW 58.0F
#define TX_POWER 10
#define PREAMBLE_LEN 16U
#define CS_PIN 5
#define IRQ_PIN 14
#define GPIO_PIN 27

bool radio_init(Params& params);
void sghz_send(void* parameter);