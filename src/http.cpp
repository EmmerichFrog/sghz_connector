#include "main.h"
#include "http.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiManager.h>

// Default settings for mqtt
char ha_server[64] = "set default endpoint here";

const char* rootCACertificate = "Add your certificate here";

String get_entity_value(JsonDocument& doc, String name) {
  String ret = "";
  Serial.print("[http.cpp] ");
  Serial.print(name);
  Serial.print(": ");
  for (JsonObject obj : doc.as<JsonArray>()) {
    String entity_name = (const char*)obj["entity_id"];
    if (entity_name == name) {
      ret = (const char*)obj["state"];
      Serial.println(ret);
      break;
    }
  }

  return ret;
}

bool needSave = false;
// callback notifying us of the need to save config
void saveConfigCallback() { needSave = true; }

void setup_wifi(String& server_name) {
  Serial.println("[http.cpp] mounted file system");
  if (LittleFS.exists("/config.json")) {
    // file exists, reading and loading
    Serial.println("[http.cpp] reading config file");
    File configFile = LittleFS.open("/config.json", FILE_READ);
    if (configFile) {
      Serial.println("[http.cpp] opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);

      JsonDocument json;
      auto deserializeError = deserializeJson(json, buf.get());
      if (!deserializeError) {
        strcpy(ha_server, json[HA_SERVER_KEY]);
        Serial.print("[http.cpp] Parsed api endpoint: ");
        Serial.println(ha_server);
      } else {
        Serial.println("[http.cpp] failed to load json config");
      }
      configFile.close();
    }
  }
  // end read

  // value id/name placeholder/prompt default length
  WiFiManagerParameter custom_ha_server("server", "Home Assistant server",
                                        ha_server, 64);
  WiFiManager wm;
  // wm.resetSettings();
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.addParameter(&custom_ha_server);
  bool res = wm.autoConnect("AutoConnectAP_CO2", DEFAULT_PW);

  strcpy(ha_server, custom_ha_server.getValue());
  Serial.println("[http.cpp] Loaded from file: ");
  Serial.println("\t- ha_server : " + String(ha_server));
  Serial.print("[http.cpp] local ip: ");
  Serial.println(WiFi.localIP());

  if (needSave) {
    Serial.println("[http.cpp] saving config");
    JsonDocument json;

    json[HA_SERVER_KEY] = ha_server;

    File configFile = LittleFS.open("/config.json", FILE_WRITE, true);
    if (!configFile) {
      Serial.println("[http.cpp] failed to open config file for writing");
    }
    serializeJson(json, configFile);
    configFile.close();
  }

  server_name = String(ha_server) + String(HA_ENDPOINT);
  Serial.printf("[http.cpp] Endpoint: %s\n", server_name.c_str());
}

bool send_get(HTTPClient& client, JsonDocument& json) {
  bool ret = false;

  if (WiFi.status() == WL_CONNECTED) {
    int httpResponseCode = client.GET();

    if (httpResponseCode > 0 && httpResponseCode < 400) {
      Serial.print("[http.cpp] HTTP Response code: ");
      Serial.println(httpResponseCode);
      deserializeJson(json, client.getStream());
      Serial.println("[http.cpp] Deserialized.");
      Serial.printf("[http.cpp] Payload size: %u\n", client.getSize());
      ret = true ? client.getSize() > 0 : false;
    } else {
      Serial.print("[http.cpp] Arduino HTTP error code: ");
      Serial.println(httpResponseCode);
      ret = false;
    }
  }

  return ret;
}
/**
 * make sure all values are 4 character long
 * @param buf char buffer to write the 4 char long value
 * @param value String to limit
 * @return 4 character long string
 */
char* to_4_char(char* buf, String value) {
  if (value != "unavailable") {
    String ret = value.substring(
        0, 4);  // since this is temp/hum, only floats are longer than 4
                // character, at worst this will cut the last decimal
    while (ret.length() < 4) {
      ret = "0" + ret;
    }
    snprintf(buf, 5, ret.c_str());
  } else {
    snprintf(buf, 5, "N/A ");
  }
  return buf;
}

void ha_read(void* parameter) {
  Params* params = (Params*)parameter;
  HTTPClient client;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  client.begin(params->server_name, rootCACertificate);
  client.addHeader("Content-Type", "application/json");

  char buffer[512];
  snprintf(buffer, sizeof(buffer), "%s %s", "Bearer", params->token.c_str());
  client.addHeader("Authorization", buffer);

  uint8_t count = 0;
  bool success;
  JsonDocument json;
  size_t str_size = params->init_str.length() + 1;
  char str[str_size];
  char buf_4_char[5];
  while (true) {
    if (send_get(client, json)) {
      if (count > 99) {
        count = 0;
      }
      count++;
      snprintf(str, str_size, "%02u", count);
      size_t cnt = 2;
      for (auto entity : params->entities) {
        snprintf(
            &str[cnt], str_size - cnt, "%s%s", entity.substring(0, 2),
            to_4_char(buf_4_char, get_entity_value(json, entity.substring(3))));
        cnt += 6;
      }
      xQueueSend(params->queue, str, portTICK_PERIOD_MS * 5000);
    } else {
      Serial.println("[http.cpp] Error with get request");
    }
    delay(1000);
  }
}
