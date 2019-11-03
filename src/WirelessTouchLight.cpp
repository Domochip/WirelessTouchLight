#include "WirelessTouchLight.h"

void TouchLight::On()
{
  //do we need to do something
  //yes if ligth is off
  if (!digitalRead(RELAY_GPIO))
  {
    //apply change to output
    digitalWrite(RELAY_GPIO, HIGH);

    //increment nextEventPos to prevent problem if interrupt occurs
    _nextEventPos = (_nextEventPos + 1) % NUMBER_OF_EVENTS;

    byte myEventPos = (_nextEventPos == 0 ? NUMBER_OF_EVENTS : _nextEventPos) - 1;

    //fillin our "reserved" event line
    _eventsList[myEventPos].lightOn = true;
    _eventsList[myEventPos].sent = false;
    _eventsList[myEventPos].retryLeft = MAX_RETRY_NUMBER;
  }
}
void TouchLight::Off()
{
  //do we need to do something
  //yes if ligth is on
  if (digitalRead(RELAY_GPIO))
  {
    //apply change to output
    digitalWrite(RELAY_GPIO, LOW);

    //increment nextEventPos to prevent problem if interrupt occurs
    _nextEventPos = (_nextEventPos + 1) % NUMBER_OF_EVENTS;

    byte myEventPos = (_nextEventPos == 0 ? NUMBER_OF_EVENTS : _nextEventPos) - 1;

    //fillin our "reserved" event line
    _eventsList[myEventPos].lightOn = false;
    _eventsList[myEventPos].sent = false;
    _eventsList[myEventPos].retryLeft = MAX_RETRY_NUMBER;
  }
}
void TouchLight::Toggle()
{
  //invert output
  digitalWrite(RELAY_GPIO, digitalRead(RELAY_GPIO) == HIGH ? LOW : HIGH);

  //increment nextEventPos to prevent problem if interrupt occurs
  _nextEventPos = (_nextEventPos + 1) % NUMBER_OF_EVENTS;

  byte myEventPos = (_nextEventPos == 0 ? NUMBER_OF_EVENTS : _nextEventPos) - 1;

  //fillin our "reserved" event line
  _eventsList[myEventPos].lightOn = (digitalRead(RELAY_GPIO) == HIGH);
  _eventsList[myEventPos].sent = false;
  _eventsList[myEventPos].retryLeft = MAX_RETRY_NUMBER;
}

//------------------------------------------
// Connect then Subscribe to MQTT
bool TouchLight::MqttConnect(bool init)
{
  if (!WiFi.isConnected())
    return false;

  char sn[9];
  sprintf_P(sn, PSTR("%08x"), ESP.getChipId());

  //generate clientID
  String clientID(F(APPLICATION1_NAME));
  clientID += sn;

  //Connect
  if (!_ha.mqtt.username[0])
    _mqttClient.connect(clientID.c_str());
  else
    _mqttClient.connect(clientID.c_str(), _ha.mqtt.username, _ha.mqtt.password);

  if (_mqttClient.connected())
  {
    //Subscribe to needed topic
    //prepare topic subscription
    String subscribeTopic = _ha.mqtt.generic.baseTopic;
    //check for final slash
    if (subscribeTopic.length() && subscribeTopic.charAt(subscribeTopic.length() - 1) != '/')
      subscribeTopic += '/';

    //Replace placeholders
    if (subscribeTopic.indexOf(F("$sn$")) != -1)
      subscribeTopic.replace(F("$sn$"), sn);

    if (subscribeTopic.indexOf(F("$mac$")) != -1)
      subscribeTopic.replace(F("$mac$"), WiFi.macAddress());

    if (subscribeTopic.indexOf(F("$model$")) != -1)
      subscribeTopic.replace(F("$model$"), APPLICATION1_NAME);

    switch (_ha.mqtt.type) //switch on MQTT type
    {
    case HA_MQTT_GENERIC: // mytopic/command
      subscribeTopic += F("command");
      break;
    }

    //subscribe topic
    if (init)
      _mqttClient.publish(subscribeTopic.c_str(), ""); //make empty publish only for init
    _mqttClient.subscribe(subscribeTopic.c_str());
  }

  return _mqttClient.connected();
}

//------------------------------------------
//Callback used when an MQTT message arrived
void TouchLight::MqttCallback(char *topic, uint8_t *payload, unsigned int length)
{
  //if payload is not 1 byte long return
  if (length != 1)
    return;

  //Do action according to payload
  switch (payload[0])
  {
  case '0':
    Off();
    break;
  case '1':
    On();
    break;
  case 't':
  case 'T':
    Toggle();
    break;
  }
}

//------------------------------------------
//Used to initialize configuration properties to default values
void TouchLight::SetConfigDefaultValues()
{

  _ha.protocol = HA_PROTO_DISABLED;
  _ha.hostname[0] = 0;

  _ha.mqtt.type = HA_MQTT_GENERIC;
  _ha.mqtt.port = 1883;
  _ha.mqtt.username[0] = 0;
  _ha.mqtt.password[0] = 0;
  _ha.mqtt.generic.baseTopic[0] = 0;
};
//------------------------------------------
//Parse JSON object into configuration properties
void TouchLight::ParseConfigJSON(DynamicJsonDocument &doc){
    //TODO
    //if (!doc["prop1"].isNull()) property1 = doc[F("prop1")];
    //if (!doc["prop2"].isNull()) strlcpy(property2, doc["prop2"], sizeof(property2));
};
//------------------------------------------
//Parse HTTP POST parameters in request into configuration properties
bool TouchLight::ParseConfigWebRequest(AsyncWebServerRequest *request)
{
  //TODO
  // if (!request->hasParam(F("prop1"), true))
  // {
  //     request->send(400, F("text/html"), F("prop1 missing"));
  //     return false;
  // }
  //if (request->hasParam(F("prop1"), true)) property1 = request->getParam(F("prop1"), true)->value().toInt();
  //if (request->hasParam(F("prop2"), true) && request->getParam(F("prop2"), true)->value().length() < sizeof(property2)) strcpy(property2, request->getParam(F("prop2"), true)->value().c_str());

  return true;
};
//------------------------------------------
//Generate JSON from configuration properties
String TouchLight::GenerateConfigJSON(bool forSaveFile = false)
{
  String gc('{');
  //TODO
  // gc = gc + F("\"p1\":") + (property1 ? true : false);
  // gc = gc + F("\"p2\":\"") + property2 + '"';

  gc += '}';

  return gc;
};
//------------------------------------------
//Generate JSON of application status
String TouchLight::GenerateStatusJSON()
{
  String gs('{');

  //TODO
  // gs = gs + F("\"p1\":") + (property1 ? true : false);
  // gs = gs + F(",\"p2\":\"") + property2 + '"';

  gs += '}';

  return gs;
};
//------------------------------------------
//code to execute during initialization and reinitialization of the app
bool TouchLight::AppInit(bool reInit)
{
  //Stop MQTT Reconnect
  _mqttReconnectTicker.detach();
  if (_mqttClient.connected()) //Issue #598 : disconnect() crash if client not yet set
    _mqttClient.disconnect();

  //if MQTT used so configure it
  if (_ha.protocol == HA_PROTO_MQTT)
  {
    //setup MQTT client
    _mqttClient.setClient(_wifiClient).setServer(_ha.hostname, _ha.mqtt.port).setCallback(std::bind(&TouchLight::MqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    //Connect
    MqttConnect(true);
  }

  _eventsList[0].lightOn = false;
  _eventsList[0].sent = false;
  _eventsList[0].retryLeft = MAX_RETRY_NUMBER;

  for (byte i = 1; i < NUMBER_OF_EVENTS; i++)
  {
    _eventsList[i].lightOn = false;
    _eventsList[i].sent = true;
    _eventsList[i].retryLeft = 0;
  }
  _nextEventPos = 1;

  if (!reInit)
  {
    pinMode(RELAY_GPIO, OUTPUT);

    _capaSensor = new CapacitiveSensor(SEND_GPIO, RECEIVE_GPIO);
  }

  return true;
};
//------------------------------------------
//Return HTML Code to insert into Status Web page
const uint8_t *TouchLight::GetHTMLContent(WebPageForPlaceHolder wp)
{
  switch (wp)
  {
  case status:
    return (const uint8_t *)status1htmlgz;
    break;
  case config:
    return (const uint8_t *)config1htmlgz;
    break;
  default:
    return nullptr;
    break;
  };
  return nullptr;
};
//and his Size
size_t TouchLight::GetHTMLContentSize(WebPageForPlaceHolder wp)
{
  switch (wp)
  {
  case status:
    return sizeof(status1htmlgz);
    break;
  case config:
    return sizeof(config1htmlgz);
    break;
  default:
    return 0;
    break;
  };
  return 0;
};

//------------------------------------------
//code to register web request answer to the web server
void TouchLight::AppInitWebServer(AsyncWebServer &server, bool &shouldReboot, bool &pauseApplication)
{

  server.on("/setON", HTTP_GET, [this](AsyncWebServerRequest *request) {
    //Turn on the light
    On();

    //return OK
    request->send(200);
  });

  server.on("/setOFF", HTTP_GET, [this](AsyncWebServerRequest *request) {
    //Turn off the light
    Off();

    //return OK
    request->send(200);
  });
};

//------------------------------------------
//Run for timer
void TouchLight::AppRun()
{
  //if touch latency is over
  if (millis() > (_lastTouch + TOUCH_LATENCY))
  {
    //measure
    _lastCapaSensorResult = _capaSensor->capacitiveSensor(_samplesNumber);
    //compare last measure with threshold
    if (_lastCapaSensorResult > 1000)
    {
      _lastTouch = millis();
      //Serial.println("Toggle"); //DEBUG
      Toggle();
    }
  }

  if (_needMqttReconnect)
  {
    _needMqttReconnect = false;
    Serial.print(F("MQTT Reconnection : "));
    if (MqttConnect())
      Serial.println(F("OK"));
    else
      Serial.println(F("Failed"));
  }

  //if MQTT required but not connected and reconnect ticker not started
  if (_ha.protocol == HA_PROTO_MQTT && !_mqttClient.connected() && !_mqttReconnectTicker.active())
  {
    Serial.println(F("MQTT Disconnected"));
    //set Ticker to reconnect after 20 or 60 sec (Wifi connected or not)
    _mqttReconnectTicker.once_scheduled((WiFi.isConnected() ? 20 : 60), [this]() { _needMqttReconnect = true; _mqttReconnectTicker.detach(); });
  }

  if (_ha.protocol == HA_PROTO_MQTT)
    _mqttClient.loop();

  //for each events in the list starting by nextEventPos
  for (byte evPos = _nextEventPos, counter = 0; counter < NUMBER_OF_EVENTS; counter++, evPos = (evPos + 1) % NUMBER_OF_EVENTS)
  {
    //if lightOn not yet sent and retryLeft over 0
    if (!_eventsList[evPos].sent && _eventsList[evPos].retryLeft)
    {
      //switch on protocol
      switch (_ha.protocol)
      {
      case HA_PROTO_DISABLED: //nothing to do
        _eventsList[evPos].sent = true;
        break;

      case HA_PROTO_MQTT:

        //if we are connected
        if (_mqttClient.connected())
        {
          //prepare topic
          String completeTopic = _ha.mqtt.generic.baseTopic;

          //check for final slash
          if (completeTopic.length() && completeTopic.charAt(completeTopic.length() - 1) != '/')
            completeTopic += '/';

          switch (_ha.mqtt.type) //switch on MQTT type
          {
          case HA_MQTT_GENERIC: // mytopic/status
            completeTopic += F("status");
            break;
          }

          //prepare sn for placeholder
          char sn[9];
          sprintf_P(sn, PSTR("%08x"), ESP.getChipId());

          //Replace placeholders
          if (completeTopic.indexOf(F("$sn$")) != -1)
            completeTopic.replace(F("$sn$"), sn);

          if (completeTopic.indexOf(F("$mac$")) != -1)
            completeTopic.replace(F("$mac$"), WiFi.macAddress());

          if (completeTopic.indexOf(F("$model$")) != -1)
            completeTopic.replace(F("$model$"), APPLICATION1_NAME);

          //send
          if ((_haSendResult = _mqttClient.publish(completeTopic.c_str(), _eventsList[evPos].lightOn ? "1" : "0")))
            _eventsList[evPos].sent = true;
        }

        break;
      }

      //if sent failed decrement retry count
      if (!_eventsList[evPos].sent)
        _eventsList[evPos].retryLeft--;
    }
  }
}

//------------------------------------------
//Constructor
TouchLight::TouchLight(char appId, String appName) : Application(appId, appName) {}