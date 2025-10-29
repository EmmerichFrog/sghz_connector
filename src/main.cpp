#include "main.h"
#include "http.h"
#include "radio.h"

#include <LittleFS.h>
#include <RadioLib.h>

extern CC1101 radio;

static Params params;

TaskHandle_t Task0;
TaskHandle_t Task1;

ICACHE_RAM_ATTR
void set_flag(void) { params.transmitted_flag = true; }

bool load_config() {
  bool ret = false;
  if (LittleFS.exists("/config.json")) {
    // file exists, reading and loading
    Serial.println("[main.cpp] reading config file");
    File configFile = LittleFS.open("/config.json", FILE_READ);
    if (configFile) {
      Serial.println("[main.cpp] opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);

      JsonDocument json;
      auto deserializeError = deserializeJson(json, buf.get());

      if (!deserializeError) {
        // HA entities
        JsonArray array = json["entities"].as<JsonArray>();
        for (JsonVariant entity : array) {
          params.entities.push_back(String((const char*)entity));
          Serial.print("[main.cpp] ");
          Serial.println((const char*)entity);
        }
        // Token
        params.token = (const char*)json["token"];
        // serializeJson(json, Serial);
        Serial.println("[main.cpp] parsed json");
        ret = true;
      } else {
        Serial.println("[main.cpp] failed to load json config");
      }

      configFile.close();
    }
  }
  return ret;
}

void setup() {
  Serial.begin(115200);

  if (!radio_init(params)) {
    while (true) {
      delay(1000);
    }
  }

  LittleFS.begin();
  setup_wifi(params.server_name);
  bool load_success = load_config();
  while (!load_success) {
    Serial.println("[main.cpp] Config Loading Failed!");
    delay(2000);
  }
  params.init_str =
      "00bt1111bh2222kt3333kh4444ot5555oh6666dhxoffadxoffco7777pm8888";

  params.queue = xQueueCreate(2, params.init_str.length() + 1);
  xQueueSend(params.queue, params.init_str.c_str(), portTICK_PERIOD_MS * 5000);

  xTaskCreate(sghz_send, "SGHZ", 1024 * 10, (void*)&params, 0, &Task0);
  xTaskCreate(ha_read, "HA", 1024 * 20, (void*)&params, 0, &Task1);
}

void loop() { vTaskDelete(NULL); }