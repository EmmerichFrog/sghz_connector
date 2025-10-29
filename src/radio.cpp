#include "main.h"
#include "radio.h"

#include <RadioLib.h>

CC1101 radio = new Module(CS_PIN, IRQ_PIN, RADIOLIB_NC, GPIO_PIN);

bool radio_init(Params& params) {
  // initialize CC1101 with default settings
  params.transmission_state = RADIOLIB_ERR_NONE;
  Serial.print(F("[CC1101] Initializing ... "));
  int16_t state =
      radio.begin(FREQ, BITRATE, FREQ_DEV, RX_BW, TX_POWER, PREAMBLE_LEN);
  if (state == RADIOLIB_ERR_NONE) {
    // set the function that will be called
    // when packet transmission is finished
    radio.setPacketSentAction(set_flag);
    radio.setSyncWord(0x46, 0x4C);
    radio.setDataShaping(RADIOLIB_SHAPING_0_5);
    radio.setCrcFiltering(true);
    params.transmitted_flag = true;
    Serial.println("[radio.cpp] success!");

    return true;

  } else {
    Serial.print("[radio.cpp] failed, code ");
    Serial.println(state);

    return false;
  }
}

void sghz_send(void* parameter) {
  Params* params = (Params*)parameter;
  bool run = true;
  char str[params->init_str.length() + 1];
  char old_str[params->init_str.length() + 1];
  char count[] = "00";

  while (run) {
    if (params->transmitted_flag) {
      params->transmitted_flag = false;

      if (params->transmission_state == RADIOLIB_ERR_NONE) {
        Serial.println("\n[radio.cpp] transmission finished!");
      } else {
        Serial.print("\n[radio.cpp] failed, code: ");
        Serial.println(params->transmission_state);
      }

      radio.finishTransmit();

      delay(1000);  // I dont want the queue to block because I need to send the
                    // old buffer if it's not ready, but also I want to wait at
                    // least one second.

      if (xQueueReceive(params->queue, &str, portTICK_PERIOD_MS * 0) ==
          pdTRUE) {
        char count[] = "00";
        memccpy(count, str, NULL, 2);
        memccpy(old_str, str, NULL, sizeof(old_str));
        Serial.printf("[radio.cpp] Sending packet: %s\n\tPayload: %s", count,
                      str);
        params->transmission_state = radio.startTransmit(str);
      } else {
        params->transmission_state = radio.startTransmit(old_str);
        Serial.println(
            "[radio.cpp] Cannot read from queue, sending old buffer");
      }
    }
  }
}
