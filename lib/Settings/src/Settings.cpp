#include <ArduinoJson.h>
#include "Settings.h"

Settings settings = {
    15, // led_brightness (0-15)
    {10,0,0},
    {0,10,0},
    {10,10,0},
};


std::string printSettings(){
  JsonDocument doc;
  doc["seg_brightness"] = settings.seg_brightness;
  doc["battery_color"][0] = settings.battery_color.r;
  doc["battery_color"][1] = settings.battery_color.g;
  doc["battery_color"][2] = settings.battery_color.b;
  doc["battery_charge_color"][0] = settings.battery_charge_color.r;
  doc["battery_charge_color"][1] = settings.battery_charge_color.g;
  doc["battery_charge_color"][2] = settings.battery_charge_color.b;
  doc["battery_discharge_color"][0] = settings.battery_discharge_color.r;
  doc["battery_discharge_color"][1] = settings.battery_discharge_color.g;
  doc["battery_discharge_color"][2] = settings.battery_discharge_color.b;
  doc["time"] = settings.time; //TODO not seen in publish, but also not getting infinite subscribe loop?

  std::string jsonString;
  jsonString.reserve(255); // Reserve enough space for the JSON string
  serializeJson(doc, jsonString);

#ifdef Arduino_h
  Serial.print("Publishing current settings: ");
  Serial.println(jsonString.c_str());
#endif
  
  return jsonString;
}

void setBatteryColors(JsonArray input, RGB& setting){
    if(input.size()==3){
        setting.r = input[0];
        setting.g = input[1];
        setting.b = input[2];
    }
}

void loadSettings(const char* content){
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, content);
  if (error) {
#ifdef Arduino_h
    Serial.print(F("load settings deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
#endif
  }

  if(settings.time == doc["time"]){
    return;
  }

#ifdef Arduino_h
    Serial.print("Loading settings from topic: ");
    Serial.println(content);
#endif

  settings.seg_brightness = doc["seg_brightness"] | 15;
  setBatteryColors(doc["battery_color"], settings.battery_color);
  setBatteryColors(doc["battery_charge_color"], settings.battery_charge_color);
  setBatteryColors(doc["battery_discharge_color"], settings.battery_discharge_color);
#ifdef Arduino_h
  settings.time = millis();
  Serial.println("Settings updated at: " + String(settings.time));
#else
  settings.time = rand();
#endif
}