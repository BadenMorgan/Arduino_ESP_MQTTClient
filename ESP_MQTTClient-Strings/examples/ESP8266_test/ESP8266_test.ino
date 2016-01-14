/*
 * Debugging Notes:
 * debugging on port 9  can be turned off by
 * commenting out _DEBUG_  you can also use a
 * more extensive debugger by commenting in 
 * ALLDEBUG
 */
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266.h>

//WIFI Login details
#define SSID "YourSSIDName"
#define Pass "YourSSIDPass"

//connectiong Details for broker CONNEC message
#define SERVER "test.mosquitto.org/"  //host for mosquitto broker
#define PORT 1883 //port number for mosquitto broker
#define password "pass"
#define username "user"
#define ID "esp_device0"
 
//used to count the messages being published and append to each message
int count = 0;

//used for timers to determine when to trigger an mqtt action
uint32_t stamp = 0;
uint32_t stamp3 = 0;
uint32_t stamp4 = 0;

//used to determine at what interval to publish messages
int PublishInterval = 5000;

//Declare esp8266 and setup waittime
ESP8266 esp8266(1000);

//count the amount of times the device has failed its connection loop
byte allretries = 0;

void setup() {
  pinMode(7, OUTPUT);
  pinMode(12, OUTPUT);
  initESP8266();
}



void loop() {
  if (esp8266.RTNConnected()) {                               //check if the connected flag is set or not
    digitalWrite(7, HIGH);                                    //set a LED to indicate the broker connection
  } else {
    digitalWrite(7, LOW);
  }
  MQTTProcess();                                              //run the proccess to check and manage the MQTT connection
}

////////////////////////////////////////
//ESP subscription handling function                
///////////////////////////////////////
/*first parameter is the
 * message received from
 * the library
 */
void SubExec(String topic) {
  //IGNORE
////////////////////////////////
  if (receivedmsg != "") {
    byte sublen = receivedmsg[0];                             //save the entire message length
    byte topiclen = receivedmsg[2];                           //length of topic string
    String topic;                                             
    for (int i = 3 ; i < topiclen + 3; i ++) {                //loop through mssg array and append to the topic string
      topic += receivedmsg[i];
    }
////////////////////////////////
  //PUT YOUR CODE HERE
    if (topic == topic) {                                     //check msg topic for desired topic     
      digitalWrite(12, receivedmsg[topiclen + 3] - 48);       //execute your code here, read byte by byte
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
  String msg = "hello world";                                 //message payload of MQTT package, put your payload here
  String topic = "device/0";                                  //topic of MQTT package, put your topic here
  msg += count;                                               //used to increment msg count to keep msgs unique, you can get rid of this if you want
  count++;'
  esp8266.MQTTPublish(topic, msg);
  //put more publish msgs here if you want
  //esp8266.MQTTPublish(yournewtopic, yournewmsg);                           
}

/////////////////////////
//Subscribe topics list
////////////////////////
void SubhQue(){
  esp8266.MQTTSubscribe("hello");                             //put your subs here with corresponding topics 
}

//////////////////////////////////////////////////////////////////////
//you need not worry about this code too much
//////////////////////////////////////////////////////////////////////

//////////////////////////
//mqtt looping function
/////////////////////////
void MQTTProcess() {
  byte connectdCheck = esp8266.RTNConnected();                //save the connection status
  if (connectdCheck == 1) {
    if (millis() - stamp4 >= 100) {                           //determines how often subs or checked
      stamp4 = millis();
      String receivedMsg = esp8266.MQTTSubCheck()                                       
      SubExec("hello");                                       //parse the received msg to user function
    }
    if (millis() - stamp3 >= PublishInterval) {               //publish interval
      stamp3 = millis();
      PublishQue();                                           //publish que function set by user
    }

  }
  if (!connectdCheck && (millis() - stamp >= 10000)) {
    esp8266.MQTTConnect(SERVER, PORT, ID, username, password);//connect to broker with username and password
    //esp8266.MQTTConnect(SERVER, PORT, ID);                  //connect to broker without username and password
    allretries++;                                             //tracks number of connection tries
    if (!esp8266.RTNConnected()) {
      if (allretries == 3) {
        initESP8266();                                        //reset and re-establish comms with esp
        allretries = 0;                                      
      }
      stamp = millis();
    } else {
      SubhQue();                                              //subscribe again after reconnect
    }
  }
  if (connectdCheck != esp8266.RTNConnected()) {              //compare connection status with saved status
    stamp = millis();
    if (esp8266.RTNConnected() == 0) {                        //if not connected
      esp8266.MQTTDisconnect();                               //disconnect from broker to be sure ie send DISCONNEC msg to broker
    }
  }
}

/////////////////////////////////////////
//esp initialization and broker connect
/////////////////////////////////////////
void initESP8266() {
  if(!esp8266.WifiCheck(SSID)){                               //check if wifi is connected to specified SSID
    esp8266.Connect(SSID, Pass);                              //connect to specified SSID
  }
  esp8266.MQTTConnect(SERVER, PORT, ID, username, password);  //connect to broker with username and password
  //esp8266.MQTTConnect(SERVER, PORT, ID);                    //connect to broker without username and password
  if (esp8266.RTNConnected() == 1) {
    SubhQue();                                                //run subs que
  }
}



