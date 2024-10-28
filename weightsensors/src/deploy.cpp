/*
   Code skeleton for weight measurements acquired from:
      https://github.com/olkal/HX711_ADC/blob/master/examples/Read_1x_load_cell_interrupt_driven/Read_1x_load_cell_interrupt_driven.ino
   WiFi and MQTT code skeleton acquired from:
      https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/

*/

#include <HX711_ADC.h>
#include <EEPROM.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <string>
#include <WiFi.h>
#include <PubSubClient.h>

const int numer_of_loadcells = 6;

const int HX711_dout = 4; // microcontroller unit > HX711 dout pin, must be external interrupt capable!
const int HX711_sck = 16; // microcontroller unit > HX711 sck pin

// additional load cells
const int LC1_dout = 2;
const int LC1_sck = 15;

const int LC2_dout = 5;
const int LC2_sck = 17;

const int LC3_dout = 19;
const int LC3_sck = 18;

const int LC4_dout = 32;
const int LC4_sck = 33;

const int LC5_dout = 12;
const int LC5_sck = 14;

// HX711 constructor:
HX711_ADC LoadCell[numer_of_loadcells] = {HX711_ADC(HX711_dout, HX711_sck), HX711_ADC(LC1_dout, LC1_sck), HX711_ADC(LC2_dout, LC2_sck), HX711_ADC(LC3_dout, LC3_sck), HX711_ADC(LC4_dout, LC4_sck), HX711_ADC(LC5_dout, LC5_sck)};

float calibrationValue = 451.22;
float calibrationValueLC1 = 743.72;
float calibrationValueLC2 = 392.59;
float calibrationValueLC3 = -118.45;
float calibrationValueLC4 = 410.49;
const int calVal_eepromAdress = 0;
bool useEEPROM = false;
bool serviceEnabled = true;

unsigned long t = 0;
const int serialPrintInterval = 100;
volatile boolean newDataReady;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
bool enableBootDisplay = true;

// Replace the next variables with your SSID/Password combination
const char *ssid = "Cocktail_Mixer";
const char *password = "process_hubby";

IPAddress server(131, 159, 6, 111);
const int port = 1883;
const std::string sensorID[] = {"1111", "1112", "1113", "1114", "1115", "1116"};

// Weight displayed on the display
int displayedWeight[numer_of_loadcells] = {0, 0, 0, 0, 0, 0};
// Weight Measured in the previous iteration
int lastWeight[numer_of_loadcells] = {0, 0, 0, 0, 0, 0};
bool firstMeasurement[numer_of_loadcells] = {true, true, true, true, true, true};

WiFiClient espClient;
PubSubClient client(espClient);

void displaySensorIDBottom()
{
  u8g2.setCursor(30, 60);
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.print("Sensor ID: ");
  u8g2.print(sensorID[0].c_str());
}

void displayReconnectMessage()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(0, 15);
  u8g2.print("Disconnected");
  u8g2.setCursor(0, 40);
  u8g2.print("Reconnecting...");
  displaySensorIDBottom();
  u8g2.sendBuffer();
}

// interrupt routine:
void dataReadyISR()
{
  if (LoadCell[0].update() || LoadCell[1].update() || LoadCell[2].update() || LoadCell[3].update() || LoadCell[4].update() || LoadCell[5].update())
  {
    newDataReady = 1;
  }
}

void setupLoadcell()
{
  if (useEEPROM)
  {
    EEPROM.begin(512);
    EEPROM.get(calVal_eepromAdress, calibrationValue);
  }
  Serial.print("Using Calibration value:");
  Serial.println(calibrationValue);

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  for (size_t i = 0; i < numer_of_loadcells; i++)
  {
    LoadCell[i].begin();
    LoadCell[i].start(stabilizingtime, _tare);
    if (LoadCell[i].getTareTimeoutFlag())
    {
      Serial.print("Timeout, check wiring to HX711 and pin designations for LoadCell ");
      Serial.println(i);
      while (true)
        ;
    }
    else
    {
      LoadCell[i].setCalFactor(calibrationValue);
      Serial.print("Startup for LoadCell ");
      Serial.print(i);
      Serial.println(" is complete");
    }
  }

  attachInterrupt(digitalPinToInterrupt(HX711_dout), dataReadyISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(LC1_dout), dataReadyISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(LC2_dout), dataReadyISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(LC3_dout), dataReadyISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(LC4_dout), dataReadyISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(LC5_dout), dataReadyISR, FALLING);
}

void setupDisplay()
{
  u8g2.begin();
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(" Message: ");
  std::string messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (messageTemp.rfind("TARE", 0) == 0) // check if it starts with TARE
  {
    Serial.println("TARE command found");
    for (size_t i = 0; i < numer_of_loadcells; i++)
    {
      LoadCell[i].tareNoDelay();
      Serial.print("Completed tare for: ");
      Serial.println(i);
    }
  }
}

void setupMQTT()
{
  setup_wifi();
  client.setServer(server, port);
  client.setCallback(callback);
}

void displayBoot()
{
  u8g2.setCursor(20, 45);
  u8g2.setFont(u8g2_font_ncenB24_tr);
  u8g2.print("TUM");
  u8g2.sendBuffer();
  sleep(1);
}

void setup()
{
  Serial.begin(9600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  setupLoadcell();
  setupDisplay();
  if (enableBootDisplay)
  {
    displayBoot();
  }
  if (serviceEnabled)
  {
    setupMQTT();
  }
}

void displayWeight(size_t i)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.setCursor(0, 15);
  u8g2.print("Current Weight of Load Cell");
  u8g2.print(i);
  u8g2.print(": ");
  u8g2.setCursor(0, 40);
  u8g2.print(displayedWeight[i]);
  u8g2.print(" grams");
  u8g2.println("");
  displaySensorIDBottom();
  u8g2.sendBuffer();
}

int gearState = 0;
void displayMeasuring()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.setCursor(0, 15);
  u8g2.print("Measuring");
  u8g2.setCursor(0, 40);
  u8g2.print("Weight  ");

  switch (gearState)
  {
  case 0:
    u8g2.print("/");
    break;
  case 1:
    u8g2.print("-");
    break;
  case 2:
    u8g2.print("\\");
    break;
  }
  gearState = ++gearState % 3;
  displaySensorIDBottom();
  u8g2.sendBuffer();
}

void publishWeightToMQTT(size_t i)
{
  std::string topic = "cocktail/weight/sensor_";
  topic = topic + sensorID[i];
  client.publish(topic.c_str(), reinterpret_cast<uint8_t *>(&displayedWeight[i]), sizeof(float));
}

void handleNewWeightData(size_t i)
{
  newDataReady = 0;
  // Cutoff milligrams as the weighing setup is not that precise
  int newWeight = static_cast<int>(LoadCell[i].getData());
  // The delta in weight is not large, do not update the display (could be due to noise)
  bool smallDelta = abs(newWeight - displayedWeight[i]) <= 5;
  // The delta in weight is pretty large and thus the weight measurement has not yet stabilized, do not update the display
  bool largeDelta = abs(newWeight - lastWeight[i]) >= 6;
  if (!firstMeasurement[i])
  {
    if (largeDelta && newWeight > 5)
    {
      displayMeasuring();
    }
    lastWeight[i] = newWeight;
    if (smallDelta || largeDelta || newWeight <= 5)
    {
      // Do not update the screen
      return;
    }
  }
  else
  {
    firstMeasurement[i] = false;
    lastWeight[i] = newWeight;
  }
  if (newWeight <= 5)
  {
    // for very small values or negative values just display 0 as the precision of the scale is not that great
    newWeight = 0;
  }
  displayedWeight[i] = newWeight;

  if (serviceEnabled)
  {
    publishWeightToMQTT(i);
  }
  displayWeight(i);
  Serial.print("Load_cell ");
  Serial.print(i);
  Serial.print(": ");
  Serial.print(newWeight);
  Serial.print(" grams");
  Serial.println("");
}

void reconnect()
{
  displayReconnectMessage();
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client"))
    {
      Serial.println("connected");
      std::__cxx11::string topic = "cocktail/weight/"; // topic for instructions
      const char *cstr = topic.c_str();
      boolean success = client.subscribe(topic.c_str());
      Serial.print("Subscribed to MQTT: ");
      Serial.println(success);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop()
{
  if (serviceEnabled)
  {
    if (!client.connected())
    {
      reconnect();
    }
    client.loop();
  }
  // get smoothed value from the dataset:
  if (newDataReady)
  {
    if (millis() > t + serialPrintInterval)
    {
      for (size_t i = 0; i < numer_of_loadcells; i++)
      {
        handleNewWeightData(i);
      }
      t = millis();
    }
  }

  if (Serial.available() > 0)
  {
    char inByte = Serial.read();
    if (inByte == 't')
    {
      for (size_t i = 0; i < numer_of_loadcells; i++)
      {
        LoadCell[i].tareNoDelay();
      }
    }
  }

  if (LoadCell[0].getTareStatus())
  {
    Serial.println("Tare complete");
  }
}