#pragma once

struct RGB { 
  // 0-255 
  int r;
  int g;
  int b;
};
struct Settings {
  int seg_brightness; // 0-15 not much range // TODO actually use dynamically
  RGB battery_color;  
  RGB battery_charge_color;
  RGB battery_discharge_color; 
  long time;
};

extern Settings settings;

std::string printSettings();
void loadSettings(const char* jsonString);

