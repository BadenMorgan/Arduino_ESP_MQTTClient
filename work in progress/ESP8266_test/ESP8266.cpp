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

#ifdef AUTORESET
//#include "SoftReset.h"
#endif

#ifdef _DEBUG_
#include <SoftwareSerial.h>
SoftwareSerial mySerial(SOFTSERIALRX, SOFTSERIALTX); // RX, TX
#endif

#define ALLDEBUGTEMP


//if defaults are good
ESP8266::ESP8266() {
}

//if the timeout period is not idealk
ESP8266::ESP8266(int SetWaitTime) {
  waittime = SetWaitTime;
}

//if there are too few SENDOK messages and you know you can allow for more
ESP8266::ESP8266(int SetWaitTime, byte setFailed) {
  if (SetWaitTime != 0) {
    waittime = SetWaitTime;
  }
  failed = setFailed;
}

ESP8266::ESP8266(int SetWaitTime, byte setFailed, int setPubInterval) {
  if (SetWaitTime != 0) {
    waittime = SetWaitTime;
  }

  if (setFailed != 0) {
    failed = setFailed;
  }

  if (setPubInterval < (2 * waittime)) {
    PublishInterval = 2 * waittime;
  } else {
    PublishInterval = setPubInterval;
  }
}

//setup comms layer
void ESP8266::InitComms() {                                       //for debugging purposes when needed prior to esp connect
  Serial.begin(9600);
#ifdef _DEBUG_
  mySerial.begin(9600);
  mySerial.println(F("hello"));
#endif
  Serial.setTimeout(5000);
}

//setup and connect esp8266
void ESP8266::Connect() {
  allretries = 0;
  // Open serial communications and wait for port to open:
#ifdef _DEBUG_
  mySerial.begin(9600);
  //mySerial.println(F("*\t*\t*\t*\t*"));
  mySerial.println(F("Resetting module"));
#endif
  Serial.begin(9600);
  Serial.setTimeout(5000);
  Serial.println(F("AT+RST"));
  Serial.flush();
  Serial.end();//resetting ESP module
  delay(1000);
  Serial.begin(9600);
  //test if the module is ready
#ifdef _DEBUG_
  mySerial.println(F("checking module connection"));
#endif
  Serial.println(F("AT"));                                        //simple check for OK response
  ResetStamp();
  while (!Serial.available()) {
    if ((millis() - stamp) >= 2000) {
      break;
    }
  }
  if (Serial.find("OK"))                                          //checking wether an OK was received and if not the micro will be reset in order to try retry module setup
  {
#ifdef _DEBUG_
    mySerial.println(F("Module Ready"));
#endif
    connectd = 128;
  }
  else
  {
#ifdef _DEBUG_
    mySerial.println(F("Module has no response"));
#endif
    /*
    #ifdef AUTORESET
        soft_restart();
    #else
        digitalWrite(RESETPIN, HIGH);
        delay(1000);
        digitalWrite(RESETPIN, LOW);
        return;
    #endif
    */
    return;
  }
#ifdef _DEBUG_
  mySerial.println(F("turning off echo"));
#endif
  Serial.println(F("ATE0"));                                      //turn off echo
  delay(100);
#ifdef _DEBUG_
  while (Serial.available()) {
    mySerial.write(Serial.read());                                //clearring buffer manually
  }
#endif
  //connect to the wifi
  for (int i = 0; i < 5; i++)                                     //tries to connect to wifi a max of 5 times before
  {
    if (connectWiFi())
    {
      connectd |= 1;
      break;
    }
  }
  if (connectd == 128) {
#ifdef _DEBUG_
    mySerial.println(F("Complete failure to connect to SSID"));
#endif
#ifdef AUTORESET
    soft_restart();
#else
    digitalWrite(RESETPIN, HIGH);
    delay(1000);
    digitalWrite(RESETPIN, LOW);
    return;
#endif
  }
  delay(500);

  Serial.println(F("AT+CIPMUX=0"));                               //set the single connection mode
#ifdef ALLDEBUG
  mySerial.println(F("AT+CIPMUX=0"));
#endif
}

//connect to specific SSID
boolean ESP8266::connectWiFi() {
  Serial.println(F("AT+CWMODE=1"));                               //setting ESP as client mode connection
#ifdef ALLDEBUG
  mySerial.println(F("AT+CWMODE=1"));
#endif
  String cmd = "AT+CWJAP=\"";                                     //setting up connection string
  cmd += WifiSSID;
  cmd += "\",\"";
  cmd += WifiPass;
  cmd += "\"";
  //
#ifdef ALLDEBUG
  mySerial.println(cmd);
#endif
  Serial.println(cmd);
  ResetStamp();
  while (!Serial.available()) {
    if ((millis() - stamp) >= 2000) {
      break;
    }
  }
  delay(100);
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

//rest the stamp
void ESP8266::ResetStamp() {
  stamp = millis();
}

//disconnect from the broker
void ESP8266::MQTTDisconnect() {
#ifdef _DEBUG_
  mySerial.println(F("Disconnecting from broker"));
#endif
  unsigned char cmd[2] = {0b11100000, 0x00};                       //DISCONNNEC message to send to broker to unregister client
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += 2;
  Serial.println(cmdtemp);                                         //sending disconnect message
  delay(waittime);
  for (int i = 0; i < 2; i++) {
    Serial.write(cmd[i]);
  }
  connectd |= 2;                                                    //set status to disconnected
  connectd ^= 2;
  ResetStamp();
  FuncActive = 100;
}

//waits 2 seconds then closes the tcp connection
void ESP8266::TCPClose() {

  if ((millis() - stamp) >= 2000) {
#ifdef _DEBUG_
    mySerial.println(F("Closing TCP connection"));
#endif
    Serial.println(F("AT+CIPCLOSE"));                                 //close TCP connection
#ifdef ALLDEBUG
    mySerial.println(F("AT+CIPCLOSE"));
#endif
    connectd |= 2;                                                    //set status to disconnected
    connectd ^= 2;
    ClearIncomingSerial();                                            //clear the serial buffer
    ResetStamp();
    FuncActive++;
  }
}

//subscribe to a topic
void ESP8266::MQTTSubscribe(String topic) {
#ifdef _DEBUG_
  mySerial.print(topic);
  mySerial.print(",");
  mySerial.println(topic.length());
#endif

  byte msgLen = topic.length();                                       //constructing message from parameter given
  unsigned char cmd[msgLen + 7];
  cmd[0] = 0x82;
  cmd[2] = 12;
  cmd[3] = 34;
  cmd[4] = 0;
  cmd[5] = msgLen;

  for (int i = 0; i < msgLen; i++) {
    cmd[6 + i] = topic[i];
  }
  cmd[1] = sizeof(cmd) - 2;
  cmd[msgLen + 6] = 0;
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += sizeof(cmd);
  //sending message
  Serial.println(cmdtemp);                                              //sending message
  Serial.flush();
  for (int i = 0; i < sizeof(cmd); i++) {
    Serial.write(cmd[i]);
  }
#ifdef ALLDEBUG
  mySerial.print(cmdtemp);
  mySerial.println();
  for (int i = 0; i < sizeof(cmd); i++) {
    mySerial.print(cmd[i]);
    mySerial.print("'");
  }
  mySerial.println();
#endif
  ResetStamp();
  while (!Serial.available()) {
    if ((millis() - stamp) >= waittime) {
      break;
    }
  }
#ifdef  ALLDEBUG
  ReadSerial();
#endif//hardwired timeout period for data to come in
  if (Serial.find("SEND OK\r\n")) {                                          //check the message was sent succesfully
    fails = 0;
#ifdef _DEBUG_
    mySerial.println(F("SEND OK"));
#endif
  } else {
    connectd |= 2;
    connectd ^= 2;
    return;
  }
  delay(50);
#ifdef  ALLDEBUG
  ReadSerial();
#endif
#ifdef  ALLDEBUG
  ReadSerial();
#endif//hardwired timeout period for data to come in
  if (Serial.find("+IPD,")) {                                                 //look from response froms erver
    unsigned char ack[Serial.read()];
    Serial.read();
    ack[0] = Serial.read();
    ack[1] = Serial.read();
    ack[2] = Serial.read();
    ack[3] = Serial.read();
    ack[4] = Serial.read();
    if (ack[0] == 144 && ack[1] == 3 && ack[2] == 12 && ack[3] == 34 ) {    //look for SUBACK message
#ifdef _DEBUG_
      mySerial.print(F("Subscribed to topic: "));
      mySerial.println(topic);
#endif
    } else if (ack[4] == 0x80) {
#ifdef _DEBUG_
      mySerial.print(F("Subscribe failed"));
      connectd |= 2;
      connectd ^= 2;
#endif
    } else {
#ifdef _DEBUG_
      mySerial.print(F("Unkown response from server"));
      connectd |= 2;
      connectd ^= 2;
#endif
    }
  } else {
#ifdef _DEBUG_
    mySerial.println(F("Unkown error 1"));                                    //shouldnt get to this point unless the message sent doesnt conform to broker MQTT protocol
#endif
  }
  delay(100);
  ClearIncomingSerial();
}

//publish with string

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
  volatile byte topiclen = topic.length();                                     //build the publish message to send the broker
  volatile byte msgLen = message.length() + topic.length() + 4;
  volatile unsigned char cmd[msgLen];
  volatile unsigned char momcmd[3] = {0x30, 0x00, 0x00,};
  for (int i = 0; i < 3; i++) {
    cmd[i] = momcmd[i];
  }
  cmd[3] = topiclen;
  for (int i = 0; i < topic.length(); i++) {
    cmd[4 + i] = topic[i];
  }
  for (int i = 0; i < msgLen; i++) {
    cmd[topiclen + 4 + i] = message[i];
  }
  cmd[1] = sizeof(cmd) - 2;
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += sizeof(cmd);
  //sending message
  Serial.println(cmdtemp);
  for (int i = 0; i < sizeof(cmd); i++) {
    Serial.write(cmd[i]);                                                     //publish message
  }
#ifdef ALLDEBUG
  mySerial.println(cmdtemp);
  for (int i = 0; i < sizeof(cmd); i++) {
    mySerial.print(cmd[i], HEX);
    mySerial.print(",");
  }
  mySerial.println();
#endif
  ResetStamp();
  FuncActive = 2;
}


//publish with an array
void ESP8266::MQTTPublish(String topic, byte *message, byte messagelen) {

#ifdef _DEBUG_
  mySerial.print(F("Topic:"));
  mySerial.print(topic);
  mySerial.print(F(", "));
  mySerial.print(F("Message:"));
  for (int i = 0; i < messagelen ; i++) {
    mySerial.write(*message++);
    //mySerial.print(",");
  }
  message -= (messagelen);
  mySerial.println();
#endif
  //constructing message
  volatile byte topiclen = topic.length();                                     //build the publish message to send the broker
  volatile byte msgLen = messagelen + topiclen + 4;
  volatile unsigned char cmd[msgLen];
  volatile unsigned char momcmd[3] = {0x30, 0x00, 0x00,};
  for (int i = 0; i < 3; i++) {
    cmd[i] = momcmd[i];
  }
  cmd[3] = topiclen;
  for (int i = 0; i < topic.length(); i++) {
    cmd[4 + i] = topic[i];
  }
  for (int i = 0; i < messagelen; i++) {
    cmd[topiclen + 4 + i] = *message++;
  }
  cmd[1] = sizeof(cmd) - 2;
  String cmdtemp = "AT+CIPSEND=";
  cmdtemp += sizeof(cmd);
  //sending message
  Serial.println(cmdtemp);
  for (int i = 0; i < sizeof(cmd); i++) {
    Serial.write(cmd[i]);                                                     //publish message
  }
  //ClearIncomingSerial();
#ifdef ALLDEBUG
  mySerial.println(cmdtemp);
  for (int i = 0; i < sizeof(cmd); i++) {
    mySerial.write(cmd[i]);
  }
  mySerial.println();
#endif
  ResetStamp();
  FuncActive = 2;
}

//new sub check doesnt work though
//check for incoming data
void ESP8266::MQTTSubCheck(void (*SubHandle)()) {
  char incoming;
  String topic = "";
  String payload = "";
#ifdef _DEBUG_
  //mySerial.println(F("checking subs"));
#endif
  byte temp[4] = {0, 0, 0, 0};                                     //used to check if the broker is sending messages to the device or if the message was sent successfully
  while (Serial.available()) {
    byte temp2 = Serial.read();

    temp[3] = temp[2];
    temp[2] = temp[1];
    temp[1] = temp[0];

    temp[0] = temp2;
    if (temp[0] == 44 && temp[1] == 68 && temp[2] == 80 && temp[3] == 73) { //search for ,IPD
      String temp = "";
      char inctemp;
      while (Serial.available()) {
        inctemp = Serial.read();
        if (inctemp != 58) {
          temp.concat(inctemp);                                   // read length of data comming in
        } else {
          break;
        }
      }
      byte len = temp.toInt();
      if (Serial.read() == 48) {
        volatile byte len = (byte) Serial.read();
        Sub1->len = len;
        Serial.read();
        volatile byte topiclen = (byte) Serial.read();
        Sub1->topiclen = topiclen;
        volatile byte payloadlen = len - topiclen - 2;
        Sub1->payloadlen = payloadlen;
        for (int i = 0; i <  topiclen; i++) {                     //read actual message received
          topic.concat((char)Serial.read());                              //concatenate the message with the length attached for easy debuging
          delay(2);
        }
        for (int i = 0; i <  payloadlen; i++) {                     //read actual message received
          payload.concat((char)Serial.read());                              //concatenate the message with the length attached for easy debuging
          delay(2);
        }
        Sub1->topic = &topic[0];
        Sub1->payload = &payload[0];
      }
#ifdef _DEBUG_
      else {
        mySerial.print(F("not a Subs msg"));
      }
#endif
      break;
    }
    delay(2);
  }
  SubHandle();
#ifdef _DEBUG_
  if (topic != "") {
    mySerial.print(F("message received:"));
    mySerial.print(Sub1->topic);
    mySerial.print(",");
    mySerial.println(Sub1->payload);
  }
#endif
  FuncActive = 2;
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
    delay(2);
  }
}

//read out serial coms data to end
void ESP8266::ReadSerial() {
#ifdef _DEBUG_
  while (Serial.available()) {
    mySerial.print(Serial.read(), HEX);
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

inline void ESP8266::DebugPrint(int msg) {
#ifdef _DEBUG_
  mySerial.println(msg);
#endif
}

//check that the esp is connected to the wifi
byte ESP8266::WifiCheck(String SSID) {
  byte Wififlag = 1;
  Serial.println(F("AT+CWJAP?"));                                           //requist which SSID the ESP is connected to
#ifdef _DEBUG_
  mySerial.println(F("checking wifi"));
#endif
#ifdef ALLDEBUG
  mySerial.println(SSID.length());
#endif
  delay(100);
  if (Serial.find("+CWJAP:\"")) {                                            //look for response
    for (int i = 0; i < SSID.length() ; i++) {
      char character = Serial.read();
#ifdef ALLDEBUG
      mySerial.write(character);
      mySerial.print(F(","));
      mySerial.write(SSID[i]);
      mySerial.println();
#endif
      if (SSID[i] != character) {                                             //check the connected SSID matches the desired SSID
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
    connectd |= 1;
  } else {
    mySerial.println(F("Not connected to SSID"));
    connectd |= 3;
    connectd ^= 3;
  }
#endif
  return Wififlag;                                                            //return 0 if not desired SSID and 1 if it is the desired SSID
}

//used to initialize the ESPdevice
void ESP8266::initESP8266() {
  if (!WifiCheck(WifiSSID)) {                             //check if wifi is connected to specified SSID
    Connect();                              //connect to specified SSID
  } else {
    connectd |= 1;
  }
}

//used to run an example of proccessing the mqtt code smoothly
void ESP8266::MQTTProcess(void (*SubFunction)(), void (*SubHandle)(), void (*PublishHandle)()) {
  byte tempcon = connectd;
  if ((tempcon & 2) == 2) {
    /*if (millis() % 100 == 0) {                           //determines how often subs or checked
      MQTTSubCheck(SubHandle);
      //parse the received msg to user function
    }
    if (millis() - stamp >= PublishInterval) {               //publish interval
      ResetStamp();
      FuncActive = 1;                                           //publish que function set by user
    }*/
    PUBTaskManager(PublishHandle);
    if (FuncActive == 3) {
      MQTTSubCheck(SubHandle);
    }
  }

  if ((connectd != tempcon) && ((connectd & 3) == 1)) {              //compare connection status with saved status
    ResetStamp();
    FuncActive = 99;                                               //disconnect from broker to be sure ie send DISCONNEC msg to broker
    disconnects ++;
#ifdef _DEBUG_
    mySerial.print(F("Disconnected:"));
    mySerial.println(disconnects);
#endif
  }

  if ((tempcon & 3) == 1) {
    if ((millis() - stamp) >= 5000) {
      FuncActive = 1;
      if ((connectd & 2) != 2) {
        if (allretries >= 10) {
          connectd |= 1;
          connectd ^= 1;
          initESP8266(); //connect to broker without username and password
        }
      }
    }
    CONNECTaskManager();
    if ((connectd & 3) == 3) {
      allretries = 0;
      fails = 0;
      SubFunction();                                              //subscribe again after reconnect
      ResetStamp();
    }
  } else  if ((connectd & 3) == 0) {
    initESP8266();
  }

}

////////////////////////////////////////////
//task manager used to prioritise functions and execute code relating to PUBlish
///////////////////////////////////////////
inline void ESP8266::PUBTaskManager(void (*PublishHandle)()) {
  switch (FuncActive) {
    case 1:
      {
        PublishHandle();
        break;
      }
    case 2:
      {
        FindSENDOK();
        break;
      }
    default:
      {
        break;
      }
  }
}

///////////////////////////////////////
//find a send ok message
///////////////////////////////////////
inline void ESP8266::FindSENDOK() {
  byte sentflag = 0;
  volatile byte temp[7] = {0, 0, 0, 0, 0, 0, 0};                                     //used to check if the broker is sending messages to the device or if the message was sent successfully
  while (Serial.available()) {
    byte temp2 = Serial.read() ;
    if (temp2 == 43) {
#ifdef _DEBUG_
      mySerial.println(F("Breaking out"));
#endif
      FuncActive++;
      return;
    } else {
      for (int i = 6 ; i > 0 ; i --) {
        temp[i] = temp[i - 1];
      }
      temp[0] = temp2;

      if (temp[0] == 84 && temp[1] == 65) {
        connectd |= 3;
        connectd ^= 3;
        Serial.println();
        Serial.println(F("ATE0"));                                          //turn off echo, was a result of a one in a million bug encountered where ESP reset by itself
        break;
      }
      if (temp[0] == 75 && temp[1] == 79 && temp[2] == 32 && temp[3] == 68 && temp[4] == 78 && temp[5] == 69 && temp[6] == 83) { //search for send ok
        sentflag = 1;
        break;
      } else if (temp[0] == 115 && temp[1] == 105 && temp[2] == 32 && temp[3] == 107 && temp[4] == 110 && temp[5] == 105 && temp[6] == 108) { //search for link is, meaning link is not
        connectd |= 2;
        connectd ^= 2;
#ifdef _DEBUG_
        mySerial.println(F("Not linked"));
#endif
        break;
      }
    }
    delay(2);
  }

  if (((millis() - stamp) > waittime) || (sentflag == 1)) {
#ifdef ALLDEBUG
    mySerial.println();
    for (int i = 0; i < 7 ; i++) {
      mySerial.print(temp[i]);
      mySerial.print(",");
    }
#endif
#ifdef _DEBUG_
    //ReadSerial();
#endif
    //check if incoming message before checking if send ok is recieved

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
      if (fails >= failed) {
        connectd |= 2;
        connectd ^= 2;
      }
    }
    FuncActive = 1;
  }
}

////////////////////////////////////////////
//task manager used to prioritise functions and execute code relating to CONNECt
///////////////////////////////////////////
inline void ESP8266::CONNECTaskManager() {
  switch (FuncActive) {
    case 1:
      {
        TCPSTART();
        break;
      }
    case 2:
      {
        LINKED();
        break;
      }
    case 3:
      {
        CONNEC();
        break;
      }
    case 4:
      {
        FINDRESPONSE();
        
        break;
      }
    case 99:
      {
        MQTTDisconnect();
        break;
      }
    case 100:
      {
        TCPClose();
        break;
      }
    default:
      {
        break;
      }
  }
}

////////////////////////////////////////////
//following are functions used for connecting to the broker
///////////////////////////////////////////
//start tcp connection
inline void ESP8266::TCPSTART() {
#ifdef _DEBUG_
  mySerial.println(F("Connecting"));
#endif
  ClearIncomingSerial();
  //setting up tcp connection
  String cmd = "AT+CIPSTART=\"TCP\",\"";                        //setup TCP string to connect to broker
  cmd += SERVER;
  cmd += "\",";
  cmd += PORT;
  Serial.println(cmd);
#ifdef ALLDEBUG
  mySerial.println(cmd);
#endif
#ifdef _DEBUG_
  mySerial.println(F("Checking link"));
#endif
  ResetStamp();
  FuncActive++;
}

//look for Linked
inline void ESP8266::LINKED() {
  byte linkflag = 0;
  volatile byte temp[6] = {0, 0, 0, 0, 0, 0};                                     //used to check if the broker is sending messages to the device or if the message was sent successfully
  while (Serial.available()) {
    byte temp2 = Serial.read() ;
    for (int i = 5 ; i > 0 ; i --) {
      temp[i] = temp[i - 1];
    }
    temp[0] = temp2;

    if (temp[0] == 84 && temp[1] == 65) {
      connectd |= 2;
      connectd ^= 2;
      Serial.println();
      Serial.println(F("ATE0"));                                          //turn off echo, was a result of a one in a million bug encountered where ESP reset by itself
      break;
    }
    if (temp[0] == 100 && temp[1] == 101 && temp[2] == 107 && temp[3] == 110 && temp[4] == 105 && temp[5] == 76 ) { //search for Linked
      linkflag = 1;
      break;
    } else if (temp[0] == 107 && temp[1] == 110 && temp[2] == 105 && temp[3] == 108 && temp[4] == 110 && temp[5] == 85) { //search for Unlink
      linkflag = 2;
      break;
    }

    delay(2);
  }

  if (((millis() - stamp) > waittime) || (linkflag != 0)) {
    if (linkflag == 1)
    {
#ifdef _DEBUG_
      mySerial.println(F("Linked"));
#endif
      FuncActive++;
    }
    else if (linkflag == 2) {
#ifdef _DEBUG_
      mySerial.println(F("Unlink found"));
#endif
      FuncActive = 99;
      ResetStamp();
      allretries++;
    } else {
#ifdef _DEBUG_
      mySerial.println(F("Not linked"));
#endif
      FuncActive = 99;
      ResetStamp();
      allretries++;
    }
  }
}

//send CONNEC
inline void ESP8266::CONNEC() {
#ifdef _DEBUG_
  mySerial.println(F("Sending CONNEC"));
#endif
#ifdef password
  //unsigned char LVL3[16]  = {0x10, 0x00, 0x00, 0x06, 0x4D, 0x51, 0x49, 0x73, 0x64, 0x70, 0x03, 0xC2, 0x00, 0x78, 0x00, 0x00};
  unsigned char LVL4[14]  = {0x10, 0x00, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0xC2, 0x00, 0x78, 0x00, 0x00};
  String DeviceID = ID;
  String Username = username;
  String Password = password;
  byte IDlen = DeviceID.length();                               //constructing connect message to send to broker with parameters given
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
    Serial.write(newcmd[i]);                                    //sending CONNEC message
    Serial.flush();
  }
#ifdef ALLDEBUG
  mySerial.println(cmdtemp);
  for (int i = 0; i < msglen; i++) {
    mySerial.print(newcmd[i]);
    mySerial.print(",");
  }
  mySerial.println();
#endif
#else
  unsigned char LVL4[14]  = {0x10, 0x00, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x02, 0x00, 0x78, 0x00, 0x00};//CONNEC message initialization
  String DeviceID = ID;
  byte IDlen = DeviceID.length();                                 //constructing connect message to send to broker with given parameters
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
    Serial.write(newcmd[i]);                                    //sending CONNEC message
    Serial.flush();
  }
#endif
#ifdef _DEBUG_
  mySerial.println(F("Checking broker response"));
#endif
  ResetStamp();
  FuncActive++;//clearing buffer manualy after sending CONNEC
}

//look for IPD ie response from server
inline void ESP8266::FINDRESPONSE() {
  byte foundflag = 0;
  volatile byte temp[6] = {0, 0, 0, 0, 0, 0};                                     //used to check if the broker is sending messages to the device or if the message was sent successfully
  while (Serial.available()) {
    byte temp2 = Serial.read() ;
    for (int i = 5 ; i > 0 ; i --) {
      temp[i] = temp[i - 1];
    }
    temp[0] = temp2;
    if (temp[0] == 44 && temp[1] == 68 && temp[2] == 80 && temp[3] == 73 && temp[4] == 43) { //search for +IPD,
      foundflag = 1;
      break;
    } else if (temp[0] == 107 && temp[1] == 110 && temp[2] == 105 && temp[3] == 108 && temp[4] == 110 && temp[5] == 85) { //search for Unlink
      foundflag = 2;
      break;
    }
#ifdef _DEBUGp_
    mySerial.println();
    for (int i = 0; i < 7 ; i++) {
      mySerial.write(temp[i]);
      mySerial.print(",");
    }
#endif
    delay(2);
  }

  if (((millis() - stamp) > waittime) || (foundflag != 0)) {
    if (foundflag == 1)
    {
#ifdef _DEBUG_
      mySerial.println(F("Found IPD"));
#endif
      byte len = Serial.read();
      Serial.read();
      unsigned char ack[len];
      for (int i = 0; i < len; i++) {                             //break down the message
        ack[i] = Serial.read();
        delay(2);
      }
      if (ack[0] == 32 && ack[1] == 2 && ack[2] == 0 && ack[3] == 0) {//check the connection was successfull
        connectd |= 2;
#ifdef _DEBUG_
        mySerial.println(F("connected to broker"));
#endif
        FuncActive = 1;
      } else {
#ifdef _DEBUG_
        mySerial.println(F("undable to connect to broker"));
#endif
        FuncActive = 99;
      }
      ResetStamp();
      //FuncActive++;
    }
    else if (foundflag == 2) {
#ifdef _DEBUG_
      mySerial.println(F("Unlink found"));
#endif
      FuncActive = 99;
      ResetStamp();
      allretries++;
    } else {
#ifdef _DEBUG_
      mySerial.println(F("NO IPD, Unkown error check ESP or Internet Access"));
#endif
      FuncActive = 99;
      ResetStamp();
      allretries++;
    }
  }
}

