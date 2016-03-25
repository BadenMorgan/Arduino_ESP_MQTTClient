/*
 * Debugging Notes:
 * debugging on port 9  can be turned off by
 * commenting out _DEBUG_  you can also use a
 * more extensive debugger by commenting in
 * ALLDEBUG
 */

#include <SoftwareSerial.h>

#include "ESP8266.h"

int count = 0;
byte count2 = 0x30;
uint32_t TimeStamp = 0;
//used to determine at what interval to publish messages

//Declare esp8266 and setup waittime
ESP8266 esp8266;
//ESP8266 esp8266(1500);
//ESP8266 esp8266(0,10); //can be used to make program more lenient towards no "send ok" messages being received
//ESP8266 esp8266(1000,8,1000); //can be used to reduce or extend time between automatic publishes

void setup() {
  pinMode(7, OUTPUT);
  for (int i = 0 ; i < 4 ; i++ ) {
    pinMode(2 + i, OUTPUT);
  }
  pinMode(13, OUTPUT);
  esp8266.initESP8266();//connect to broker with username and password
}

void loop() {
  //this code is used to visually see without debugging wether your device is connected to the broker or not
  if ((esp8266.connectd & 2) == 2) {                               //check if the connected flag is set or not
    digitalWrite(7, HIGH);                                    //set a LED to indicate the broker connection
  } else {
    digitalWrite(7, LOW);
  }
  ////////////////////////////////////////////
  esp8266.MQTTProcess(SubhQue, SubExec, PublishQue);                           //run the proccess to check and manage the MQTT connection
  if (millis() - TimeStamp >= 1000) {
    if (digitalRead(13)) {
      digitalWrite(13, LOW);
    } else {
      digitalWrite(13, HIGH);
    }
    TimeStamp = millis();
  }
}

////////////////////////////////////////
//ESP subscription handling function
///////////////////////////////////////
void SubExec() {
  if (esp8266.Sub1->len > 0) { //receivedmsg != "") {
    if (( ((String)(esp8266.Sub1->topic)) == "hello") && (esp8266.Sub1->payloadlen == 4)) {
      for (int i = 0 ; i < 4 ; i++ ) {
        digitalWrite(2 + i, (byte)(esp8266.Sub1->payload[i]) - 48);
      }
    }
    else{
      if(esp8266.Sub1->payload == "disconnect"){
        esp8266.MQTTDisconnect();
        while(1);
      }
    }
    esp8266.Sub1->len = 0;
  }
}


/////////////////////////
//publish messages
////////////////////////
/*put your publish topics and
 *messages in this function
*/
void PublishQue() {
  //publish a message using a string
  /*String msg = "hello world";                                 //message payload of MQTT package, put your payload here
  String topic = "device/0";                                  //topic of MQTT package, put your topic here
  msg += count;                                               //used to increment msg count to keep msgs unique, you can get rid of this if you want
  count++;
  esp8266.MQTTPublish(topic, msg);*/
  //publish a message using an array
  byte msg[12] = {0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x00};                                 //message payload of MQTT package, put your payload here
  String topic = "device/0";                                  //topic of MQTT package, put your topic here
  msg[11] = count2++;                                          //used to increment msg count to keep msgs unique, you can get rid of this if you want
  if (count2 == 0x3A) {
    count2 = 0x30;
  }
  esp8266.MQTTPublish(topic, &msg[0], 12);
  //put more publish msgs here if you want
  //esp8266.MQTTPublish(yournewtopic, yournewmsg);
}

/////////////////////////
//Subscribe topics list
////////////////////////
void SubhQue() {
  esp8266.MQTTSubscribe("hello");                             //put your subs here with corresponding topics
}
