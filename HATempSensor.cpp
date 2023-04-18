/*

*/

#include "HATempSensor.h"
#include <HAMqtt.h>
#include <String.h>
#include <DatedVersion.h>
DATED_VERSION(0, 1)
#define DEVICE_NAME  "Remote TempSensor"
#define DEVICE_MODEL "DS1820 MQTT Logger ESP8266"

////////////////////////////////////////////////////////////////////////////////////////////
//#define LOG(s)    Serial.print(s)
#define LOG(s)    

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HATempSensor::begin(DallasTemperature *interface, int idx) {
  return interface->getAddress(address, idx);
}

bool HATempSensor::loop(DallasTemperature *interface) {
  float temp = interface->getTempC(address);
  if (temp < -10.0 || temp > 35.0)
    return false;
 
  return setValue(temp);
}

////////////////////////////////////////////////////////////////////////////////////////////
#define CONSTRUCT_P0(var)       var(#var, HABaseDeviceType::PrecisionP0)
#define CONSTRUCT_P2(var)       var(#var, HABaseDeviceType::PrecisionP2)

#define CONFIGURE_BASE(var, name, class, icon)  var.setName(name); var.setDeviceClass(class); var.setIcon("mdi:" icon)
#define CONFIGURE(var, name, class, icon, unit) CONFIGURE_BASE(var, name, class, icon); var.setUnitOfMeasurement(unit)
#define CONFIGURE_TEMP(var, name, icon)         CONFIGURE(var, name, "temperature", icon, "°C")
#define CONFIGURE_DS(var, icon)                 CONFIGURE_TEMP(var, #var, icon)

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
HATempDevice::HATempDevice(int wire_pin)
: CONSTRUCT_P2(sensor), _wire(wire_pin), _Tsensors(&_wire)
{
  CONFIGURE_DS(sensor,    "thermometer");
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HATempDevice::begin(const byte mac[6], HAMqtt *mqtt) 
{
  
  logmsg.clear();
  setUniqueId(mac, 6);
  setManufacturer("InnoVeer");
  setName(DEVICE_NAME);
  setSoftwareVersion(VERSION);
  setModel(DEVICE_MODEL);

  // boiler
  mqtt->addDeviceType(&sensor);  

  _Tsensors.begin();
  if (_Tsensors.getDeviceCount() < 1)
    logmsg = "ERROR: We have not found the DS sensor";

  bool result = true;
  result &= sensor.begin(&_Tsensors, 0);

  if (!result)
    logmsg = "ERROR: The sensor did not give its address";

  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HATempDevice::loop()
{
  logmsg.clear();
  _Tsensors.requestTemperatures();   // send command to sensors to measure

  bool result = true;
  result &= sensor.loop(&_Tsensors);       // retrieve temp

  if (!result)
    logmsg = "ERROR: getting/setting one of the temeratures";
  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

