# ESP_MQTTClient
A client program for the ESP8266 designed for the 
Arduino platform but can be ported to straight C 
for other microcontrollers. There is a Reliance
on Strings for this Library which makes it easy
to use, a more efficient library is being developed
that will get rid of most use of Strings and
making is more reliable and efficient.
A client program for the ESP8266 designed for the
Arduino platform but can be ported to straight C
for other microcontrollers.

##How to use
Add publish messages in the function "PublishQue()"
like the one already in the example as desired.
Change the macro PublishInterval to determine how
often messages are published, in milli seconds.
use SubhQue() to subscribe to different topics
like the one already on the example.
Executing code on the messages received must be
decoded from a string returned by the library which
has the format: byte 0 = entire message length, 
byte 1 = ignore, byte 2 = topic length, remaining
contains topic and message. I have provided an example
that handles toggling an LED on/off with a 1 or 0
message.


##Where to find Firmware
Code used to operate the eps8266 which is running
the firmaware version 0.9.2.2 AT
view datasheet for AT commands and descriptions
https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/

##Flashing firmware(if needed)
For further instructions on how to setup
Your ESP8266 follow the instructions at:
http://www.xess.com/blog/esp8266-reflash/

The code relating to mqtt is all based
on the documentation of mqtt v3.1.1

The module was connecting to a mosquitto
(v1.4.4) server over a local network,
your results may differ depending on
the server you use, please make sure its
not the server before logging an issue.

The functionality was also tested on an
cloudmqtt server successfully.

Please note that This code is not
intended for sale by anyone and is
opensource. If you do intend to use
this software all I ask is that you
give me credit were it is due. By
downloading and using this you accept
to not charge for this software.

##Programming notes
First when putting in your prarameters dont 
write essays. Try keep them as short 
as possible were ever you can as to 
long an input could cause a stack 
overflow. If you are having this 
problem try turning off DEBUG
which should free up a little bit 
of resources for you to use. Secondly
The example sketch is what i could
come up with and it fairly reliable
and able to reconnect to the server
without user input. There are facilities
to include disconnection to wireless comms
if you need that. The example sketch isnt 
as easy to use as I would have liked it to 
be however this is a somewhat complicated 
program and its the best I could do 

##Server notes
A local server like mosquitto running
on a raspberry pi is much more reliable
than a remote server like cloud mqtt in
my expereince so I suggest testing your 
code on a local server before dealing
with a remote server to figure out what
is giving you problems

##Example of Serial Data
This is what you should see first run
once you have put in a broker address
and port, password and username if needed:
```
***	***	***	***	***
Resetting module
checking module connection
Module Ready
turning off echo

ATE0


OK
OK, Conneceted to WIFI
undable to connect to broker
trying to reconnect
********Disconnecting from broker************
connected to broker
hello,5
SEND OK
Subscribed to topic: hello
Topic:device/0, Message:hello world0
SEND OK
Topic:device/0, Message:hello world1
SEND OK
Topic:device/0, Message:hello world2
SEND OK
Topic:device/0, Message:hello world3
SEND OK
Topic:device/0, Message:hello world4
SEND OK
Topic:device/0, Message:hello world5
SEND OK
```

