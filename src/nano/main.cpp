// Heavily based on demo1.ino https://github.com/RobTillaart/HT16K33
#include "HT16K33.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <LedStatus.h>
#include <PubSubClient.h>
#include <Settings.h>

WiFiClient espClient;
PubSubClient client(espClient);
String clientId;

// TODO move to settings or status or somthing
int batteryLevel = 0;
int chargeRate = 0;

#define LED_PIN D4
#define LED_COUNT 10 // hardcoded assumptions on this being 10

// #region x
#ifndef WIFI_SSID
#error "WIFI_SSID must be defined in config"
#endif

#ifndef WIFI_PASS
#error "WIFI_PASS must be defined in config"
#endif

#ifndef BROKER
#error "BROKER must be defined in config"
#endif

#ifndef BROKER_PASS
#error "BROKER_PASS must be defined in config"
#endif

HT16K33 seg1(0x70);
HT16K33 seg2(0x71);
HT16K33 seg3(0x72);
auto segs = {seg1, seg2, seg3}; // Not sure if fewer devices reduces flicker?

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

String willTopic;

void subscribe(const char *sub_topic) {
  auto topic = String(TOPIC_BASE) + "/" + String(sub_topic);
  Serial.println("Subscribing to '" + topic + "'");
  client.subscribe(topic.c_str());
}

void displayOff() {
  for (auto seg : segs) {
    seg.displayOff();
  }
}
void displayOn() {
  for (auto seg : segs) {
    seg.displayOn();
    seg.displayClear();
  }
}

void displayOffline() {
  seg1.displayHex(0x1);
  seg2.displayHex(0x2);
  seg3.displayHex(0x3);
  strip.clear();
  for (int i = 0; i < 10; i++) {
    int c = 10 - i; // inverse brightness
    strip.setPixelColor(i, Adafruit_NeoPixel::Color(c, 0, c));
  }
  strip.show();
}

void publishCurrentSettings() {
  auto jsonString = printSettings();
  // TODO set 7seg brightness setting
  auto settings_topic = String(TOPIC_BASE) + "/" + clientId + "/settings";
  client.publish(settings_topic.c_str(), jsonString.c_str(), true);
}

void checkComms() {
  if (WiFi.status() != WL_CONNECTED) {
    while (WiFi.status() != WL_CONNECTED) {
      displayOffline();
      Serial.print("waiting for wifi, rc= ");
      Serial.println(WiFi.status());
      // while connecting flash led
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  if (!client.connected()) {
    displayOffline();
    Serial.print("MQTT connecting to ");
    Serial.print(BROKER);
    Serial.println(" ...");
    while (!client.connected()) {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.print(".");
      if (client.connect(clientId.c_str(), "use-token-auth", BROKER_PASS, willTopic.c_str(), 0, true, "offline")) {
        Serial.println("\nconnected");
        client.publish(willTopic.c_str(), "online", true);
        publishCurrentSettings();

        subscribe("generation");
        subscribe("generated_today");
        subscribe("consumption");
        subscribe("charge_level");
        subscribe("charge_rate");
        subscribe((clientId + "/settings").c_str());

        digitalWrite(LED_BUILTIN, LOW);
      } else {
        Serial.print("failed, rc=");
        Serial.println(client.state());
      }
      delay(1000);
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
}

void display7segValue(HT16K33 device, const String &content) {
  device.clearCache(); // displayClear() doesn't seem to actually clear the display
  if (content.indexOf(".") != -1) {
    auto val = content.toFloat();
    device.displayFloat(val, 1);
  } else {
    auto val = content.toInt();
    device.displayInt(val);
  }
}

int charge_indicator = 0;

uint32_t colorToSetting(const RGB &setting) { return Adafruit_NeoPixel::Color(setting.r, setting.g, setting.b); }

void displayRgb() {
  strip.clear();
  char *batterydata = buildLedData(batteryLevel, chargeRate);
  for (int i = 0; i < 10; i++) {
    uint32_t color;
    switch (batterydata[i]) {
    case 'C':
      color = colorToSetting(settings.battery_charge_color);
      break;
    case 'D':
      color = colorToSetting(settings.battery_discharge_color);
      break;
    case 'B':
      color = colorToSetting(settings.battery_color);
      break;
    // otherwise off
    case 'O':
    default:
      color = 0;
      // nothing
    }
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void callback(char *topic, byte *payload, unsigned int length) {
  char bytes[length + 1];
  bytes[length] = '\0';
  strncpy(bytes, reinterpret_cast<char *>(payload), length);
  String content = String(bytes);

  Serial.println("Message (" + String(length) + "b) arrived on: " + String(topic));
  Serial.println("> " + content);

  auto topic_s = String(topic);
  String subtopic = topic_s.substring(String(TOPIC_BASE).length() + 1, topic_s.length());

  if (subtopic == clientId + "/settings") {
    loadSettings(bytes);
  } else if (subtopic == "charge_level") {
    batteryLevel = content.toInt();
  } else if (subtopic == "charge_rate") {
    chargeRate = content.toInt();
  } else if (subtopic == "generation") {
    display7segValue(seg1, content); // TODO spit into set() and display()?
  } else if (subtopic == "generated_today") {
    display7segValue(seg2, content);
  } else if (subtopic == "consumption") {
    display7segValue(seg3, content);
  } else {
    Serial.println("!Unmapped topic: " + String(topic));
  }
  displayRgb();
}

void setupDevice(HT16K33 device, String name) {
  device.begin();
  auto addr = device.getAddress();
  Serial.print(String(name) + " address: ");
  Serial.println(addr);
  device.displayOn();
  device.setBrightness(settings.seg_brightness);
  device.displayClear();
}

void init_wire() {
  Wire.begin();
  Wire.setClock(100000); // 800000 still seems to flicker slightly
  int i = 0;
  for (auto seg : segs) {
    i++;
    setupDevice(seg, String(i));
  }
}

void init_strip() {
  strip.begin();
  // TODO maybe should make this configurable - it is insanely bright at 255 pixel level
  strip.setBrightness(255);
}

void init_network() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  clientId = WiFi.macAddress();
  client.setServer(BROKER, 1883);
  client.setCallback(callback);
  willTopic = String(TOPIC_BASE) + "/" + clientId + "/will";
  checkComms();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("WIFI configured to ");
  Serial.println(WIFI_SSID);
  init_wire();
  init_strip();
  displayOffline();
  init_network();
}

void loop() {
  checkComms();
  client.loop();
}
