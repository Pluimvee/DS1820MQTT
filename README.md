# Remote temperature sensor
Monitor for remote temperature using one Dallas DS1820 temperature sensor. Its used to measure the actual outside temperatures in the garden.

This code is running on a 12F ESP8266 reading a temperature sensor using OneWire and broadcasting the readings to MQTT compatible to the Home Automation auto-discovery. Its my first 'pure' ESP 12F module (not WEMOS) using a 230Vac to 3.3Vdc convertor. It supports DeepSleep, OTA, remote logging, MQTT and HA integration.
This project serves as step-up to a low-power consuming, battery powered, module.

