# ESP_MQTTClient
A client program for the ESP8266 designed for the
Arduino platform but can be ported to straight C
for other microcontrollers.

Author: Baden Morgan

Code used to operate the eps8266 which is running
the firmaware version 0.9.2.2 AT
view datasheet for AT commands and descriptions
https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/

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

*Programming notes
First when putting in your prarameters dont 
write essays. Try keep them as short 
as possible were ever you can as to 
long an input could cause a stack 
overflow. If you are having this 
problem try truning off ALLDEBUG
which should free up a little bit 
of resources for you to use. Secondly
The example sketch is what i could
come up with and it fairly reliable
and able to reconnect to the server
without user input. There are facilities
to include disconnection to wireless comms
if you need that. The example sketch isnt 
as easy to use as i would have like it to 
be however this is a somewhat complicated 
program and its the best i could do
