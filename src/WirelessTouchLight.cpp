#include "WirelessTouchLight.h"

void TouchLight::on()
{
  //do we need to do something
  //yes if ligth is off
  if (!digitalRead(RELAY_GPIO))
  {
    //apply change to output
    digitalWrite(RELAY_GPIO, HIGH);

    //Log
    LOG_SERIAL.println(F("Light On"));

    //increment nextEventPos to prevent problem if interrupt occurs
    _nextEventPos = (_nextEventPos + 1) % NUMBER_OF_EVENTS;

    byte myEventPos = (_nextEventPos == 0 ? NUMBER_OF_EVENTS : _nextEventPos) - 1;

    //fillin our "reserved" event line
    _eventsList[myEventPos].lightOn = true;
    _eventsList[myEventPos].sent = false;
    _eventsList[myEventPos].retryLeft = MAX_RETRY_NUMBER;
  }
}
void TouchLight::off()
{
  //do we need to do something
  //yes if ligth is on
  if (digitalRead(RELAY_GPIO))
  {
    //apply change to output
    digitalWrite(RELAY_GPIO, LOW);

    //Log
    LOG_SERIAL.println(F("Light Off"));

    //increment nextEventPos to prevent problem if interrupt occurs
    _nextEventPos = (_nextEventPos + 1) % NUMBER_OF_EVENTS;

    byte myEventPos = (_nextEventPos == 0 ? NUMBER_OF_EVENTS : _nextEventPos) - 1;

    //fillin our "reserved" event line
    _eventsList[myEventPos].lightOn = false;
    _eventsList[myEventPos].sent = false;
    _eventsList[myEventPos].retryLeft = MAX_RETRY_NUMBER;
  }
}
void TouchLight::toggle()
{
  //invert output
  digitalWrite(RELAY_GPIO, digitalRead(RELAY_GPIO) == HIGH ? LOW : HIGH);

  //Log
  LOG_SERIAL.println(F("Light Toggle"));

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
void TouchLight::mqttConnectedCallback(MQTTMan *mqttMan, bool firstConnection)
{

  //Subscribe to needed topic
  //prepare topic subscription
  String subscribeTopic = _ha.mqtt.generic.baseTopic;

  //Replace placeholders
  MQTTMan::prepareTopic(subscribeTopic);

  switch (_ha.mqtt.type) //switch on MQTT type
  {
  case HA_MQTT_GENERIC: // mytopic/command
    subscribeTopic += F("command");
    break;
  }

  //subscribe topic
  if (firstConnection)
    mqttMan->publish(subscribeTopic.c_str(), ""); //make empty publish only for firstConnection
  mqttMan->subscribe(subscribeTopic.c_str());
}

//------------------------------------------
//Callback used when an MQTT message arrived
void TouchLight::mqttCallback(char *topic, uint8_t *payload, unsigned int length)
{
  //if payload is not 1 byte long return
  if (length != 1)
    return;

  //Do action according to payload
  switch (payload[0])
  {
  case '0':
    _lastTouchMillis = millis(); //set last touch to now to prevent interference during TOUCH_LATENCY
    off();
    break;
  case '1':
    _lastTouchMillis = millis(); //set last touch to now to prevent interference during TOUCH_LATENCY
    on();
    break;
  case 't':
  case 'T':
    _lastTouchMillis = millis(); //set last touch to now to prevent interference during TOUCH_LATENCY
    toggle();
    break;
  }
}

//------------------------------------------
//Used to initialize configuration properties to default values
void TouchLight::setConfigDefaultValues()
{
  _samplesNumberOff = 10;
  _samplesNumberOn = 10;
  _capaSensorThreshold = 15000;

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
void TouchLight::parseConfigJSON(DynamicJsonDocument &doc)
{
  if (!doc[F("samplesNumberOff")].isNull())
    _samplesNumberOff = doc[F("samplesNumberOff")];
  if (!doc[F("samplesNumberOn")].isNull())
    _samplesNumberOn = doc[F("samplesNumberOn")];

  if (!doc[F("haproto")].isNull())
    _ha.protocol = doc[F("haproto")];
  if (!doc[F("hahost")].isNull())
    strlcpy(_ha.hostname, doc[F("hahost")], sizeof(_ha.hostname));

  if (!doc[F("hamtype")].isNull())
    _ha.mqtt.type = doc[F("hamtype")];
  if (!doc[F("hamport")].isNull())
    _ha.mqtt.port = doc[F("hamport")];
  if (!doc[F("hamu")].isNull())
    strlcpy(_ha.mqtt.username, doc[F("hamu")], sizeof(_ha.mqtt.username));
  if (!doc[F("hamp")].isNull())
    strlcpy(_ha.mqtt.password, doc[F("hamp")], sizeof(_ha.mqtt.password));

  if (!doc[F("hamgbt")].isNull())
    strlcpy(_ha.mqtt.generic.baseTopic, doc[F("hamgbt")], sizeof(_ha.mqtt.generic.baseTopic));
};
//------------------------------------------
//Parse HTTP POST parameters in request into configuration properties
bool TouchLight::parseConfigWebRequest(AsyncWebServerRequest *request)
{
  if (request->hasParam(F("samplesNumberOff"), true))
    _samplesNumberOff = request->getParam(F("samplesNumberOff"), true)->value().toInt();
  if (request->hasParam(F("samplesNumberOn"), true))
    _samplesNumberOn = request->getParam(F("samplesNumberOn"), true)->value().toInt();

  //Parse HA protocol
  if (request->hasParam(F("haproto"), true))
    _ha.protocol = request->getParam(F("haproto"), true)->value().toInt();

  //if an home Automation protocol has been selected then get common param
  if (_ha.protocol != HA_PROTO_DISABLED)
  {
    if (request->hasParam(F("hahost"), true) && request->getParam(F("hahost"), true)->value().length() < sizeof(_ha.hostname))
      strcpy(_ha.hostname, request->getParam(F("hahost"), true)->value().c_str());
  }

  //Now get specific param
  switch (_ha.protocol)
  {
  case HA_PROTO_MQTT:

    if (request->hasParam(F("hamtype"), true))
      _ha.mqtt.type = request->getParam(F("hamtype"), true)->value().toInt();
    if (request->hasParam(F("hamport"), true))
      _ha.mqtt.port = request->getParam(F("hamport"), true)->value().toInt();
    if (request->hasParam(F("hamu"), true) && request->getParam(F("hamu"), true)->value().length() < sizeof(_ha.mqtt.username))
      strcpy(_ha.mqtt.username, request->getParam(F("hamu"), true)->value().c_str());
    char tempPassword[64 + 1] = {0};
    //put MQTT password into temporary one for predefpassword
    if (request->hasParam(F("hamp"), true) && request->getParam(F("hamp"), true)->value().length() < sizeof(tempPassword))
      strcpy(tempPassword, request->getParam(F("hamp"), true)->value().c_str());
    //check for previous password (there is a predefined special password that mean to keep already saved one)
    if (strcmp_P(tempPassword, appDataPredefPassword))
      strcpy(_ha.mqtt.password, tempPassword);

    switch (_ha.mqtt.type)
    {
    case HA_MQTT_GENERIC:
      if (request->hasParam(F("hamgbt"), true) && request->getParam(F("hamgbt"), true)->value().length() < sizeof(_ha.mqtt.generic.baseTopic))
        strcpy(_ha.mqtt.generic.baseTopic, request->getParam(F("hamgbt"), true)->value().c_str());

      if (!_ha.hostname[0] || !_ha.mqtt.generic.baseTopic[0])
        _ha.protocol = HA_PROTO_DISABLED;
      break;
    }
    break;
  }
  return true;
};
//------------------------------------------
//Generate JSON from configuration properties
String TouchLight::generateConfigJSON(bool forSaveFile = false)
{
  String gc('{');

  gc = gc + F("\"samplesNumberOff\":") + _samplesNumberOff;
  gc = gc + F(",\"samplesNumberOn\":") + _samplesNumberOn;

  gc = gc + F(",\"haproto\":") + _ha.protocol;
  gc = gc + F(",\"hahost\":\"") + _ha.hostname + '"';

  //if for WebPage or protocol selected is MQTT
  if (!forSaveFile || _ha.protocol == HA_PROTO_MQTT)
  {
    gc = gc + F(",\"hamtype\":") + _ha.mqtt.type;
    gc = gc + F(",\"hamport\":") + _ha.mqtt.port;
    gc = gc + F(",\"hamu\":\"") + _ha.mqtt.username + '"';
    if (forSaveFile)
      gc = gc + F(",\"hamp\":\"") + _ha.mqtt.password + '"';
    else
      gc = gc + F(",\"hamp\":\"") + (__FlashStringHelper *)appDataPredefPassword + '"'; //predefined special password (mean to keep already saved one)

    gc = gc + F(",\"hamgbt\":\"") + _ha.mqtt.generic.baseTopic + '"';
  }

  gc += '}';

  return gc;
};
//------------------------------------------
//Generate JSON of application status
String TouchLight::generateStatusJSON()
{
  String gs('{');

  gs = gs + F("\"lightState\":") + (digitalRead(RELAY_GPIO) == HIGH ? 1 : 0);

  gs = gs + F(",\"lcsr\":") + _lastCapaSensorResult;

  gs = gs + F(",\"has1\":\"");
  switch (_ha.protocol)
  {
  case HA_PROTO_DISABLED:
    gs = gs + F("Disabled");
    break;
  case HA_PROTO_MQTT:
    gs = gs + F("MQTT Connection State : ");
    switch (_mqttMan.state())
    {
    case MQTT_CONNECTION_TIMEOUT:
      gs = gs + F("Timed Out");
      break;
    case MQTT_CONNECTION_LOST:
      gs = gs + F("Lost");
      break;
    case MQTT_CONNECT_FAILED:
      gs = gs + F("Failed");
      break;
    case MQTT_CONNECTED:
      gs = gs + F("Connected");
      break;
    case MQTT_CONNECT_BAD_PROTOCOL:
      gs = gs + F("Bad Protocol Version");
      break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
      gs = gs + F("Incorrect ClientID ");
      break;
    case MQTT_CONNECT_UNAVAILABLE:
      gs = gs + F("Server Unavailable");
      break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
      gs = gs + F("Bad Credentials");
      break;
    case MQTT_CONNECT_UNAUTHORIZED:
      gs = gs + F("Connection Unauthorized");
      break;
    }

    if (_mqttMan.state() == MQTT_CONNECTED)
      gs = gs + F("\",\"has2\":\"Last Publish Result : ") + (_haSendResult ? F("OK") : F("Failed"));

    break;
  }
  gs += '"';

  gs += '}';

  return gs;
};
//------------------------------------------
//code to execute during initialization and reinitialization of the app
bool TouchLight::appInit(bool reInit)
{
  //Stop MQTT
  _mqttMan.disconnect();

  //if MQTT used so configure it
  if (_ha.protocol == HA_PROTO_MQTT)
  {
    //prepare will topic
    String willTopic = _ha.mqtt.generic.baseTopic;
    MQTTMan::prepareTopic(willTopic);
    willTopic += F("connected");

    //setup MQTT
    _mqttMan.setClient(_wifiClient).setServer(_ha.hostname, _ha.mqtt.port);
    _mqttMan.setConnectedAndWillTopic(willTopic.c_str());
    _mqttMan.setConnectedCallback(std::bind(&TouchLight::mqttConnectedCallback, this, std::placeholders::_1, std::placeholders::_2));
    _mqttMan.setCallback(std::bind(&TouchLight::mqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    //Connect
    _mqttMan.connect(_ha.mqtt.username, _ha.mqtt.password);
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
const uint8_t *TouchLight::getHTMLContent(WebPageForPlaceHolder wp)
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
size_t TouchLight::getHTMLContentSize(WebPageForPlaceHolder wp)
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
void TouchLight::appInitWebServer(AsyncWebServer &server, bool &shouldReboot, bool &pauseApplication)
{

  server.on("/setON", HTTP_GET, [this](AsyncWebServerRequest *request) {
    //Turn on the light
    on();

    //return OK
    request->send(200);
  });

  server.on("/setOFF", HTTP_GET, [this](AsyncWebServerRequest *request) {
    //Turn off the light
    off();

    //return OK
    request->send(200);
  });
};

//------------------------------------------
//Run for timer
void TouchLight::appRun()
{
  //if touch latency is over
  if (millis() > (_lastTouchMillis + TOUCH_LATENCY))
  {
    //measure
    _lastCapaSensorResult = _capaSensor->capacitiveSensor((digitalRead(RELAY_GPIO) == HIGH ? _samplesNumberOn : _samplesNumberOff));

    //DEBUG
    //LOG_SERIAL.println(_lastCapaSensorResult);

    //compare last measure with threshold
    if (_lastCapaSensorResult > _capaSensorThreshold)
    {
      _lastTouchMillis = millis();
      toggle();
    }
  }

  if (_ha.protocol == HA_PROTO_MQTT)
    _mqttMan.loop();

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
        if (_mqttMan.connected())
        {
          //prepare topic
          String completeTopic = _ha.mqtt.generic.baseTopic;

          //Replace placeholders
          MQTTMan::prepareTopic(completeTopic);

          switch (_ha.mqtt.type) //switch on MQTT type
          {
          case HA_MQTT_GENERIC: // mytopic/status
            completeTopic += F("status");
            break;
          }

          //send
          if ((_haSendResult = _mqttMan.publish(completeTopic.c_str(), _eventsList[evPos].lightOn ? "1" : "0")))
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