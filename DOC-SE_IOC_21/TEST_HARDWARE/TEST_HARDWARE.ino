#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define ETH_CS 14
#define ETH_SCLK 18
#define ETH_MISO 19
#define ETH_MOSI 23

#define OUT1 25
#define OUT2 26
#define OUT3 27
#define OUT4 14
#define OUT5 15
#define OUT6 2
#define INPUT1 36
#define INPUT2 39
#define INPUT3 34
#define INPUT4 35

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

const char* mqtt_broker = "192.168.0.173";
const char* mqtt_topic = "esp32/test";
const char* mqtt_username = "";
const char* mqtt_password = "public";
const int mqtt_port = 1883;

EthernetClient ethClient;
PubSubClient client(ethClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("ESP32 reconnected to MQTT Broker");
    if (client.connect("ESP32", mqtt_username, mqtt_password)) {
      Serial.println("-connected");
      client.subscribe(mqtt_topic);
      client.setCallback(callback);
      client.publish("so1/", "ESP32 reconnected");
    } else {
      Serial.print("-failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
void setup() {
  // put your setup code here, to run once:
  delay(5000);
  Wire.begin();
  Serial.begin(115200);
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  pinMode(OUT4, OUTPUT);
  pinMode(OUT5, OUTPUT);
  pinMode(OUT6, OUTPUT);
  pinMode(INPUT1, INPUT);
  pinMode(INPUT2, INPUT);
  pinMode(INPUT3, INPUT);
  pinMode(INPUT4, INPUT);
  digitalWrite(OUT1, LOW);
  digitalWrite(OUT2, LOW);
  digitalWrite(OUT3, LOW);
  digitalWrite(OUT4, LOW);
  digitalWrite(OUT5, LOW);
  digitalWrite(OUT6, LOW);
  Serial.println("\nI2C Scanner");
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknow error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
  }
  while (!client.connected()) {
    Serial.print("The client connects to the public mqtt broker\n");
    if (client.connect("esp32", mqtt_username, mqtt_password)) {
      Serial.println("Public emqx mqtt broker connected");
      client.subscribe(mqtt_topic);
      client.setCallback(callback);
      client.publish("so1/", "xin chao");
    } else {
      reconnect();
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(INPUT1) == 0 || digitalRead(INPUT2) == 0 || digitalRead(INPUT3) == 0 || digitalRead(INPUT4) == 0) {
    digitalWrite(OUT1, LOW);
    digitalWrite(OUT2, LOW);
    digitalWrite(OUT3, LOW);
    digitalWrite(OUT4, LOW);
    digitalWrite(OUT5, LOW);
    digitalWrite(OUT6, LOW);
  } else {
    digitalWrite(OUT1, !digitalRead(OUT1));
    digitalWrite(OUT2, !digitalRead(OUT2));
    digitalWrite(OUT3, !digitalRead(OUT3));
    digitalWrite(OUT4, !digitalRead(OUT4));
    digitalWrite(OUT5, !digitalRead(OUT5));
    digitalWrite(OUT6, !digitalRead(OUT6));
    delay(1000);
  }
  client.loop();
  Ethernet.maintain();
  if (!client.connected()) reconnect();
}
