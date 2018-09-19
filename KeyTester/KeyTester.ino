

#include <ESP8266WiFi.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>


/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/
#define wifi_ssid "Your-WiFi-SSID"
#define wifi_password "Your-WiFi-Password"

#define mqtt_server "Your-MQTT-Server"
#define mqtt_user "User" 
#define mqtt_password "password"
#define mqtt_port 18355


/************* MQTT TOPICS (change these topics as you wish)  **************************/
#define sensor_state_topic "techhome/sensornode1"
#define light_set_topic "techhome/sensornode1/set"

#define light_off_topic "techhome/keytester/turn_off_light"
#define light_on_topic "techhome/keytester/turn_on_light"

const char* on_cmd = "ON";
const char* off_cmd = "OFF";


/**************************** FOR OTA **************************************************/
#define SENSORNAME "keyTester"
#define OTApassword "5683" // change this to whatever password you want to use when you upload OTA
int OTAport = 8266;


/**************************** PIN DEFINITIONS ********************************************/
#define DHTPIN    D7
#define DHTTYPE   DHT22

const byte COLS = 6; //four columns
char hexKeys[COLS] = {'S', 'I', 'M', 'E', 'N', 'G'};
const byte pins[COLS] = {D8, D6, D5, D2, D1, D0};


/**************************** SENSOR DEFINITIONS *******************************************/
float diffTEMP = 0.2;
float tempValue;

float diffHUM = 1;
float humValue;

char message_buff[100];

int calibrationTime = 0;

const int BUFFER_SIZE = 300;

#define MQTT_MAX_PACKET_SIZE 512


/******************************** GLOBALS for fade/flash *******************************/
bool stateOn = false;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);


/********************************** START SETUP*****************************************/
void setup() {
  
  Serial.begin(115200);

  for (byte i = 0; i < COLS; i++) {
    pinMode(pins[i], INPUT);
  }

  delay(10);
  
  ArduinoOTA.setPort(OTAport);

  ArduinoOTA.setHostname(SENSORNAME);

  ArduinoOTA.setPassword((const char *)OTApassword);
  
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());
  reconnect();
}


/********************************** START SETUP WIFI*****************************************/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    return;
  }

  sendState(sensor_state_topic);
}


/********************************** START PROCESS JSON*****************************************/
bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
    }
  }

  return true;
}


/********************************** START SEND STATE*****************************************/
void sendState(char* topic) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? on_cmd : off_cmd;
  root["humidity"] = (String)humValue;
  root["temperature"] = (String)tempValue;
  root["heatIndex"] = (String)calculateHeatIndex(humValue, tempValue);
  
  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  if (root["humidity"] != "0.00") {
    client.publish(topic, buffer, true);
  }
  //client.publish(topic, buffer, true);
}


/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(light_set_topic);
      //setColor(0, 0, 0);
      sendState(sensor_state_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


/********************************** START CHECK SENSOR **********************************/
bool checkBoundSensor(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}


/********************************** GET KEY***********************************************/
char getKey() {
  for (byte i = 0; i < COLS; i++) {
    //byte scanVal = digitalRead(pins[i]);
    if (digitalRead(pins[i]) == HIGH) {
      delay(10);  //软件消抖
      if (digitalRead(pins[i]) == HIGH) {
        while (digitalRead(pins[i]) == HIGH) ; //wait for releasing the key
        Serial.println(hexKeys[i]);
        return hexKeys[i];
      }
    }
  }
  return 'X';
}


/********************************** PROCESS KEY***********************************************/
void processKey(char input) {
  if (input == 'E') {
    sendState(light_on_topic);
  } else if (input == 'M') {
    sendState(light_off_topic);
  }
}


/********************************** START MAIN LOOP***************************************/
void loop() {      

  ArduinoOTA.handle();

  if (!client.connected()) {
    // reconnect();
    software_Reset();
  }
  client.loop();

  if (stateOn) {
    for (byte i = 0; i < COLS; i++) {
      pinMode(pins[i], OUTPUT);
    }
    for (byte i = 0; i < COLS; i++) {
      digitalWrite(pins[i], HIGH);
      delay(20);
    } 
    for (byte i = 0; i < COLS; i++) {
      digitalWrite(pins[i], LOW);
      delay(20);
    }
  } 
  else {
    for (byte i = 0; i < COLS; i++) {
      pinMode(pins[i], INPUT);
    }
    float newTempValue = dht.readTemperature(); //to use celsius remove the true text inside the parentheses  
    float newHumValue = dht.readHumidity();
    
    if (checkBoundSensor(newTempValue, tempValue, diffTEMP) && (newHumValue != 0.0)) {
      tempValue = newTempValue;
      sendState(sensor_state_topic);
    }

    if (checkBoundSensor(newHumValue, humValue, diffHUM) && (newHumValue != 0.0)) {
      humValue = newHumValue;
      sendState(sensor_state_topic);
    }

    //processKey(getKey());
  }
  
}


/****reset***/
void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  Serial.print("resetting");
  ESP.reset();
}



/*
 * Calculate Heat Index value AKA "Real Feel"
 * NOAA heat index calculations taken from
 * http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
 */
float calculateHeatIndex(float humidity, float cTemp) {
  float temp = cTemp * 1.8 + 32.0;
  float heatIndex = 0;
  float cHeatIndex = 0;
  if (temp >= 80) {
    heatIndex = -42.379 + 2.04901523*temp + 10.14333127*humidity;
    heatIndex = heatIndex - .22475541*temp*humidity - .00683783*temp*temp;
    heatIndex = heatIndex - .05481717*humidity*humidity + .00122874*temp*temp*humidity;
    heatIndex = heatIndex + .00085282*temp*humidity*humidity - .00000199*temp*temp*humidity*humidity;
  } else {
     heatIndex = 0.5 * (temp + 61.0 + ((temp - 68.0)*1.2) + (humidity * 0.094));
  }

  if (humidity < 13 && 80 <= temp <= 112) {
     float adjustment = ((13-humidity)/4) * sqrt((17-abs(temp-95.))/17);
     heatIndex = heatIndex - adjustment;
  }

  cHeatIndex = (heatIndex - 32) * 5 / 9;
  
  return cHeatIndex;
}

