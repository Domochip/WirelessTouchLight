#include "WirelessTouchLight.h"

void TouchLight::on()
{
  //do we need to do something
  //yes if ligth is off
  if (!digitalRead(RELAY_GPIO))
  {
    //apply change to output
    digitalWrite(RELAY_GPIO, HIGH);

    //increment nextEventPos to prevent problem if interrupt occurs
    m_nextEventPos = (m_nextEventPos + 1) % NUMBER_OF_EVENTS;

    byte myEventPos = (m_nextEventPos == 0 ? NUMBER_OF_EVENTS : m_nextEventPos) - 1;

    //fillin our "reserved" event line
    m_eventsList[myEventPos].lightOn = true;
    m_eventsList[myEventPos].sent = false;
    m_eventsList[myEventPos].retryLeft = MAX_RETRY_NUMBER;
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

    //increment nextEventPos to prevent problem if interrupt occurs
    m_nextEventPos = (m_nextEventPos + 1) % NUMBER_OF_EVENTS;

    byte myEventPos = (m_nextEventPos == 0 ? NUMBER_OF_EVENTS : m_nextEventPos) - 1;

    //fillin our "reserved" event line
    m_eventsList[myEventPos].lightOn = false;
    m_eventsList[myEventPos].sent = false;
    m_eventsList[myEventPos].retryLeft = MAX_RETRY_NUMBER;
  }
}
void TouchLight::toggle()
{
  //invert output
  digitalWrite(RELAY_GPIO, digitalRead(RELAY_GPIO) == HIGH ? LOW : HIGH);

  //increment nextEventPos to prevent problem if interrupt occurs
  m_nextEventPos = (m_nextEventPos + 1) % NUMBER_OF_EVENTS;

  byte myEventPos = (m_nextEventPos == 0 ? NUMBER_OF_EVENTS : m_nextEventPos) - 1;

  //fillin our "reserved" event line
  m_eventsList[myEventPos].lightOn = (digitalRead(RELAY_GPIO) == HIGH);
  m_eventsList[myEventPos].sent = false;
  m_eventsList[myEventPos].retryLeft = MAX_RETRY_NUMBER;
}

//------------------------------------------
// Connect then Subscribe to MQTT
bool TouchLight::mqttConnect(bool init)
{
  if (!WiFi.isConnected())
    return false;

  char sn[9];
  sprintf_P(sn, PSTR("%08x"), ESP.getChipId());

  //generate clientID
  String clientID(F(APPLICATION1_NAME));
  clientID += sn;

  //Connect
  if (!m_ha.mqtt.username[0])
    m_mqttClient.connect(clientID.c_str());
  else
    m_mqttClient.connect(clientID.c_str(), m_ha.mqtt.username, m_ha.mqtt.password);

  if (m_mqttClient.connected())
  {
    //Subscribe to needed topic
    //prepare topic subscription
    String subscribeTopic = m_ha.mqtt.generic.baseTopic;
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

    switch (m_ha.mqtt.type) //switch on MQTT type
    {
    case HA_MQTT_GENERIC: // mytopic/command
      subscribeTopic += F("command");
      break;
    }

    //subscribe topic
    if (init)
      m_mqttClient.publish(subscribeTopic.c_str(), ""); //make empty publish only for init
    m_mqttClient.subscribe(subscribeTopic.c_str());
  }

  return m_mqttClient.connected();
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
    off();
    break;
  case '1':
    on();
    break;
  case 't':
  case 'T':
    toggle();
    break;
  }
}

//------------------------------------------
//Used to initialize configuration properties to default values
void TouchLight::SetConfigDefaultValues()
{

  m_samplesNumber = 10;

  m_ha.protocol = HA_PROTO_DISABLED;
  m_ha.hostname[0] = 0;

  m_ha.mqtt.type = HA_MQTT_GENERIC;
  m_ha.mqtt.port = 1883;
  m_ha.mqtt.username[0] = 0;
  m_ha.mqtt.password[0] = 0;
  m_ha.mqtt.generic.baseTopic[0] = 0;
};
//------------------------------------------
//Parse JSON object into configuration properties
void TouchLight::ParseConfigJSON(DynamicJsonDocument &doc)
{
  if (!doc[F("samplesNumber")].isNull())
    m_samplesNumber = doc[F("samplesNumber")];

  if (!doc[F("haproto")].isNull())
    m_ha.protocol = doc[F("haproto")];
  if (!doc[F("hahost")].isNull())
    strlcpy(m_ha.hostname, doc[F("hahost")], sizeof(m_ha.hostname));

  if (!doc[F("hamtype")].isNull())
    m_ha.mqtt.type = doc[F("hamtype")];
  if (!doc[F("hamport")].isNull())
    m_ha.mqtt.port = doc[F("hamport")];
  if (!doc[F("hamu")].isNull())
    strlcpy(m_ha.mqtt.username, doc[F("hamu")], sizeof(m_ha.mqtt.username));
  if (!doc[F("hamp")].isNull())
    strlcpy(m_ha.mqtt.password, doc[F("hamp")], sizeof(m_ha.mqtt.password));

  if (!doc[F("hamgbt")].isNull())
    strlcpy(m_ha.mqtt.generic.baseTopic, doc[F("hamgbt")], sizeof(m_ha.mqtt.generic.baseTopic));
};
//------------------------------------------
//Parse HTTP POST parameters in request into configuration properties
bool TouchLight::ParseConfigWebRequest(AsyncWebServerRequest *request)
{

  if (request->hasParam(F("samplesNumber"), true))
    m_samplesNumber = request->getParam(F("samplesNumber"), true)->value().toInt();

  //Parse HA protocol
  if (request->hasParam(F("haproto"), true))
    m_ha.protocol = request->getParam(F("haproto"), true)->value().toInt();

  //if an home Automation protocol has been selected then get common param
  if (m_ha.protocol != HA_PROTO_DISABLED)
  {
    if (request->hasParam(F("hahost"), true) && request->getParam(F("hahost"), true)->value().length() < sizeof(m_ha.hostname))
      strcpy(m_ha.hostname, request->getParam(F("hahost"), true)->value().c_str());
  }

  //Now get specific param
  switch (m_ha.protocol)
  {
  case HA_PROTO_MQTT:

    if (request->hasParam(F("hamtype"), true))
      m_ha.mqtt.type = request->getParam(F("hamtype"), true)->value().toInt();
    if (request->hasParam(F("hamport"), true))
      m_ha.mqtt.port = request->getParam(F("hamport"), true)->value().toInt();
    if (request->hasParam(F("hamu"), true) && request->getParam(F("hamu"), true)->value().length() < sizeof(m_ha.mqtt.username))
      strcpy(m_ha.mqtt.username, request->getParam(F("hamu"), true)->value().c_str());
    char tempPassword[64 + 1] = {0};
    //put MQTT password into temporary one for predefpassword
    if (request->hasParam(F("hamp"), true) && request->getParam(F("hamp"), true)->value().length() < sizeof(tempPassword))
      strcpy(tempPassword, request->getParam(F("hamp"), true)->value().c_str());
    //check for previous password (there is a predefined special password that mean to keep already saved one)
    if (strcmp_P(tempPassword, appDataPredefPassword))
      strcpy(m_ha.mqtt.password, tempPassword);

    switch (m_ha.mqtt.type)
    {
    case HA_MQTT_GENERIC:
      if (request->hasParam(F("hamgbt"), true) && request->getParam(F("hamgbt"), true)->value().length() < sizeof(m_ha.mqtt.generic.baseTopic))
        strcpy(m_ha.mqtt.generic.baseTopic, request->getParam(F("hamgbt"), true)->value().c_str());

      if (!m_ha.hostname[0] || !m_ha.mqtt.generic.baseTopic[0])
        m_ha.protocol = HA_PROTO_DISABLED;
      break;
    }
    break;
  }
  return true;
};
//------------------------------------------
//Generate JSON from configuration properties
String TouchLight::GenerateConfigJSON(bool forSaveFile = false)
{
  String gc('{');

  gc = gc + F("\"samplesNumber\":") + m_samplesNumber;

  gc = gc + F(",\"haproto\":") + m_ha.protocol;
  gc = gc + F(",\"hahost\":\"") + m_ha.hostname + '"';

  //if for WebPage or protocol selected is MQTT
  if (!forSaveFile || m_ha.protocol == HA_PROTO_MQTT)
  {
    gc = gc + F(",\"hamtype\":") + m_ha.mqtt.type;
    gc = gc + F(",\"hamport\":") + m_ha.mqtt.port;
    gc = gc + F(",\"hamu\":\"") + m_ha.mqtt.username + '"';
    if (forSaveFile)
      gc = gc + F(",\"hamp\":\"") + m_ha.mqtt.password + '"';
    else
      gc = gc + F(",\"hamp\":\"") + (__FlashStringHelper *)appDataPredefPassword + '"'; //predefined special password (mean to keep already saved one)

    gc = gc + F(",\"hamgbt\":\"") + m_ha.mqtt.generic.baseTopic + '"';
  }

  gc += '}';

  return gc;
};
//------------------------------------------
//Generate JSON of application status
String TouchLight::GenerateStatusJSON()
{
  String gs('{');

  gs = gs + F("\"lightState\":") + (digitalRead(RELAY_GPIO) == HIGH ? 1 : 0);

  gs = gs + F(",\"has1\":\"");
  switch (m_ha.protocol)
  {
  case HA_PROTO_DISABLED:
    gs = gs + F("Disabled");
    break;
  case HA_PROTO_MQTT:
    gs = gs + F("MQTT Connection State : ");
    switch (m_mqttClient.state())
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

    if (m_mqttClient.state() == MQTT_CONNECTED)
      gs = gs + F("\",\"has2\":\"Last Publish Result : ") + (m_haSendResult ? F("OK") : F("Failed"));

    break;
  }
  gs += '"';

  gs += '}';

  return gs;
};
//------------------------------------------
//code to execute during initialization and reinitialization of the app
bool TouchLight::AppInit(bool reInit)
{
  //Stop MQTT Reconnect
  m_mqttReconnectTicker.detach();
  if (m_mqttClient.connected()) //Issue #598 : disconnect() crash if client not yet set
    m_mqttClient.disconnect();

  //if MQTT used so configure it
  if (m_ha.protocol == HA_PROTO_MQTT)
  {
    //setup MQTT client
    m_mqttClient.setClient(m_wifiClient).setServer(m_ha.hostname, m_ha.mqtt.port).setCallback(std::bind(&TouchLight::mqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    //Connect
    mqttConnect(true);
  }

  m_eventsList[0].lightOn = false;
  m_eventsList[0].sent = false;
  m_eventsList[0].retryLeft = MAX_RETRY_NUMBER;

  for (byte i = 1; i < NUMBER_OF_EVENTS; i++)
  {
    m_eventsList[i].lightOn = false;
    m_eventsList[i].sent = true;
    m_eventsList[i].retryLeft = 0;
  }
  m_nextEventPos = 1;

  if (!reInit)
  {
    pinMode(RELAY_GPIO, OUTPUT);

    m_capaSensor = new CapacitiveSensor(SEND_GPIO, RECEIVE_GPIO);
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
void TouchLight::AppRun()
{
  //if touch latency is over
  if (millis() > (m_lastTouch + TOUCH_LATENCY))
  {
    //measure
    m_lastCapaSensorResult = m_capaSensor->capacitiveSensor(m_samplesNumber);
    //compare last measure with threshold
    if (m_lastCapaSensorResult > 1000)
    {
      m_lastTouch = millis();
      //Serial.println("Toggle"); //DEBUG
      toggle();
    }
  }

  if (m_needMqttReconnect)
  {
    m_needMqttReconnect = false;
    Serial.print(F("MQTT Reconnection : "));
    if (mqttConnect())
      Serial.println(F("OK"));
    else
      Serial.println(F("Failed"));
  }

  //if MQTT required but not connected and reconnect ticker not started
  if (m_ha.protocol == HA_PROTO_MQTT && !m_mqttClient.connected() && !m_mqttReconnectTicker.active())
  {
    Serial.println(F("MQTT Disconnected"));
    //set Ticker to reconnect after 20 or 60 sec (Wifi connected or not)
    m_mqttReconnectTicker.once_scheduled((WiFi.isConnected() ? 20 : 60), [this]() { m_needMqttReconnect = true; m_mqttReconnectTicker.detach(); });
  }

  if (m_ha.protocol == HA_PROTO_MQTT)
    m_mqttClient.loop();

  //for each events in the list starting by nextEventPos
  for (byte evPos = m_nextEventPos, counter = 0; counter < NUMBER_OF_EVENTS; counter++, evPos = (evPos + 1) % NUMBER_OF_EVENTS)
  {
    //if lightOn not yet sent and retryLeft over 0
    if (!m_eventsList[evPos].sent && m_eventsList[evPos].retryLeft)
    {
      //switch on protocol
      switch (m_ha.protocol)
      {
      case HA_PROTO_DISABLED: //nothing to do
        m_eventsList[evPos].sent = true;
        break;

      case HA_PROTO_MQTT:

        //if we are connected
        if (m_mqttClient.connected())
        {
          //prepare topic
          String completeTopic = m_ha.mqtt.generic.baseTopic;

          //check for final slash
          if (completeTopic.length() && completeTopic.charAt(completeTopic.length() - 1) != '/')
            completeTopic += '/';

          switch (m_ha.mqtt.type) //switch on MQTT type
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
          if ((m_haSendResult = m_mqttClient.publish(completeTopic.c_str(), m_eventsList[evPos].lightOn ? "1" : "0")))
            m_eventsList[evPos].sent = true;
        }

        break;
      }

      //if sent failed decrement retry count
      if (!m_eventsList[evPos].sent)
        m_eventsList[evPos].retryLeft--;
    }
  }
}

//------------------------------------------
//Constructor
TouchLight::TouchLight(char appId, String appName) : Application(appId, appName) {}