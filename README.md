# WiFiMoistureSensor

Arduino code for an ESP8266 based wifi enabled moisture sensor.

## Setup
My setup uses a Wemos D1 Mini, a Capacitive Soil Mositure Sensor v1.2, a lithium ion battery charging module, and an 18650 battery. 

The Moisture sensor goes to 3.3v on the wemos, ground, and A0. RST and D0 need to be tied together to enable deep sleep mode. The 18650 goes to the charging module B+ and B+ while out + and out - go to the 3.3v pin and ground on the wemos d1 mini. 

When you short RST and D0 the wemos will not allow for uploading, so I the jumper removeable to allow for future updates to the code.

Before uploading your sketch, set the mqtt server, user, and password.

After uploading the sketch to the wemos it will enter configuration mode. First you'll need to get a dry reading from the board by letting it cycle and looking for the moisture value while the sensor isn't in contact with anything. Write it down. Then dip the sensor in water (only up to the water line keeping the components out of the liquid), and reset the module. Get the moisture value and note it as the "water value". Then connect to the "MoistSensorSetup" AP, opt to configure wireless and connect to your AP making sure to enter the air (dry) value and the water value into the correct fields.

### Todo
* Add the MQTT details to the configuration wizard.
* Figure out a config process that walks a user through configuring the dry/wet values within the interface.
* Add schematic or video instructions on how to build
* Add .stl file for 3d printed case.
