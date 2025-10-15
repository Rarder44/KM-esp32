#include <Arduino.h>


#define LOGGING_ENABLED 1
//#define LOG_ON_WEBSOCKET 1  //NON USARE
#define LOG_ON_SERIAL 1
#define LED_BUILDIN 48


#ifndef _VARIABLES_DEF
#define _VARIABLES_DEF

const String MDNS_name = "KVESP32"; // Nome del server per mDNS
const uint16_t webServerPort = 80;
const String webSocketEndpoint = "/ws";

#endif