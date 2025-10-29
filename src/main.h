#pragma once
#include <list>
#include <Arduino.h>

struct Params {
  String token;
  String server_name;
  std::list<String> entities;
  QueueHandle_t queue;
  volatile bool transmitted_flag;
  int16_t transmission_state;
  String init_str;
};

void set_flag(void);