// Heavily based on demo1.ino https://github.com/RobTillaart/HT16K33
#include "config.h"
#include "HT16K33.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

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
HT16K33 seg4(0x73);
auto segs = {seg1, seg2}; // Not sure if fewer devices reduces flicker?

uint32_t start, stop;

WiFiClient espClient;
PubSubClient client(espClient);
String clientId;
String willTopic;

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

void checkComms(){
  if(WiFi.status() != WL_CONNECTED){
      while (WiFi.status() != WL_CONNECTED) {
          displayOff();
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
      displayOn();
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
  }
  if(!client.connected()){
      displayOff();
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
              subscribe("4");

              digitalWrite(LED_BUILTIN, LOW); 
          }else {
              Serial.print("failed, rc=");
              Serial.println(client.state());
          }
          delay(2000);
      }
      displayOn();
  }
  digitalWrite(LED_BUILTIN, LOW);
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

  //todo device from subtopic
  if(subtopic == "1"){
    //todo detect type (no `.`)
    auto val = content.toInt();
    seg1.displayInt(val);
  }else if(subtopic == "2"){
    //todo detect type (has `.`)
    auto val = content.toFloat();
    seg2.displayFloat(val,1);
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
  seg1.displayHex(0xA);
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
}

void loop()
{
  checkComms();
  client.loop();
  delay(1);
}