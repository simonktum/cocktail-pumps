#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncMqtt_Generic.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>

const char* ssid = "ssid";
const char* password = "password";
const char* mqttServer = "mqttServer";
const int mqttPort = 1;
const char* mqttTopic = "cocktail/pumpen";
const int pins[8] = { 27, 14, 26, 13, 33, 5, 18, 19 };

AsyncMqttClient mqttClient;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("Connected to Wi-Fi. Connecting to MQTT...");
      connectToMqtt();
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("Disconnected from Wi-Fi. Reconnecting...");
      connectToWifi();
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  uint16_t packetIdSub = mqttClient.subscribe(mqttTopic, 2);
  Serial.print("Subscribing at QoS 2: ");
  Serial.println(mqttTopic);
  Serial.print("packetId: ");
  Serial.println(packetIdSub);
}

void setPinState(const char* payload, int state) {
  for (int i = 0; i < 8; i++) {
    char pinStr[2];
    itoa(i, pinStr, 10);
    if (strstr(payload, pinStr) != NULL) {
      Serial.print(i);
      Serial.println(state == HIGH ? " -- HIGH" : " -- LOW");
      digitalWrite(pins[i], state);
    }
  }
}

void activateInBurst(const char* payload, int state) {
  using namespace std::this_thread;
  using namespace std::chrono;
  setPinState(payload, LOW);
  sleep_for(milliseconds(1000));
  setPinState(payload, HIGH);
}

void activateWithTime(const char* payload, int time) {
  using namespace std::this_thread;
  using namespace std::chrono;
  int i = time * 1000;
  setPinState(payload, LOW);
  sleep_for(milliseconds(i));
  setPinState(payload, HIGH);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  //Inverted Relais!!!!!!
  Serial.print("Received: ");
  Serial.println(payload);
  String tmp = String(payload);
  tmp = tmp.substring(0, tmp.indexOf("-")+1); //need to cut of the stuff sent afterwards
  char const* instruction = tmp.c_str();
  Serial.print("Cut down String: ");
  Serial.println(instruction);
  if (strstr(instruction, "OFF") != NULL) {
    setPinState(instruction, HIGH);
  } else if (strstr(instruction, "ON") != NULL) {
    setPinState(instruction, LOW);
  } else if (strstr(instruction, "BURST") != NULL) {
    activateInBurst(instruction, LOW);
  } else if (strstr(instruction, "TIMED") != NULL) {
  tmp = String(instruction);
  String tmp_instructions = tmp.substring(0, tmp.indexOf("AS"));
  tmp = tmp.substring(tmp.indexOf("AS") + 2, tmp.indexOf("-"));
  char const* time = tmp.c_str();
  std::stringstream strValue;
  strValue << time;
  unsigned int intValue;
  strValue >> intValue;
  instruction = tmp_instructions.c_str();
    activateWithTime(instruction, intValue);
  }
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 8; i++) {
    pinMode(pins[i], OUTPUT);
  }
  for (int i = 0; i < 8; i++) {
    digitalWrite(pins[i], HIGH);
  }
  WiFi.onEvent(WiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(mqttServer, mqttPort);
  connectToWifi();
}

void loop() {
}
