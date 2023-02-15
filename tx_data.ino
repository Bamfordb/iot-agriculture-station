#include "Arduino.h"
#include "ArduinoJson.h"
#include "PubSubClient.h"

#include "SoftwareSerial.h"
#include "WiFiEsp.h"
#include "WiFiEspClient.h"
#include "WiFiEspUdp.h"

#include "DHT.h"
#include "Adafruit_Sensor.h"
//#include "Adafruit_TSL2591.h"
#include "Wire.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "hp_BH1750.h"

#include "Credentials.h"

#define DHTTYPE DHT22
#define DHTPIN 7        // temp/humid input
#define SensorPinA0 A0  // pH input
#define ONE_WIRE_BUS 6  // water temp probe input

//Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
hp_BH1750 BH1750;
WiFiEspClient espClient;
PubSubClient client(espClient);
SoftwareSerial Serial1(8,9); // UNO 8 --> ESP TX  UNO 9 --> ESP RX


const char ssid[] = WIFI_SSID; // your network SSID
const char pass[] = WIFI_PASSWORD; // your network password
int status = WL_IDLE_STATUS; // the Wifi radio's status

const char* mqtt_server = MQTT_SERVER_ADDRESS;
const int   mqtt_port = MQTT_SERVER_PORT;
const char* mqtt_clientID = MQTT_CLIENT_NAME;
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
char* mqtt_publish_topic = MQTT_PUB_TOPIC;
char* mqtt_subscribe_topic = MQTT_SUB_TOPIC;

void setup() {
  Serial.begin(9600);  // sensors
  Serial1.begin(9600); // wifi
  WiFi.init(&Serial1);
  dht.begin();
  //tsl.begin();
  sensors.begin();

  BH1750.begin(BH1750_TO_GROUND);
  BH1750.calibrateTiming();
  BH1750.start();



  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    // don't continue
  while (true);
  }

  while (status != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to WPA SSID: "));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  if (status = WL_CONNECTED) {
    Serial.println(F("You're connected to the network"));
    //connect to MQTT server
    client.setServer("192.168.7.54", 1883);
    client.setCallback(callback);
    Serial.println(F("Setting up MQTT"));
  }
}

//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);

  Serial.print("] ");
  for (int i=0;i<length;i++) {
    char receivedChar = (char)payload[i];
    Serial.print(receivedChar);
  if (receivedChar == '0')
    Serial.println("Off");
  if (receivedChar == '1')
    Serial.println("On");

  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
  Serial.println(F("Attempting MQTT connection..."));
  // Attempt to connect, just a name to identify the client
    if (client.connect(mqtt_clientID)) {
      Serial.println(F("connected"));
  // Once connected, publish an announcement...
      //client.publish(mqtt_publish_topic, "message connection start");
  // ... and resubscribe
      client.subscribe(mqtt_subscribe_topic, 0);
      Serial.println(F("subscribed"));
    }
  else {
    Serial.print(F("failed, rc="));
    Serial.print(client.state());
    Serial.println(F(" try again in 5 seconds"));
    // Wait 5 seconds before retrying
    delay(5000);
    }
  }
}

void getReading(){
  StaticJsonDocument<64> doc;
  Serial.println(F("Creating JSON Data"));

  //Serial.println(F("Getting pH values"));
  float analog_value = analogRead(SensorPinA0);
  float voltage = analog_value * (5.0 / 1023.0);
  //Serial.println(F("Getting Lux values"));
  //uint32_t lum = tsl.getFullLuminosity();
  //uint16_t ir, full;
  //ir = lum >> 16;
  //full = lum & 0xFFFF;  // transform color values from a special format to standard RGB values

  if (BH1750.hasValue() == true) {    // non blocking reading
    float lux = BH1750.getLux();
    //Serial.println(lux);
    BH1750.start();
  }

  //Serial.println(F("Getting water temperature values"));
  sensors.requestTemperatures();
  doc["water_temp_c"] = sensors.getTempCByIndex(0);
  //Serial.println(F("Getting humidity"));
  doc["humidity"] = dht.readHumidity();
  //Serial.println(F("Getting temperature"));
  doc["temp_c"] = dht.readTemperature();
  doc["temp_f"] = dht.readTemperature(true);
  doc["pH"] = 3.5*voltage;
  //doc["ir"] = ir;
  //doc["full_spectrum"] = full;
  //Serial.println(F("Calculating lux"));
  //doc["lux"] = tsl.calculateLux(full, ir);
  //Serial.println(F("Writing data"));
  doc["lux"] = BH1750.getLux();

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  //Serial.println(F("Stringify Data"));
  client.publish(mqtt_publish_topic, buffer, n);
  Serial.println(F("Published Data"));
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  if (client.connected()) {
  //client.loop();
  //Serial.println(client.state());
  Serial.println(F("Getting Reading"));
  getReading();
  Serial.println(F("Finished reading"));
  delay(4200);
  }
}