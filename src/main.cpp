#include <Arduino.h>
#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "MCP23017.h"

MCP23017 mcp(0x20); // ที่อยู่ของ MCP23017 คือ 0x20

// การตั้งค่าเครือข่าย
byte mac[] = {0xAE, 0xED, 0xBA, 0xFA, 0xFA, 0x50};


const char *BASE_ROOT   = "led_monitor/m2";
const char *mqtt_server = "10.11.54.123";
const int mqtt_port = 1883;
const char *clientId = BASE_ROOT; 
const char *mqtt_user   = "mymqtt";
const char *mqtt_pass   = "P@ssw0rd4!";

// พินของรีเลย์ที่เชื่อมต่อกับ MCP23017
const int mc_pins[] = {0, 1, 2, 3, 4, 5}; // แมกเนติก
const int relay_pins[] = {8, 9, 10};      // RELAY

// สถานะของรีเลย์
char r1 = 0, r2 = 0, r3 = 0;
char m1 = 0, m2 = 0, m3 = 0, m4 = 0, m5 = 0, m6 = 0;
bool relaySoftStart = false;
unsigned long ledStartTime = 0;

// บัฟเฟอร์ MQTT
char fullClientId[20];
char subscribeTopic[32];
char ans[8];
char reponseBuf[256];

EthernetClient ethClient;
PubSubClient client(ethClient);

void statusGW() {
   snprintf(ans, sizeof(ans), "%s/%01X%01X%01X%01X%01X%01X%01X%01X%01X", "OK", m1, m2, m3, m4,m5,m6,r1,r2,r3);
  client.publish(BASE_ROOT, ans);
  }

void LED() {
   mcp.write1(8, HIGH);  // เปิด Relay 1 
    r1 = 1;
    delay(10000);  // รอให้ Relay ถัดไปเปิด
   mcp.write1(9, HIGH);  // เปิด Relay 2 หลังจาก 10 วินาที
    r2 = 1;
    delay(10000);  // รอให้ Relay ถัดไปเปิด
    mcp.write1(10, HIGH);  // เปิด Relay 3
    r3 = 1;
    delay(1000);
    statusGW();
    }

    
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);
  JsonObject relayData = doc.as<JsonObject>();


  if (strcmp(topic, subscribeTopic) == 0)
  {
     if (relayData.containsKey("ledRelay"))
    {
      bool relayState = relayData["ledRelay"];
      if (relayState == true)
      {
        LED();
        
      }
      else
      {
        mcp.write1(8, LOW);
        mcp.write1(9, LOW);
        mcp.write1(10, LOW);
        r1 = 0;
        r2 = 0;
        r3 = 0;
        statusGW();
      }
    }
    else if (relayData.containsKey("relay3"))
    {
      bool relayState = relayData["relay3"];
      if (relayState == true)
      {
        mcp.write1(10, HIGH);
        r3 = 1;
        statusGW();
      }
      else
      {
        mcp.write1(10, LOW);
        r3 = 0;
        statusGW();
      }
    }
    else if (relayData.containsKey("relay4"))
    {
      bool relayState = relayData["relay4"];
      if (relayState == true)
      {
        mcp.write1(11, LOW); 
        r4 = 1;
        statusGW();
      }
      else
      {
         mcp.write1(11, HIGH);
         r4 =0;
         statusGW();
      }
    }
    else if (relayData.containsKey("status"))
    {
      bool relayState = relayData["status"];
      if (relayState == true)
      {
       statusGW();
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(BASE_ROOT, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.publish(BASE_ROOT, subscribeTopic);  // ประกาศอุปกรณ์
      client.subscribe(subscribeTopic);  // รับการสมัครรับข้อมูล
    } else {
      Serial.println("Failed to connect to MQTT. Retrying...");
      digitalWrite(PB12,1);
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(PB12,OUTPUT);
// เริ่มต้นใช้งาน MCP23017
  if (!mcp.begin()) {
    Serial.println("MCP23017 init failed!");
    while (1) delay(1000);
  }
  Serial.println("MCP23017 init success!");
// ตั้งพิน 8 ถึง 11 เป็น OUTPUT
  for (int i = 0; i < 4; i++) {
    mcp.pinMode1(relay_pins[i], OUTPUT);
    mcp.write1(relay_pins[i], LOW);  // เริ่มต้นรีเลย์ทั้งหมดเป็น OFF
  }
  LED();
  snprintf(fullClientId, sizeof(fullClientId), "%s-%02X%02X%02X%02X%02X%02X", clientId, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  snprintf(subscribeTopic, sizeof(subscribeTopic), "%s/cmd", BASE_ROOT);
  
  // เริ่มต้นเครือข่าย Ethernet
  Ethernet.init(PA4);
  Ethernet.begin(mac);
  delay(1000);  // ให้เวลาในการตั้งค่า Ethernet

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  delay(1000);  // ให้เวลาในการตั้งค่า Ethernet
 
  // เริ่มใช้งาน MQTT
  reconnect();
  statusGW();
}

void loop() {
 
  
if (!client.connected()) {
    reconnect();
  }
  client.loop();  // ให้ MQTT client ทำงาน
}
