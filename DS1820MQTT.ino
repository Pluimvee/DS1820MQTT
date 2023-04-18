#include "HATempSensor.h"
#include <ESP8266WiFi.h>
#include <HAMqtt.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <Clock.h>
#include <Timer.h>
#include <LED.h>
#include <DatedVersion.h>
DATED_VERSION(0, 2)

////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
const char* sta_ssid      = STA_SSID;
const char* sta_pswd      = STA_PASS;

const char* mqtt_server   = "192.168.2.170";   // test.mosquitto.org"; //"broker.hivemq.com"; //6fee8b3a98cd45019cc100ef11bf505d.s2.eu.hivemq.cloud";
int         mqtt_port     = 1883;             // 8883;
const char* mqtt_user     = MQTT_USER;
const char *mqtt_passwd   = MQTT_PASS;

////////////////////////////////////////////////////////////////////////////////////////////
// Global instances
WiFiClient        socket;
HATempDevice      remote_temp(D1);                     // THe Floorheat monitor with all of its sensors
HAMqtt            mqtt(socket, remote_temp, DEVICE_SENSOR_COUNT);  // Home Assistant MTTQ
Clock             rtc;                            // A real (software) time clock
LED               led;                            // 

////////////////////////////////////////////////////////////////////////////////////////////
// For remote logging the log include needs to be after the global MQTT definition
#define LOG_REMOTE
#define LOG_LEVEL 2
#include <Logging.h>

void LOG_CALLBACK(char *msg) { 
  LOG_REMOVE_NEWLINE(msg);
  mqtt.publish("RemoteTemp/log", msg, true); 
}

////////////////////////////////////////////////////////////////////////////////////////////
// Connect to the STA network
void wifi_connect() 
{ 
  if (((WiFi.getMode() & WIFI_STA) == WIFI_STA) && WiFi.isConnected())
    return;

  DEBUG("Wifi connecting to %s.", sta_ssid);
  WiFi.begin(sta_ssid, sta_pswd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG(".");
  }
  DEBUG("\n");
  INFO("WiFi connected with IP address: %s\n", WiFi.localIP().toString().c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////
// MQTT Connect
void mqtt_connect() {
  INFO("Remote temperature v%s saying hello\n", VERSION);
}


///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
DateTime bedtime;

void sync_clock()
{
  rtc.ntp_sync();
  DateTime now = rtc.now();
  DEBUG("Clock synchronized to %s\n", now.timestamp().c_str());
  
  // set deepsleep for 23:00 today, or if its already past we set the deepsleep for 5 minutes past 0:00
  bedtime = now + TimeSpan(0,12,0,0);  // RE SYNC IN 12 HOURS

  INFO("[%s] - Clock synchronized and resync at %s\n", 
              now.timestamp(DateTime::TIMESTAMP_TIME).c_str(), 
              bedtime.timestamp().c_str());
}

///////////////////////////////////////////////////////////////////////////////////////
bool deep_sleep(DateTime &now)
{
#if (0) // enable to use deepsleep
  // deepsleep requires D0 (GPIO16) to be hard wired with the RST pin. According to some web pages this may cause issues uploading firmware
  // we need to test if this also applies to OTA. So for now we stick to a Restart instead of Sleep

  // TODO: instead of 5:30 use sunrise - 30 minutes
  DateTime wakeupTime(now.year(), now.month(), now.day(), now.hour(), now.minute()+2, 0);    // note the time constructed might be invalid due to the incremented day
  INFO("[%s] - Its time to enter Deepsleep, I have set the wakeup time for %s\n", 
          now.timestamp(DateTime::TIMESTAMP_TIME).c_str(),
          wakeupTime.timestamp().c_str());
  int secondsUntilWakeup = wakeupTime.unixtime() - now.unixtime();      // however the unixtime returned will still be valid

  INFO("We are about to completely turn of the Wifi, so this is the last you here from me\n");
  INFO("According to my calculations I will wake up in %d seconds.... CU!!\n", secondsUntilWakeup);
  // wait 2 seconds
  delay(2000);  
  // SLEEPING........
  ESP.deepSleep(secondsUntilWakeup * 1e6);//, RF_DISABLED);
#endif
  // default -> Sync clock and reconfigure next sync/reboot/sleep time
  sync_clock(); 
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////
// the main method which determines what we will be doing
///////////////////////////////////////////////////////////////////////////////////////
Timer T_Throttle;

enum ProxyMode {
  NOTHING,
  BLINK,
  SLEEP
};

int scheduler(DateTime &now)
{
  if (now > bedtime)  // its past bedtime
    return SLEEP;

  if (T_Throttle.passed()) {
    T_Throttle.set(5000);
    return BLINK;         // most of the time we are in receiving mode
  }
  return NOTHING;
}

///////////////////////////////////////////////////////////////////////////////////////
// Main Setup 
///////////////////////////////////////////////////////////////////////////////////////
void setup() 
{
  Serial.begin(115200);
  INFO("\nRemote Sensor\n");
  wifi_connect();
  // start MQTT to enable remote logging asap
  INFO("Connecting to MQTT server %s\n", mqtt_server);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  remote_temp.begin(mac, &mqtt);              // 5) make sure the device gets a unique ID (based on mac address)
  mqtt.onConnected(mqtt_connect);           // register function called when newly connected
  mqtt.begin(mqtt_server, mqtt_port, mqtt_user, mqtt_passwd);  // 

  sync_clock(); 

  INFO("Initialize OTA\n");
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("RemoteTempSensor");
  ArduinoOTA.setPassword(OTA_PASS);

  ArduinoOTA.onStart([]() {
    INFO("Starting remote software update");
  });
  ArduinoOTA.onEnd([]() {
    INFO("Remote software update finished");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ERROR("Error remote software update");
  });
  ArduinoOTA.begin();
  INFO("Setup complete\n\n");
}

///////////////////////////////////////////////////////////////////////////////////////
// Main loop
///////////////////////////////////////////////////////////////////////////////////////
void loop() 
{
  // ensure we are still connected (STA-mode)
  wifi_connect();
  // handle any OTA requests
  ArduinoOTA.handle();
  // handle MQTT
  mqtt.loop();
  // whats the time
  DateTime now = rtc.now();
  // now lets deterime what we are going to do
  switch (scheduler(now))
  {
  default:
  case NOTHING:
    break;
  case SLEEP:
    deep_sleep(now);
    break;
  case BLINK:
    led.blink();
    if (!remote_temp.loop())
      ERROR("[%s] - %s\n", 
            rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str(), 
            remote_temp.logmsg.c_str());
    INFO("[%s] - Temperature: %f\n", 
            now.timestamp(DateTime::TIMESTAMP_TIME).c_str(), 
            remote_temp.sensor.getCurrentValue().toFloat());
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
