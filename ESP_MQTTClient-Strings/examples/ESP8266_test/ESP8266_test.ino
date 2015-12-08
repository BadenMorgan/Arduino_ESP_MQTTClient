//#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266.h>

#define SSID "MorganOnly"
#define Pass "1234m0rg@n2015"

//#define SERVER "10.0.0.23" //"m11.cloudmqtt.com"  //host for mosquitto broker
//#define PORT 1883  //18675  //port number for mosquitto broker
#define SERVER "m11.cloudmqtt.com"  //host for mosquitto broker
#define PORT 18675  //port number for mosquitto broker
#define password "S3i9xIc5KAJF"
#define username "ubexgazz"
#define ID "esp_device0"

int count = 0;

uint32_t stamp = 0;
uint32_t stamp3 = 0;
uint32_t stamp4 = 0;

int PublishInterval = 5000;

ESP8266 esp8266(1000);

byte allretries = 0;

void setup() {
  pinMode(7, OUTPUT);
  pinMode(12, OUTPUT);
  //esp8266.InitComms();
  initESP8266();
}

void loop() {
  if (esp8266.RTNConnected()) {
    digitalWrite(7, HIGH);
  } else {
    digitalWrite(7, LOW);
  }
  MQTTProcess();
}

//////////////////////////
//mqtt looping function
/////////////////////////
void MQTTProcess() {
  byte connectdCheck = esp8266.RTNConnected();
  if (connectdCheck == 1) {
    if (millis() - stamp4 >= 100) {//play around with shortening
      stamp4 = millis();
      SubExec(esp8266.MQTTSubCheck());
    }
    if (millis() - stamp3 >= PublishInterval) {
      stamp3 = millis();
      PublishQue();
    }

  }
  if (!connectdCheck && (millis() - stamp >= 10000)) {
    esp8266.MQTTConnect(SERVER, PORT, ID, username, password);
    //esp8266.MQTTConnect(SERVER, PORT, ID);
    allretries++;
    if (!esp8266.RTNConnected()) {
      if (allretries == 3) {
        initESP8266();
        allretries = 0;
      }
      stamp = millis();
    } else {
      esp8266.MQTTSubscribe("hello");
    }
  }
  if (connectdCheck != esp8266.RTNConnected()) {
    stamp = millis();
    if (esp8266.RTNConnected() == 0) {
      esp8266.MQTTDisconnect();
    }
  }
}

/////////////////////////////////////////
//esp initialization and broker connect
/////////////////////////////////////////
void initESP8266() {
  if(!esp8266.WifiCheck(SSID)){
    esp8266.Connect(SSID, Pass);
  }
  esp8266.MQTTConnect(SERVER, PORT, ID, username, password);
  //esp8266.MQTTConnect(SERVER, PORT, ID);
  if (esp8266.RTNConnected() == 1) {
    SubhQue();    
  }
}

////////////////////////////////////////
//ESP subscription handling function
///////////////////////////////////////
/*first parameter is the
 * message received from
 * the library
 */
void SubExec(String receivedmsg) {
  if (receivedmsg != "") {
    byte sublen = receivedmsg[0];
    byte topiclen = receivedmsg[2];
    String topic;
    for (int i = 3 ; i < topiclen + 3; i ++) {
      topic += receivedmsg[i];
    }
    if (topic == "hello") {
      digitalWrite(12, receivedmsg[topiclen + 3] - 48);
    }
  }
}


/////////////////////////
//publish messages
////////////////////////
/*put your publish topics and
 *messages in this function
*/
void PublishQue() {
  String msg = "hello world";
  String topic = "device/0";
  msg += count;
  count++;
  esp8266.MQTTPublish(topic, msg);
}

/////////////////////////
//Subscribe topics list
////////////////////////
void SubhQue(){
  esp8266.MQTTSubscribe("hello");  
}

