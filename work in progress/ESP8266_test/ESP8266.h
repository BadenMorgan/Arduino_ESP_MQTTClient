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
#ifndef ESP8266_h
#define ESP8266_h



#include "Wifi_Config.h"

struct SubReceived {
  byte len;
  byte topiclen;
  byte payloadlen;
  char *topic;
  char *payload;
};



class ESP8266 {

  public:
    //settings variables
    int waittime = 2500;
    byte failed = 10;
    int PublishInterval = 5000;

    //standard variable
    byte fails = 0;
    byte allretries = 0;
    int disconnects = 0;
    byte connectd = 0;

    SubReceived *Sub1 = (struct SubReceived*)malloc(sizeof(struct SubReceived));

    ESP8266();
    ESP8266(int SetTimeOut);
    ESP8266(int SetWaitTime, byte setFailed);
    ESP8266(int SetWaitTime, byte setFailed, int newval);
    ESP8266(int SetWaitTime, byte setFailed, int newval, int setPubInterval);
    void InitComms();
    void Connect();
    void MQTTConnect(String broker, int port, String DeviceID);
    void MQTTConnect(String broker, int port, String DeviceID, String Username, String Password);
    void MQTTDisconnect();
    void MQTTPublish(String topic, String message);
    void MQTTPublish(String topic, byte *message, byte msglen);
    void MQTTSubscribe(String topic);
    void MQTTSubCheck(void (*SubHandle)());
    void idler();
    inline void DebugPrint(String msg);
    inline void DebugPrint(int msg);
    byte WifiCheck(String SSID);
    void initESP8266();
    void MQTTProcess(void (*SubFunction)(), void (*SubHandle)(), void (*PublishHandle)());
  private:
    boolean connectWiFi();
    void ClearIncomingSerial();
    void ReadSerial();
    inline void PUBTaskManager(void (*PublishHandle)());
    inline void CONNECTaskManager();
    inline void FindSENDOK();
    inline void TCPSTART();
    inline void LINKED();
    inline void CONNEC();
    inline void FINDRESPONSE();

    uint32_t stamp = 0;
    byte FuncActive = 0;

};

#endif
