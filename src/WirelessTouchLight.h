#ifndef WirelessTouchLight_h
#define WirelessTouchLight_h

#include "Main.h"
#include "base\Utils.h"
#include "base\MQTTMan.h"
#include "base\Application.h"

const char appDataPredefPassword[] PROGMEM = "ewcXoCt4HHjZUvY1";

#include "data\status1.html.gz.h"
#include "data\config1.html.gz.h"

#include <Ticker.h>
#include <CapacitiveSensor.h>

//Pin number of send/trigger GPIO
#define SEND_GPIO D6
//Pin number of receive/sense GPIO
#define RECEIVE_GPIO D5
//Pin number of relay
#define RELAY_GPIO D1

//minimum time between 2 touch detection (ms)
#define TOUCH_LATENCY 1000

//Number of Event is the number of events to retain for send
#define NUMBER_OF_EVENTS 16
//number of retry to send event to Home Automation
#define MAX_RETRY_NUMBER 3

class TouchLight : public Application
{
public:
  typedef struct
  {
    bool lightOn;   //light switched ON or OFF
    bool sent;      //lightOn event sent to HA or not
    byte retryLeft; //number of retries left to send lightOn to Home Automation
  } Event;

private:
#define HA_MQTT_GENERIC 0

  typedef struct
  {
    byte type = HA_MQTT_GENERIC;
    uint32_t port = 1883;
    char username[128 + 1] = {0};
    char password[150 + 1] = {0};
    struct
    {
      char baseTopic[64 + 1] = {0};
    } generic;
  } MQTT;

#define HA_PROTO_DISABLED 0
#define HA_PROTO_MQTT 1

  typedef struct
  {
    byte protocol = HA_PROTO_DISABLED;
    char hostname[64 + 1] = {0};
    uint16_t uploadPeriod = 60;
    MQTT mqtt;
  } HomeAutomation;

  CapacitiveSensor *m_capaSensor;
  uint8_t m_samplesNumberOff = 10;
  uint8_t m_samplesNumberOn = 10;
  long m_lastCapaSensorResult = 0;
  unsigned long m_lastTouch = 0;

  Event m_eventsList[NUMBER_OF_EVENTS];
  byte m_nextEventPos = 0;

  HomeAutomation m_ha;
  int m_haSendResult = 1;
  WiFiClient m_wifiClient;

  MQTTMan m_mqttMan;

  void on();
  void off();
  void toggle();

  void mqttConnectedCallback(MQTTMan *mqttMan, bool firstConnection);
  void mqttCallback(char *topic, uint8_t *payload, unsigned int length);

  void SetConfigDefaultValues();
  void ParseConfigJSON(DynamicJsonDocument &doc);
  bool ParseConfigWebRequest(AsyncWebServerRequest *request);
  String GenerateConfigJSON(bool forSaveFile);
  String GenerateStatusJSON();
  bool AppInit(bool reInit);
  const uint8_t *GetHTMLContent(WebPageForPlaceHolder wp);
  size_t GetHTMLContentSize(WebPageForPlaceHolder wp);
  void AppInitWebServer(AsyncWebServer &server, bool &shouldReboot, bool &pauseApplication);
  void AppRun();

public:
  TouchLight(char appId, String fileName);
};

#endif
