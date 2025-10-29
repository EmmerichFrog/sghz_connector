# DESCRIPTION
ESP32 firmare allowing to parse the Home Assistant json for entities and send the data via CC1101

# HOW TO USE
- add your Home Assistant address in the config;
- populate the array "entities", each element is one entity in the format "<two_letter_key>:<ha_entity>", for example "bt:sensor.atc_f03f_temperature",
- add your login token to the config;
- in "http.cpp" add your https root certificate.

The data will be sent via the CC1101 radio in the format "two_letter_keyvalue", where value is the entity value converted to a 4 char string, for example "bt23.1"
