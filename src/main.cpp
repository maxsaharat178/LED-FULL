#include <Arduino.h>
#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "MCP23017.h"

MCP23017 mcp(0x20); // ที่อยู่ของ MCP23017 คือ 0x20



// การตั้งค่าเครือข่าย
const int num = 100;  // เปลี่ยนแค่ตัวนี้พอ

byte mac[6] = {0xFE, 0xED, 0xBA, 0xFA, 0xFA, 0x00}; // MAC address ของ Ethernet shield

// const char *BASE_ROOT   = "led_monitor/m2";
const char *mqtt_server = "49.0.64.55";
const int mqtt_port = 7535;
const char *clientId = "stepGW"; 
const char *mqtt_user   = "admin";
const char *mqtt_pass   = "Qwerty123?";

// พินของรีเลย์ที่เชื่อมต่อกับ MCP23017
const int mc_pins[] = {0, 1, 2, 3, 4, 5}; // แมกเนติก
const int relay_pins[] = {8, 9, 10};      // RELAY

// สถานะของรีเลย์
char r1 = 1, r2 = 1, r3 = 1;
char m1 = 0, m2 = 0, m3 = 0, m4 = 0, m5 = 0, m6 = 0;

// บัฟเฟอร์ MQTT
char fullClientId[20];
char subscribeTopic[32];
char ans[16];
char reponseBuf[256];

EthernetClient ethClient;
PubSubClient client(ethClient);

void statusGW() {
     snprintf(ans, sizeof(ans), "%s/%01X%01X%01X%01X%01X%01X%01X%01X%01X", "OK", m1, m2, m3, m4,m5,m6,r1,r2,r3);
     delay(50);
  client.publish(("step/SIMS1000TOR/" + String(num)).c_str(), ans);
  }

void LED() {
   mcp.write1(0, LOW);  // เปิด Relay 1 
    m1 = 1;
    client.loop();
    delay(10000);  // รอให้ Relay ถัดไปเปิด
   mcp.write1(1, LOW);  // เปิด Relay 2 หลังจาก 10 วินาที
    m2 = 1;
    client.loop();
    delay(10000);  // รอให้ Relay ถัดไปเปิด
    mcp.write1(2, LOW);  // เปิด Relay 3
    m3 = 1;
    client.loop();
    delay(10000);
    mcp.write1(3, LOW);  // เปิด Relay 1 
    m4 = 1;
    client.loop();
    delay(10000);  // รอให้ Relay ถัดไปเปิด
   mcp.write1(4, LOW);  // เปิด Relay 2 หลังจาก 10 วินาที
    m5 = 1;
    client.loop();
    delay(10000);  // รอให้ Relay ถัดไปเปิด
    mcp.write1(5, LOW);  // เปิด Relay 3
    m6 = 1;
    client.loop();
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
        statusGW();
      }
      else
      {
        mcp.write1(0, HIGH);
        mcp.write1(1, HIGH);
        mcp.write1(2, HIGH);
        mcp.write1(3, HIGH);
        mcp.write1(4, HIGH);
        mcp.write1(5, HIGH);
        
        m1 = 0;
        m2 = 0;
        m3 = 0;
        m4 = 0;
        m5 = 0;
        m6 = 0;
        statusGW();
      }
    }
    else if (relayData.containsKey("relay1"))
    {
      bool relayState = relayData["relay1"];
      if (relayState == true)
      {
        mcp.write1(8, LOW);
        r1 = 1;
        statusGW();
      }
      else
      {
        mcp.write1(8, HIGH);
        r1 = 0;
        statusGW();
      }
      
    }
    else if (relayData.containsKey("relay2"))
    {
      bool relayState = relayData["relay2"];
      if (relayState == true)
       {
        mcp.write1(10, LOW);
        r2 = 1;
        statusGW();
      }
      else
      {
        mcp.write1(10, HIGH);
        r2 = 0;
        statusGW();
      }
      
    }
    else if (relayData.containsKey("relay3"))
    {
      bool relayState = relayData["relay3"];
      if (relayState == true)
      {
        mcp.write1(9,LOW );
        r3 = 1;
        statusGW();
      }
      else
      {
        mcp.write1(9, HIGH);
        r3 = 0;
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
    if (client.connect(fullClientId, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.publish("step/device", subscribeTopic);  // ประกาศอุปกรณ์
      client.subscribe(subscribeTopic);  // รับการสมัครรับข้อมูล
    } else {
      Serial.println("Failed to connect to MQTT. Retrying...");
      delay(5000);
    }
  }
}

void setup() {
   mac[5] = num & 0xFF;
  snprintf(fullClientId, sizeof(fullClientId), "%s-%02X%02X%02X%02X%02X%02X", clientId, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  snprintf(subscribeTopic, sizeof(subscribeTopic), "%s/%02X%02X%02X%02X%02X%02X/cmd", clientId, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.begin(9600);
  Wire.begin();
  pinMode(PB12,OUTPUT);
 
// เริ่มต้นใช้งาน MCP23017
  if (!mcp.begin()) {
    Serial.println("MCP23017 init failed!");
    while (1) delay(1000);
  }
  Serial.println("MCP23017 init success!");
// ตั้งพิน 0 ถึง 5 เป็น OUTPUT
for (uint8_t i = 0; i < 6; i++) {        // mc_pins: 0..5
  mcp.pinMode1(mc_pins[i], OUTPUT);
  mcp.write1(mc_pins[i], HIGH);
  delay(10);
}


// ตั้งพิน 8 ถึง 10เป็น OUTPUT

  for (uint8_t i = 0; i < 3; i++) {        // relay_pins: {8,9,10}
  mcp.pinMode1(relay_pins[i], OUTPUT);
  mcp.write1(relay_pins[i], LOW);
  delay(10);
}
  LED();
  
  
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
if(r1==0){
  delay(3000);
 mcp.write1(8, LOW);
        r1 = 1;
      delay(100);
      statusGW();
} 
if(r2==0){
   delay(3000);
 mcp.write1(10, LOW);
        r2 = 1;
    delay(100);
      statusGW();
} 
if(r3==0){
   delay(3000);
 mcp.write1(9, LOW);
        r3 = 1;
        delay(100);
      statusGW();
} 

}
