//Created by: Mohan Palanisamy
//1. Assumes the MQTT broker has a REST capable HTTP End point similar to Ponte
//This code was tested using a Eclipse Ponte bridge and the
//Arduino Yun Http endpoints from the Bridge Library
//2. PUT has been tested. POST seems to work reasonably well. GET has kinks in it.
//3. Please note there is a Timeout value set here that will determine how long the 
//software serial will wait for a response. Take that in to account in any delay() you 
//program in to your sketch.
//4. Uncomment the _DEBUG flag if you want to see Serial.println()s sent to the Serial Port
//   If you do so, you might want to comment the Serial.begin() in your sketch becuase this opens the serial

#include <SoftwareSerial.h>
#include "ESP8266RESTHelper.h"

#define _DEBUG

//using Digital 2 and 3 pins for connecting to ESP as software serial
#define ESP_RX_PIN 10
#define ESP_TX_PIN 11
#define TIMEOUT 5000

SoftwareSerial debug(ESP_RX_PIN, ESP_TX_PIN);
boolean isESP8266Available = false;

ESP8266RESTHelper::ESP8266RESTHelper()
{
}

//begins a ESP Session. Assumes that Wireless configuration has already been done and connected to network.
//IP address is already set
boolean ESP8266RESTHelper::begin(void)
{

  #if defined(_DEBUG)
  debug.begin(9600);
  debug.println("Debug Serial Port opened...");
  #endif

  Serial.begin(9600);
  Serial.setTimeout(TIMEOUT);

  Serial.println("AT");

  #if defined(_DEBUG)
  debug.println("checking..");
  #endif

  delay(1000);

  if (Serial.find("OK"))
  {
    isESP8266Available = true;
  }

  #if defined(_DEBUG)
  if (isESP8266Available)
  {
    debug.println("ESP 8266 is available and responding..");
   
  }
  else
    debug.println("ESP 8266 is not available..");
  #endif
  
  return isESP8266Available;
}

boolean ESP8266RESTHelper::begin(String WifiNetworkNameSSID, String WifiPassword)
{
 
  #if defined(_DEBUG)
    debug.begin(9600);
    debug.println("Debug Serial Port opened...");
  #endif

  Serial.begin(9600);
  Serial.setTimeout(TIMEOUT);

  Serial.println("AT");

  #if defined(_DEBUG)
    debug.println("checking..");
  #endif

  delay(1000);

  if (Serial.find("OK"))
  {
    isESP8266Available = true;
  }

  #if defined(_DEBUG)
  if (isESP8266Available)
  {
    debug.println("ESP 8266 is available and responding..");
    debug.println("Connecting to WiFi Network: " + WifiNetworkNameSSID);
  }
  else{
    debug.println("ESP 8266 is not available..");
    return false;
  }
  #endif
  Serial.println("AT+CWMODE=1");
  String wifiConnectCommand="AT+CWJAP=\"" + WifiNetworkNameSSID + "\",\"" + WifiPassword + "\"";
  debug.println(wifiConnectCommand);
  Serial.println(wifiConnectCommand);
  delay(2000);
  debug.println("checking");
  if (Serial.find("OK"))
  {
    isESP8266Available = true;
    debug.println("1");
    debug.println("Connected to WiFi Network: " + WifiNetworkNameSSID);
    delay(5000);
  //set the single connection mode
  //Serial.println("AT+CIPMUX=0");
  }else
  {
    debug.println("2");
    debug.println("Can not connect to wifi");
    isESP8266Available = false;
  }
  debug.println("3");
  
   return isESP8266Available;
}

boolean ESP8266RESTHelper::sendMQTTMessage(String server, int port, String topic, String message)
{
  String response = "";
  return httpAction("PUT", server, port, topic, message, &response);
}

//These methods are work in progress. Not tested fully. Use at your own risk and modify as you see fit.
boolean ESP8266RESTHelper::httpPOST(String server, int port, String resourceURI, String content, String* response)
{
  return httpAction("POST", server, port, resourceURI, content, response );
}
//These methods are work in progress. Not tested fully. Use at your own risk and modify as you see fit.
boolean ESP8266RESTHelper::httpPUT(String server, int port, String resourceURI, String content, String* response)
{
  return httpAction("PUT", server, port, resourceURI, content, response );
}
//These methods are work in progress. Not tested fully. Use at your own risk and modify as you see fit.
boolean ESP8266RESTHelper::httpGET(String server, int port, String resourceURI, String* response)
{
  return httpAction("GET", server, port, resourceURI, "", response );
}


boolean ESP8266RESTHelper::httpAction(String httpVerb, String server, int port, String resourceURI, String content, String* response)
{
  if (!isESP8266Available) return false;

  //Prepare all command strings

  //1. prepare TCP Connection AT Command
  String startTCPSession = "AT+CIPSTART=\"TCP\",\"" + server + "\"," + port;

  //2. prepare PUT Request AT Command
  String tcpPacket =
    httpVerb + " " + resourceURI + " HTTP/1.1\r\n"
    "Host: " + server + ":" + port + "\r\n"
    "Accept: */*\r\n";

  if (!httpVerb.equals("GET"))
  {
    tcpPacket = tcpPacket +
                   "Content-Length: " + content.length() + "\r\n"
                   "Content-Type: application/x-www-form-urlencoded\r\n"
                   "\r\n" +
                   content;
  }
  
 // Serial.println(putTCPPacket);
  //3. prepare Length of TCP Packet ATP Command
  String temp_ATCommand = "AT+CIPSEND=";
  String lengthTCPPacket = temp_ATCommand + tcpPacket.length();


  //AT Commands are ready.. Lets Send them after checking at each command.
  //Each check is a blocking call.

#if defined(_DEBUG)
  debug.println("Opening TCP Connection..");
  debug.println("Command : " + startTCPSession);
#endif

  Serial.println(startTCPSession);
  if (Serial.find("Error")){
    return false;
    debug.println("FUCK");
  }
  //if (Serial.find("Linked"))
  else
  {
#if defined(_DEBUG)
    debug.println("Connected..");
    debug.println("Sending Packet Length...");
    debug.println("Command : " + lengthTCPPacket);
#endif

    Serial.println(lengthTCPPacket);

#if defined(_DEBUG)
    debug.println("Sending Packet....");
    debug.println(tcpPacket);
#endif

    //Serial.println(tcpPacket);
    delay(1000);
 
    if (Serial.find("SEND OK"))
    {
#if defined(_DEBUG)
      debug.println("Packet Sent!!!");
#endif
        while (Serial.available())
        {
          String SerialResponse = Serial.readString();
          
          response = &SerialResponse;
        }
   
      //After reading close the connection. Do not want to reuse because the TCP stack on
      //ESP8266 might close the connection after a while.
      //So make sure the connection is closed by sending AT+CIPCLOSE
      Serial.println("AT+CIPCLOSE");
      return true;
    } else
    {
#if defined(_DEBUG)
      debug.println("Sending Packet Failed");
#endif
      return false;
    }
  } 
  /*else
  {
#if defined(_DEBUG)
    debug.println("Cannot Start TCP Session");
#endif

    return false;
  }*/
}


