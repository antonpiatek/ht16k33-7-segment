// Heavily based on demo1.ino https://github.com/RobTillaart/HT16K33
#include "config.h"
#include "HT16K33.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "config.h"

#include <LedStatus.h>
#include <Settings.h>

WiFiClient espClient;
PubSubClient client(espClient);
String clientId;

int batteryLevel = 0;
int chargeRate = 0;

String settings_topic_cmnd;

#define _TASK_SLEEP_ON_IDLE_RUN  // Enable 1 ms SLEEP_IDLE powerdowns between runs if no callback methods were invoked during the pass
#define _TASK_STATUS_REQUEST     // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
#include <TaskScheduler.h>



#define LED_PIN D4
#define LED_COUNT 10 // hardcoded assumptions on this being 10

//#region x
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

uint32_t start, stop;

String willTopic;

Scheduler ts;
#define PERIOD3 3000
void displayRgb();
Task tBlink3 (PERIOD3, -1, &displayRgb, &ts, true);
void publishCurrentSettings();

void subscribe(const char* sub_topic, bool useBase)
{
  String topic;
  if(useBase){
    topic = String(TOPIC_BASE)+"/"+String(sub_topic);
  }else{
    topic = String(sub_topic);
  }
  
  Serial.println("Subscribing to '" + topic + "'");
  client.subscribe(topic.c_str());
}

void subscribe(const char* sub_topic){
  subscribe(sub_topic, true);
}


void displayOff(){
  for (auto seg : segs){
    seg.displayOff();
  }
}
void displayOn(){
  for (auto seg : segs){
    seg.displayOn();
    seg.displayClear();
  }
}

void displayOffline(){
  seg1.displayHex(0x1);
  seg2.displayHex(0x2);
  seg3.displayHex(0x3);
  strip.clear();
  for(int i=0;i<10;i++){
    int c = 10-i;//inverse brightness
    strip.setPixelColor(i,strip.Color(c,0,c));
  }
  strip.show();
}

void checkComms(){
  if(WiFi.status() != WL_CONNECTED){
      while (WiFi.status() != WL_CONNECTED) {
          displayOffline();
          Serial.print("waiting for wifi, rc= ");
          Serial.println(WiFi.status());
          // while connecting flash led
          digitalWrite(LED_BUILTIN, HIGH); 
          delay(500);
          digitalWrite(LED_BUILTIN, LOW); 
          delay(500);
          digitalWrite(LED_BUILTIN, HIGH); 
          delay(500);
          digitalWrite(LED_BUILTIN, LOW); 
          delay(500);
      }
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
  }
  if(!client.connected()){
      displayOffline();
      Serial.print("MQTT connecting to ");
      Serial.print(BROKER);
      Serial.println(" ...");
      while (!client.connected()) {
          digitalWrite(LED_BUILTIN, HIGH); 
          Serial.print(".");
          if (client.connect(clientId.c_str(), "use-token-auth", BROKER_PASS, willTopic.c_str(), 0, 1, "offline")) {
              Serial.println("\nconnected");
              client.publish(willTopic.c_str(), "online", true);
              publishCurrentSettings();

              subscribe("1");
              subscribe("2");
              subscribe("3");
              subscribe("rgb");
              subscribe("battery_charge");
              subscribe(settings_topic_cmnd.c_str(), false);

              digitalWrite(LED_BUILTIN, LOW); 
          }else {
              Serial.print("failed, rc=");
              Serial.println(client.state());
          }
          delay(1000);
      }  }
  digitalWrite(LED_BUILTIN, LOW);
}

void display7segValue(HT16K33 device, String content)
{
  device.clearCache(); // displayClear() doesn't seem to actually clear the display
  bool isFloat = content.indexOf(".") == -1 ? false : true;
  if(isFloat){
    auto val = content.toFloat();
    device.displayFloat(val,1);
  }else{
    auto val = content.toInt();
    device.displayInt(val);
 }
}

int charge_indicator=0;

uint32_t colorToSetting(RGB setting){
  return strip.Color(setting.r,setting.g,setting.b);
}

//TODO do on events rather than timer?
void displayRgb(){
  strip.clear();
  char* batterydata = buildLedData(batteryLevel, chargeRate);
  for(int i=0;i<10;i++) {
    uint32_t color;
    switch (batterydata[i])
    {
      case 'C':
        color = colorToSetting(settings.battery_charge_color);
        break;
      case 'D':
        color = colorToSetting(settings.battery_discharge_color);
        break;
      case 'B':
        color = colorToSetting(settings.battery_color);
        break;
      //otherwise off
      case 'O':
      default:
        color=0;
        //nothing
    }
    strip.setPixelColor(i,color);
  }
  strip.show();
}

void initSettingsTopics(){
  settings_topic_cmnd = String(TOPIC_BASE)+"/cmnd/"+clientId+"/settings";
}

void publishCurrentSettings(){
  auto jsonString = printSettings();
  //TODO set 7seg brightness setting
  client.publish(settings_topic_cmnd.c_str(), jsonString.c_str(), true);
}



void callback(char* topic, byte* payload, unsigned int length) {
  char bytes[length+1];
  bytes[length] = '\0';
  strncpy(bytes, (char*)payload, length);
  String content = String(bytes);
  
  Serial.println("Message ("+String(length)+"b) arrived on: "+String(topic));
  Serial.println("> "+content);
  
  auto topic_s = String(topic);
  String subtopic = topic_s.substring(String(TOPIC_BASE).length()+1, topic_s.length());
    
  if(subtopic == "rgb"){
    batteryLevel = content.toInt();
  }else if(subtopic == "battery_charge"){
    chargeRate = content.toInt();
  }else if (topic_s == settings_topic_cmnd){
    loadSettings(bytes);
  }else if(subtopic.length() == 1){
    int segIndex = subtopic.toInt() - 1;
    if (segIndex < 0 || segIndex+1 > static_cast<int>(segs.size())) {
      Serial.println("!Unmapped topic: "+String(topic));
      return;
    }
    display7segValue((HT16K33&)segs.begin()[segIndex], content);
  }else{
    Serial.println("!Unmapped topic: "+String(topic));
  }
  
}

void setupDevice(HT16K33 device, String name)
{
  device.begin();
  auto addr = device.getAddress();
  Serial.print(String(name)+" address: ");
  Serial.println(addr);
  device.displayOn();
  device.setBrightness(settings.seg_brightness);
  device.displayClear();
}

void init_wire(){
  Wire.begin();
  Wire.setClock(100000); // 800000 still seems to flicker slightly
  int i = 0;
  for (auto seg : segs){
    i++;    
    setupDevice(seg, String(i));
  }
}

void init_strip(){
  strip.begin();
  //TODO maybe should make this configurable - it is insanely bright at 255 pixel level
  strip.setBrightness(255);
}

void init_network()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  clientId = WiFi.macAddress();
  client.setServer(BROKER, 1883);
  initSettingsTopics();
  client.setCallback(callback);
  willTopic = String(TOPIC_BASE)+"/status/"+clientId+"/will";
  checkComms();
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("WIFI configured to ");
  Serial.println(WIFI_SSID);
  init_wire();
  init_strip();
  displayOffline();
  init_network();
}

void loop()
{
  checkComms();
  client.loop();
  ts.execute();
}

