#ifndef Main_h
#define Main_h

#include <arduino.h>

//include Application header file
#include "WirelessTouchLight.h"

#define APPLICATION1_NAME "WTouchLight"
#define APPLICATION1_DESC "DomoChip Wireless Touch Light"
#define APPLICATION1_CLASS TouchLight

#define VERSION_NUMBER "0.8"

#define DEFAULT_AP_SSID "WirelessTouchLight"
#define DEFAULT_AP_PSK "PasswordTouchLight"

//Enable developper mode (SPIFFS editor)
#define DEVELOPPER_MODE 0

//Choose Serial Speed
#define SERIAL_SPEED 115200

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


