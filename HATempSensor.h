#pragma once
#include <HADevice.h>
#include <device-types\HASensorNumber.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define DEVICE_SENSOR_COUNT 2 

////////////////////////////////////////////////////////////////////////////////////////////
//
class HATempSensor : public HASensorNumber
{
private:
  byte address[8];
public:
  HATempSensor(const char*id, const NumberPrecision p) : HASensorNumber(id, p) {};
  bool begin(DallasTemperature *interface, int idx);
  bool loop(DallasTemperature *interface);
};

////////////////////////////////////////////////////////////////////////////////////////////
class HATempDevice : public HADevice 
{
private:
  OneWire            _wire;
  DallasTemperature  _Tsensors;
public:
  HATempDevice(int wire_pin);

  HATempSensor sensor; 
  
  bool begin(const byte mac[6], HAMqtt *mqqt);
  bool loop();                                                    // read DS1820 sensors

  String logmsg; 
};

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
