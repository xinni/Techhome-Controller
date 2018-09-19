# ESP MQTT JSON Switch Controller

This project shows a super easy way to get started with your own DIY Switch Controller to use with [Home Assistant](https://home-assistant.io/), a sick, open-source Home Automation platform that can do just about anything.

This project requires **some soldering**, I use some PCB board and soldering by hands, but I also designed a PCB for the Switch Tester.


### Supported Features Include
- **WASD 6-Key Cherry MX Switch Tester**
- **DHT22** temperature sensor
- **DHT22** humidity sensor
- **LED** with support for color, flash, fade, and transition
- **Over-the-Air (OTA)** upload from the ArduinoIDE


#### OTA Uploading
This code also supports remote uploading to the ESP8266 using Arduino's OTA library. To utilize this, you'll need to first upload the sketch using the traditional USB method. However, if you need to update your code after that, your WIFI-connected ESP chip should show up as an option under Tools -> Port -> Porch at your.ip.address.xxx. More information on OTA uploading can be found [here](http://esp8266.github.io/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html). Note: You cannot access the serial monitor over WIFI at this point.


### Parts List
- [WASD 6-Key Cherry MX Switch Tester](https://goo.gl/EkDsxH)
- [NodeMCU](https://goo.gl/JZa6Bt)
- [DHT22]()
- [LED]()
- [PCB Board]()
