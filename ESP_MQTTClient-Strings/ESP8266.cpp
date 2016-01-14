/*
Author: Baden Morgan

ESP_MQTTClient
A client program for the ESP8266 designed for the Arduino platform but can be ported 
to straight C for other microcontrollers.

Code used to operate the eps8266 which is running the firmaware version 0.9.2.2 AT
view datasheet for AT commands and descriptions
https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/

For further instructions on how to setupYour ESP8266 follow the instructions at:
http://www.xess.com/blog/esp8266-reflash/

The library softreset was used to help with the connection sequence and automating its
functionality, the library can be found here but has been included with this library
adn can be found as a standalone here: https://github.com/WickedDevice/SoftReset

The code relating to mqtt is all based on the documentation of mqtt v3.1.1

The module was connecting to a mosquitto(v1.4.4) server over a local network,
your results may differ depending on the server you use, please make sure its
not the server before logging an issue.

The functionality was also tested on an cloudmqtt server successfully.

Please note that This code is not intended for sale by anyone and is
opensource. If you do intend to use this software all I ask is that you
give me credit were it is due. By downloading and using this you accept
to not charge for this software.

This software is provided by the copyright holders and contributors "as is"
and any express or implied warranties, including, but not limited to, the
implied warranties of merchantability and fitness for a particular purpose
are disclaimed. In no event shall the copyright owner or contributors be
liable for any direct, indirect, incidental, special, exemplary, or
consequential damages (including, but not limited to, procurement of
substitute goods or services; loss of use, data, or profits; or business
interruption) however caused and on any theory of liability, whether in
contract, strict liability, or tort (including negligence or otherwise)
arising in any way out of the use of this software, even if advised of the
possibility of such damage.
*/
#include "Arduino.h"
#include "ESP8266.h"

#define _DEBUG_
//#define ALLDEBUG // may break functionality but will help diagnose connection issues

#ifdef _DEBUG_
#include <SoftwareSerial.h>
SoftwareSerial mySerial(8, 9); // RX, TX
#endif

//standard variables 
int waittime = 1000;
byte fails = 0;
byte attempts = 3;
byte connectd = 0;
byte retries = 0;
byte failed = 5;

//if defaults are good
ESP8266::ESP8266() {
}

//if the timeout period is not idealk
ESP8266::ESP8266(int SetWaitTime) {
  waittime = SetWaitTime;
}

//if there are too few SENDOK messages and you know you can allow for more
ESP8266::ESP8266(int SetWaitTime, byte setFailed) {
  waittime = SetWaitTime;
  failed = setFailed;
}

//setup comms layer
void ESP8266::InitComms() {
  Serial.begin(9600);
#ifdef _DEBUG_
  mySerial.begin(9600);
  mySerial.println(F("hello"));
#endif
  Serial.setTimeout(5000);
}
//setup and connect esp8266
void ESP8266::Connect(String SSID, String Pass) {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
#ifdef _DEBUG_
  mySerial.begin(9600);
  mySerial.println(F("***\t***\t***\t***\t***"));
#endif
  Serial.setTimeout(5000);
#ifdef _DEBUG_
  mySerial.println(F("Resetting module"));
#endif
  Serial.println(F("AT+RST"));
  delay(5000);
  ClearIncomingSerial();
  delay(100);
  //test if the module is ready
#ifdef _DEBUG_
  mySerial.println(F("checking module connection"));
#endif
  Serial.println(F("AT"));
  while (!Serial.read());
  delay(1000);
  if (Serial.find("OK"))
  {
#ifdef _DEBUG_
    mySerial.println(F("Module Ready"));
#endif
  }
  else
  {
#ifdef _DEBUG_
    mySerial.println(F("Module has no response"));
#endif
    soft_restart();
  }
#ifdef _DEBUG_
  mySerial.println(F("turning off echo"));
#endif
  Serial.println(F("ATE0"));//turn off echo
  delay(100);
#ifdef _DEBUG_
  while (Serial.available()) {
    mySerial.write(Serial.read());
  }
#endif
  //connect to the wifi
  boolean connectd = false;
  for (int i = 0; i < 5; i++)
  {
    if (connectWiFi(SSID, Pass))
    {
      connectd = 1;
      break;
    }
  }
  if (!connectd) {
    while (1);
  }
  delay(1000);

  //set the single connection mode
  Serial.println(F("AT+CIPMUX=0"));
#ifdef ALLDEBUG
  mySerial.println(F("AT+CIPMUX=0"));
#endif
}

//connect to specific SSID
boolean ESP8266::connectWiFi(String SSID, String Pass) {
  Serial.println(F("AT+CWMODE=1"));
#ifdef ALLDEBUG
  mySerial.println(F("AT+CWMODE=1"));
#endif
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += Pass;
  cmd += "\"";
  //
#ifdef ALLDEBUG
  mySerial.println(cmd);
#endif
  Serial.println(cmd);
  delay(2000);
  if (Serial.find("OK"))
  {
#ifdef _DEBUG_
    mySerial.println(F("OK, Conneceted to WIFI"));
#endif
    return true;
  } else
  {
#ifdef _DEBUG_
    mySerial.println(F("Can not connect to WIFI"));
#endif
    return false;
  }
}

//////////////////////////////////////////
//mqtt code
/////////////////////////////////////////

//connect to server
void ESP8266::MQTTConnect(String broker, int port, String DeviceID) {
  if (connectd) retries = 0;
  //setting up tcp connection
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += broker;
  cmd += "\",";
  cmd += port;
  Serial.println(cmd);
#ifdef ALLDEBUG
  mySerial.println(cmd);
#endif
  delay(waittime);
  unsigned char LVL4[14]  = {0x10, 0x00, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x02, 0x00, 0x78, 0x00, 0x00};
  //constructing connect message to send to broker
  byte IDlen = DeviceID.length();
  byte msglen;
  msglen = IDlen + 14;
  unsigned char newcmd[msglen];
  for (int i = 0; i < 14 ; i ++) {
    newcmd[i] = LVL4[i];
  }
  newcmd[1] = IDlen + 12;
  newcmd[13] = IDlen;
  for (int i = 0; i < IDlen ; i++) {
    newcmd[i + 14] = DeviceID[i];
  }
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += msglen;
  Serial.println(cmdtemp);
  Serial.flush();
  for (int i = 0; i < msglen; i++) {
    Serial.write(newcmd[i]);
    Serial.flush();
  }
  ClearIncomingSerial();
#ifdef ALLDEBUG
  mySerial.println(cmdtemp);
  for (int i = 0; i < msglen; i++) {
    mySerial.write(newcmd[i]);
  }
  mySerial.println();
#endif
  delay(waittime);
  Serial.find("+IPD,");
  while (Serial.available())
  {
    byte len = Serial.read();
    Serial.read();
    unsigned char ack[len];
    for (int i = 0; i < len; i++) {
      ack[i] = Serial.read();
    }
    if (ack[0] == 32 && ack[1] == 2 && ack[2] == 0 && ack[3] == 0) {
      connectd = 1;
#ifdef _DEBUG_
      mySerial.println(F("connected to broker"));
#endif
      retries = 0;
    }
    ClearIncomingSerial();
  }
  if (connectd == 0) {
#ifdef _DEBUG_
    mySerial.println(F("undable to connect to broker"));
#endif
    retries++;
    if (retries <= attempts) {
#ifdef _DEBUG_
      mySerial.println(F("trying to reconnect"));
#endif
      MQTTDisconnect();
      MQTTConnect(broker, port, DeviceID);
    }
  }
}

//connect to server with username and password
void ESP8266::MQTTConnect(String broker, int port, String DeviceID, String Username, String Password) {
  if (connectd) retries = 0;
  //setting up tcp connection
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += broker;
  cmd += "\",";
  cmd += port;
  Serial.println(cmd);
#ifdef ALLDEBUG
  mySerial.println(cmd);
#endif
  delay(waittime);
#ifdef ALLDEBUG
  ReadSerial();
#endif
  unsigned char LVL3[16]  = {0x10, 0x00, 0x00, 0x06, 0x4D, 0x51, 0x49, 0x73, 0x64, 0x70, 0x03, 0xC2, 0x00, 0x78, 0x00, 0x00};
  unsigned char LVL4[14]  = {0x10, 0x00, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0xC2, 0x00, 0x78, 0x00, 0x00};
  //constructing connect message to send to broker
  byte IDlen = DeviceID.length();
  byte Userlen = Username.length();
  byte Passlen = Password.length();
  byte msglen;
  msglen = IDlen + 18 + Userlen + Passlen;
  unsigned char newcmd[msglen];
  for (int i = 0; i < 14 ; i ++) {
    newcmd[i] = LVL4[i];
  }
  newcmd[1] = IDlen + 16 + Userlen + Passlen;
  newcmd[13] = IDlen;
  for (int i = 0; i < IDlen ; i++) {
    newcmd[i + 14] = DeviceID[i];
  }
  newcmd[IDlen + 14] = 0;
  newcmd[IDlen + 15] = Userlen;
  for (int i = 0; i < Userlen ; i++) {
    newcmd[i + IDlen + 16] = Username[i];
  }
  newcmd[IDlen + 16 + Userlen] = 0;
  newcmd[IDlen + 17 + Userlen] = Passlen;
  for (int i = 0; i < Passlen ; i++) {
    newcmd[i + IDlen + 18 + Userlen] = Password[i];
  }
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += msglen;
  Serial.println(cmdtemp);
  Serial.flush();
  for (int i = 0; i < msglen; i++) {
    Serial.write(newcmd[i]);
    Serial.flush();
  }
  ClearIncomingSerial();
#ifdef ALLDEBUG
  mySerial.println(cmdtemp);
  for (int i = 0; i < msglen; i++) {
    mySerial.print(newcmd[i]);
    mySerial.print(",");
  }
  mySerial.println();
#endif
  delay(waittime);
  Serial.find("+IPD,");
  while (Serial.available())
  {
    byte len = Serial.read();
    Serial.read();
    unsigned char ack[len];
    for (int i = 0; i < len; i++) {
      ack[i] = Serial.read();
    }
    if (ack[0] == 32 && ack[1] == 2 && ack[2] == 0 && ack[3] == 0) {
      connectd = 1;
#ifdef _DEBUG_
      mySerial.println(F("connected to broker"));
#endif
      retries = 0;
    }
    ClearIncomingSerial();
  }
  if (connectd == 0) {
#ifdef _DEBUG_
    mySerial.println(F("undable to connect to broker"));
#endif
    retries++;
    if (retries <= attempts) {
#ifdef _DEBUG_
      mySerial.println(F("trying to reconnect"));
#endif
      MQTTDisconnect();
      delay(100);
      MQTTConnect(broker, port, DeviceID, Username, Password );
    }
  }
}

//disconnect from the broker
void ESP8266::MQTTDisconnect() {
#ifdef _DEBUG_
  mySerial.println(F("********Disconnecting from broker************"));
#endif
  unsigned char cmd[2] = {0b11100000, 0x00};
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += 2;
  Serial.println(cmdtemp);
  delay(waittime);
  for (int i = 0; i < 2; i++) {
    Serial.write(cmd[i]);
  }
#ifdef ALLDEBUG
  mySerial.println(cmdtemp);
  for (int i = 0; i < 2; i++) {
    mySerial.write(cmd[i]);
  }
  mySerial.println();
#endif
  delay(waittime);
  Serial.println(F("AT+CIPCLOSE"));
#ifdef ALLDEBUG
  mySerial.println(F("AT+CIPCLOSE"));
#endif
  connectd = 0;
  ClearIncomingSerial();
}

//publish message test
/*
void ESP8266::MQTTPublish(byte deviceNo, String message) {
#ifdef _DEBUG_
  mySerial.print(F("Message:"));
  mySerial.print(message);
  mySerial.print(F(", "));
  mySerial.print(F("Device No.:"));
  mySerial.print(deviceNo);
  mySerial.println();
#endif
  //constructing message
  volatile byte msgLen = message.length() + 16;
  volatile unsigned char cmd[msgLen];
  volatile unsigned char momcmd[16] = {0x32, 0x00, 0x00, 0x0A, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2F, 0x00, 0x00, 0x00, 0x0C, 0x22};
  for (int i = 0; i < 16; i++) {
    cmd[i] = momcmd[i];
  }
  volatile byte temp1 = deviceNo / 100;
  volatile byte temp2 = (deviceNo - temp1 * 100) / 10;
  volatile byte temp3 = deviceNo - temp1 * 100 - temp2 * 10;
  cmd[11] = temp1 + 48;
  cmd[12] = temp2 + 48;
  cmd[13] = temp3 + 48;
  for (int i = 0; i < msgLen; i++) {
    cmd[16 + i] = message[i];
  }
  cmd[1] = sizeof(cmd) - 2;
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += sizeof(cmd);
  //sending message
  Serial.println(cmdtemp);
  Serial.flush();
  for (int i = 0; i < sizeof(cmd); i++) {
    Serial.write(cmd[i]);
    Serial.flush();
  }

  //ClearIncomingSerial();
#ifdef ALLDEBUG
  mySerial.println(cmdtemp);
  for (int i = 0; i < sizeof(cmd); i++) {
    mySerial.write(cmd[i]);
  }
  mySerial.println();
#endif
  delay(waittime);
  byte sentflag = 0;
  byte temp[7] = {0, 0, 0, 0, 0, 0, 0};
  while (Serial.available()) {
    byte temp2 = Serial.read() ;
    if (temp2 == 43) {
#ifdef _DEBUG_
      mySerial.println(F("Breaking out"));
#endif
      return;
    } else {
      for (int i = 6 ; i > 0 ; i --) {
        temp[i] = temp[i - 1];
      }
      temp[0] = temp2;
#ifdef ALLDEBUG
      mySerial.println();
      for (int i = 0; i < 7 ; i++) {
        mySerial.print(temp[i]);
        mySerial.print(",");
      }
#endif
      if (temp[0] == 84 && temp[1] == 65) {
        connectd = 0;
        Serial.println(F("ATE0"));//turn off echo
        delay(100);
#ifdef _DEBUG_
        while (Serial.available()) {
          mySerial.write(Serial.read());
        }
#endif
        break;
      }
      if (temp[0] == 75 && temp[1] == 79 && temp[2] == 32 && temp[3] == 68 && temp[4] == 78 && temp[5] == 69 && temp[6] == 83 ) {
        sentflag = 1;
        break;
      }
    }
  }
  ClearIncomingSerial();
  if (sentflag)
  {
#ifdef _DEBUG_
    mySerial.println(F("SEND OK"));
#endif
    fails = 0;
  }
  else {
#ifdef _DEBUG_
    mySerial.println(F("no SEND OK"));
#endif
    fails++;
    if (fails == 5) {
      connectd = 0;
    }
    return;
  }
  /*
   * was for qos 1, does work but found qos 0 worked better as
   *the server disconnects the client anyway
    delay(100);
    if (Serial.find("+IPD,")) {
      volatile unsigned char ack[Serial.read()];
      Serial.read();
      ack[0] = Serial.read();
      ack[1] = Serial.read();
      ack[2] = Serial.read();
      ack[3] = Serial.read();
      if (ack[0] == 64 && ack[1] == 2 && ack[2] == 12 && ack[3] == 34 ) {
  #ifdef _DEBUG_
        mySerial.println(F("Published message"));
  #endif
      } else {
        connectd = 0;
      }
    }
    Serial.find("\r\nOK\r\n");*/
}*/

//subscribe to a topic
void ESP8266::MQTTSubscribe(String topic) {
#ifdef _DEBUG_
  mySerial.print(topic);
  mySerial.print(",");
  mySerial.println(topic.length());
#endif
  //constructing message
  byte msgLen = topic.length();
  unsigned char cmd[msgLen + 7];
  cmd[0] = 0x82;
  cmd[2] = 12;
  cmd[3] = 34;
  cmd[4] = 0;
  cmd[5] = msgLen;
  cmd[msgLen + 6] = 0;
  for (int i = 0; i < msgLen; i++) {
    cmd[6 + i] = topic[i];
  }
  cmd[1] = sizeof(cmd) - 2;
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += sizeof(cmd);
  //sending message
  Serial.println(cmdtemp);
  Serial.flush();
  for (int i = 0; i < sizeof(cmd); i++) {
    Serial.write(cmd[i]);
  }
#ifdef ALLDEBUG
  mySerial.print(cmdtemp);
  delay(waittime);
  for (int i = 0; i < sizeof(cmd); i++) {
    mySerial.write(cmd[i]);
  }
  mySerial.println();
#endif
  delay(50);
  if (Serial.find("SEND OK\r\n")) {
    fails = 0;
#ifdef _DEBUG_
    mySerial.println(F("SEND OK"));
#endif
  } else {
    connectd = 0;
    return;
  }
  delay(50);
  while (Serial.available()) {
    if (Serial.find("+IPD,")) {
      unsigned char ack[Serial.read()];
      Serial.read();
      ack[0] = Serial.read();
      ack[1] = Serial.read();
      ack[2] = Serial.read();
      ack[3] = Serial.read();
      ack[4] = Serial.read();
      if (ack[0] == 144 && ack[1] == 3 && ack[2] == 12 && ack[3] == 34 ) {
#ifdef _DEBUG_
        mySerial.print(F("Subscribed to topic: "));
        mySerial.println(topic);
#endif
      } else if (ack[4] == 0x80) {
#ifdef _DEBUG_
        mySerial.print(F("Subscribe failed"));
        connectd = 0;
#endif
      } else {
#ifdef _DEBUG_
        mySerial.print(F("Unkown response from server"));
        connectd = 0;
#endif
      }
    }
  }
  delay(100);
  ClearIncomingSerial();
}

/*
 * this code keeps crashing the board
 * have a feeling it has something to
 * do with software serial but havnt
 * had a chance to diagnose it yet*/
void ESP8266::MQTTPublish(String topic, String message) {
#ifdef _DEBUG_
  mySerial.print(F("Topic:"));
  mySerial.print(topic);
  mySerial.print(F(", "));
  mySerial.print(F("Message:"));
  mySerial.print(message);
  mySerial.println();
#endif
  //constructing message
  volatile byte topiclen = topic.length();
  volatile byte msgLen = message.length() + topic.length() + 6;
  volatile unsigned char cmd[msgLen];
  volatile unsigned char momcmd[3] = {0x30, 0x00, 0x00,};
  for (int i = 0; i < 3; i++) {
    cmd[i] = momcmd[i];
  }
  cmd[3] = topiclen;
  for (int i = 0; i < topic.length(); i++) {
    cmd[4 + i] = topic[i];
  }
  cmd[topiclen + 4] = 12;
  cmd[topiclen + 5] = 34;
  for (int i = 0; i < msgLen; i++) {
    cmd[topiclen + 6 + i] = message[i];
  }
  cmd[1] = sizeof(cmd) - 2;
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += sizeof(cmd);
  //sending message
  Serial.println(cmdtemp);
  for (int i = 0; i < sizeof(cmd); i++) {
    Serial.write(cmd[i]);
  }
  //ClearIncomingSerial();
#ifdef ALLDEBUG
  mySerial.println(cmdtemp);
  for (int i = 0; i < sizeof(cmd); i++) {
    mySerial.write(cmd[i]);
  }
  mySerial.println();
#endif
  delay(waittime);
  byte sentflag = 0;
  byte temp[7] = {0, 0, 0, 0, 0, 0, 0};
  while (Serial.available()) {
    byte temp2 = Serial.read() ;
    if (temp2 == 43) {
#ifdef _DEBUG_
      mySerial.println(F("Breaking out"));
#endif
      return;
    } else {
      for (int i = 6 ; i > 0 ; i --) {
        temp[i] = temp[i - 1];
      }
      temp[0] = temp2;
#ifdef ALLDEBUG
      mySerial.println();
      for (int i = 0; i < 7 ; i++) {
        mySerial.print(temp[i]);
        mySerial.print(",");
      }
#endif
      if (temp[0] == 84 && temp[1] == 65) {
        connectd = 0;
        Serial.println(F("ATE0"));//turn off echo
        delay(100);
#ifdef _DEBUG_
        while (Serial.available()) {
          mySerial.write(Serial.read());
        }
#endif
        break;
      }
      if (temp[0] == 75 && temp[1] == 79 && temp[2] == 32 && temp[3] == 68 && temp[4] == 78 && temp[5] == 69 && temp[6] == 83 ) {
        sentflag = 1;
        break;
      }

    }
  }
  ClearIncomingSerial();
#ifdef _DEBUG_
  //ReadSerial();
#endif
  //cehck if incloming message before checking if send ok is recieved

  if (sentflag)
  {
#ifdef _DEBUG_
    mySerial.println(F("SEND OK"));
#endif
    fails = 0;
  }
  else {
#ifdef _DEBUG_
    mySerial.println(F("no SEND OK"));
#endif
    fails++;
    if (fails == failed) {
      connectd = 0;
    }
    return;
  }
  /*
   *was for qos 1 check, works but found qos 0 is fine
   *as the server disconnects the client anyway
    delay(100);
    if (Serial.find("+IPD,")) {
      volatile unsigned char ack[Serial.read()];
      Serial.read();
      ack[0] = Serial.read();
      ack[1] = Serial.read();
      ack[2] = Serial.read();
      ack[3] = Serial.read();
      if (ack[0] == 64 && ack[1] == 2 && ack[2] == 12 && ack[3] == 34 ) {
  #ifdef _DEBUG_
        mySerial.println(F("Published message"));
  #endif
      } else {
        connectd = 0;
      }
    }
    Serial.find("\r\nOK\r\n");
    */
}

//check for incoming data
String ESP8266::MQTTSubCheck() {
  String received = "";
  char incoming;
#ifdef _DEBUG_
  //mySerial.println(F("checking subs"));
#endif
  if (Serial.available()) {
    if (Serial.find("IPD,")) {
      String temp = "";
      char inctemp;
      while (Serial.available()) {
        inctemp = Serial.read();
        if (inctemp != 58) {
          temp.concat(inctemp);
        } else {
          break;
        }
      }
      byte len = temp.toInt();
      for (int i = 0; i <  len + 1 ; i++) {
        if (i == 0) {
          Serial.read();
        } else {
          incoming = Serial.read();
          received.concat(incoming);
          delayMicroseconds(100);
        }
      }
    }
  }
#ifdef ALLDEBUG
  if (received != "") {
    for (int i = 0; i < received.length() ; i ++) {
      mySerial.print(received[i]);
      mySerial.print(",");
      mySerial.print(received[i], DEC);
      mySerial.print(":");
    }
    mySerial.println();
  }
#endif
#ifdef _DEBUG_
  if (received != "") {
    mySerial.print(F("message received:"));
    mySerial.println(received);
  }
#endif
  ClearIncomingSerial();
  return received;
}

/*idling function used to check the
program is still running printing out
to software serial if debuging is on*/
void ESP8266::idler() {
#ifdef _DEBUG_
  mySerial.print(F("/"));
  delay(1000);
#endif
}

/*needed to clear the serial buffer to
prevent the buffer from overflowing
and clearing SEND OK messages sent
from the ESP*/
void ESP8266::ClearIncomingSerial() {
  while (Serial.available()) {
    Serial.read();
  }
}

//read out serial coms data to end
void ESP8266::ReadSerial() {
#ifdef _DEBUG_
  while (Serial.available()) {
    mySerial.write(Serial.read());
    mySerial.print(F(","));
  }
#endif
}

//print debug message
void ESP8266::DebugPrint(String msg) {
#ifdef _DEBUG_
  mySerial.println(msg);
#endif
}

//check that the esp is connected to the wifi
byte ESP8266::WifiCheck(String SSID) {
  byte Wififlag = 1;
  Serial.println(F("AT+CWJAP?"));
#ifdef _DEBUG_
  mySerial.println(F("checking wifi"));
#endif
#ifdef ALLDEBUG
  mySerial.println(SSID.length());
#endif
  delay(100);
  if (Serial.find("+CWJAP:\"")) {
    for (int i = 0; i < SSID.length() ; i++) {
      char character = Serial.read();
#ifdef ALLDEBUG
      mySerial.write(character);
      mySerial.print(F(","));
      mySerial.write(SSID[i]);
      mySerial.println();
#endif
      if (SSID[i] != character) {
        Wififlag = 0;
      }
    }
  } else {
    Wififlag = 0;
  }
#ifdef ALLDEBUG
  mySerial.println(Wififlag);
#endif
#ifdef _DEBUG_
  if (Wififlag) {
    mySerial.println(F("Connected to SSID"));
  } else {
    mySerial.println(F("Not connected to SSID"));
  }
#endif
  if (!Wififlag) {
    connectd = 0;
  }
  return Wififlag;
}

byte ESP8266::RTNConnected() {
  if (connectd) {
    return 1;
  } else {
    return 0;
  }
}

