#pragma once
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <ArduinoHttpClient.h>

#define DEFAULT_PW "password"
#define HA_ENDPOINT "/api/states"
#define HA_SERVER_KEY "ha"

void setup_wifi(String& server_name);
bool send_get(HTTPClient& client, JsonDocument& json);
String get_entity_value(JsonDocument& doc, String name);
void ha_read(void* parameter);