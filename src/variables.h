#include <Arduino.h>


#define LOGGING_ENABLED 1
//#define LOG_ON_WEBSOCKET 1  //con messaggi troppo veloci, la websocket si satura e si chiude (se va male, crasha tutto)
#define LOG_ON_SERIAL 1
#define LED_BUILDIN 48


#ifndef _VARIABLES_DEF
#define _VARIABLES_DEF

const String MDNS_name = "KVESP32"; // Nome del server per mDNS
const uint16_t webServerPort = 80;
const String webSocketEndpoint = "/ws";


//AP
const int wifi_timeOut=5000;//30000; //millescond
const String AP_ssid="KVESP32_AP";
const String AP_password="";

IPAddress local_ip(192, 168, 4, 1);    // IP dell'AP  
IPAddress gateway(192, 168, 4, 1);    // Gateway  
IPAddress subnet(255, 255, 255, 0);   // Subnet mask


    //WIFI
    #if defined(__has_include)
    #if __has_include("wifi_credential.h")
        #include "wifi_credential.h"
    #else
        // fallback: valori di default se il file non esiste
        #define SSID_DEFAULT "ssid"
        #define PSW_DEFAULT  "password"
    #endif
    #else
    // se il compilatore non supporta __has_include,
    // puoi o includere il file normalmente (potrebbe fallire)
    // o fornire i default qui e aspettarti che il file venga fornito tramite build system.
    #include "wifi_credential.h" // tenterà di includere normalmente
    #endif


#endif