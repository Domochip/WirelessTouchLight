#ifndef Main_h
#define Main_h

#include <arduino.h>


#define APPLICATION1_HEADER "WirelessTouchLight.h"
#define APPLICATION1_NAME "WTouchLight"
#define APPLICATION1_DESC "DomoChip Wireless Touch Light"
#define APPLICATION1_CLASS TouchLight

#define VERSION_NUMBER "1.1"

#define DEFAULT_AP_SSID "WirelessTouchLight"
#define DEFAULT_AP_PSK "PasswordTouchLight"

//Enable status webpage EventSource
#define ENABLE_STATUS_EVENTSOURCE 0

//Enable developper mode (SPIFFS editor)
#define DEVELOPPER_MODE 0

//Log Serial Object
#define LOG_SERIAL Serial
//Choose Log Serial Speed
#define LOG_SERIAL_SPEED 115200

//Choose Pin used to boot in Rescue Mode
//#define RESCUE_BTN_PIN 2

//Define time to wait for Rescue press (in s)
//#define RESCUE_BUTTON_WAIT 3

//Status LED
//#define STATUS_LED_SETUP pinMode(XX, OUTPUT);pinMode(XX, OUTPUT);
//#define STATUS_LED_OFF digitalWrite(XX, HIGH);digitalWrite(XX, HIGH);
//#define STATUS_LED_ERROR digitalWrite(XX, HIGH);digitalWrite(XX, HIGH);
//#define STATUS_LED_WARNING digitalWrite(XX, HIGH);digitalWrite(XX, HIGH);
//#define STATUS_LED_GOOD digitalWrite(XX, HIGH);digitalWrite(XX, HIGH);

//construct Version text
#if DEVELOPPER_MODE
#define VERSION VERSION_NUMBER "-DEV"
#else
#define VERSION VERSION_NUMBER
#endif

#endif


