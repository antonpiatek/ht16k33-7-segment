// Heavily based on demo1.ino https://github.com/RobTillaart/HT16K33
#include "config.h"
#include "HT16K33.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#define _TASK_SLEEP_ON_IDLE_RUN  // Enable 1 ms SLEEP_IDLE powerdowns between runs if no callback methods were invoked during the pass
#define _TASK_STATUS_REQUEST     // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
#include <TaskScheduler.h>

#define LED_PIN D4
#define LED_COUNT 10 // hardcoded assumptions on this being 10
#define LED_BRIGHTNESS 10 // out of 255

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

WiFiClient espClient;
PubSubClient client(espClient);
String clientId;
String willTopic;

int rgbLevel = 0;
int chargeRate = 0;

Scheduler ts;
#define DURATION 10000
#define PERIOD3 50
void displayRgb();
Task tBlink3 (PERIOD3, -1, &displayRgb, &ts, true);

void subscribe(const char* sub_topic)
{
  String topic = String(TOPIC_BASE)+"/"+String(sub_topic);
  Serial.println("Subscribing to '" + topic + "'");
  client.subscribe(topic.c_str());
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
              client.publish (willTopic.c_str(), "online", true);

              subscribe("1");
              subscribe("2");
              subscribe("3");
              subscribe("rgb");
              subscribe("battery_charge");

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

void displayRgb(){
  strip.clear();
  int max_color = 150;
  uint32_t color = strip.Color(max_color,0,0); // Red
  int litCount =(int)round(rgbLevel/10);
  for(int i=0;i<10;i++) {
    if(i < litCount){
      strip.setPixelColor(i,color);
    }
    // led brightness range is not amazing so not sure this is worth it
    if(i == litCount){;
      int pixel_pct = round(rgbLevel % 10 / 10.0 * max_color);
      uint32_t color = strip.Color(pixel_pct,0,0); // Red
      strip.setPixelColor(i,color);
    }
  }
  //tBlink3.getRunCounter() %1
  if(chargeRate > 0){
    auto fade = tBlink3.getRunCounter() %20;
    auto pulse = fade < 10? fade/10.0*max_color : (20-fade)/10.0*max_color;
    uint32_t color = strip.Color(0,pulse,0); // Green
    for(int i=0; i < chargeRate; i++) {
        int led = i + litCount+1;
        if(led >= LED_COUNT) break; // don't overflow the strip
        strip.setPixelColor(led,color);
    }
  }
  strip.show();
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
    rgbLevel = content.toInt();
  }else if(subtopic == "battery_charge"){
    chargeRate = content.toInt();
  }else{
    int segIndex = subtopic.toInt() - 1;
    if (segIndex < 0 || segIndex+1 > static_cast<int>(segs.size())) {
      Serial.println("!Unmapped topic: "+String(topic));
      return;
    }
    display7segValue((HT16K33&)segs.begin()[segIndex], content);
  }
  
}

void setupDevice(HT16K33 device, String name)
{
  device.begin();
  auto addr = device.getAddress();
  Serial.print(String(name)+" address: ");
  Serial.println(addr);
  device.displayOn();
  //device.setBrightness(255);// not much range
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
  seg1.setBrightness(15);
  seg2.setBrightness(2);
  seg2.setBrightness(1);
}

void init_strip(){
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  
  // seems to do nothing, perhaps pin2 is being used for something else during wifi setup?
  uint32_t color = strip.Color(0,0,10);
  for(int i=0;i<LED_COUNT;i++) {
      strip.setPixelColor(i,color);
  }
  
}

void init_network()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  clientId = WiFi.macAddress();
  client.setServer(BROKER, 1883);
  client.setCallback(callback);
  willTopic = String(TOPIC_BASE)+"/status";
  checkComms();
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("WIFI configured to ");
  Serial.println(WIFI_SSID);
  init_wire();
  init_network();
  init_strip();
}

void loop()
{
  checkComms();
  client.loop();
  ts.execute();
}

