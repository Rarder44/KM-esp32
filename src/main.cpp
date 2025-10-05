//LEGGIMI
//prima dell'upload occorre tenere premuto il tasto boot e premere reset
//l'esp si collegherà come COM e non tastiera/mouse


#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include "USB.h"
#include "USBHIDMouse.h"
#include "USBHIDKeyboard.h"
#include <Adafruit_NeoPixel.h>
#include <UrlEncode.h>
#include <ESPmDNS.h> 

#define LOGGING_ENABLED 1
#define LOG_ON_WEB 1
#define LED_BUILDIN 48

#include "wifi_credential.h"

//the file "wifi_credential.h" must contains these 2 variables
//const char* ssid = "ssid";         
//const char* password = "password"; 



const unsigned int localUdpPort = 4210; // Porta UDP di ascolto
const String serverName = "KVESP32"; // Nome del server per mDNS


Adafruit_NeoPixel strip(1, LED_BUILDIN, NEO_GRB + NEO_KHZ800);



USBHIDMouse Mouse;
USBHIDKeyboard Keyboard;


WiFiUDP udp;


void manageKeyboardEvent(char incomingPacket[]);
void manageMouseEvent(char incomingPacket[]);
void manageCopyEvent(char incomingPacket[]);


// Log function using Serial.printf
size_t Log(const char *format, ...)
{
  #if LOGGING_ENABLED
    char loc_buf[128];
        char *temp = loc_buf;
        va_list arg;
        va_list copy;
        va_start(arg, format);
        va_copy(copy, arg);
        int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
        va_end(copy);

        if (len < 0) {
            va_end(arg);
            return 0;
        }
        if (len >= (int)sizeof(loc_buf)) {
            temp = (char *)malloc(len + 1);
            if (temp == NULL) {
                va_end(arg);
                return 0;
            }
            len = vsnprintf(temp, len + 1, format, arg);
        }
        va_end(arg);

    #if LOG_ON_WEB
        // Costruisci la URL
        String msg = urlEncode(String(temp));

        HTTPClient http;
        http.begin("http://192.168.1.66:5000/LOG");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String payload = "msg=" + msg;
        int httpCode = http.POST(payload);
        http.end();


    #else
        /*char loc_buf[64];
        char * temp = loc_buf;
        va_list arg;
        va_list copy;
        va_start(arg, format);
        va_copy(copy, arg);
        int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
        va_end(copy);
        if(len < 0) {
            va_end(arg);
            return 0;
        }
        if(len >= (int)sizeof(loc_buf)){  // comparation of same sign type for the compiler
            temp = (char*) malloc(len+1);
            if(temp == NULL) {
                va_end(arg);
                return 0;
            }
            len = vsnprintf(temp, len+1, format, arg);
        }
        va_end(arg);*/
        len = Serial.write((uint8_t*)temp, len);
        
      #endif

      if(temp != loc_buf){
            free(temp);
      }
      return len;
    #else
      return 0;
    #endif
}

void LogColor(int R,int G, int B)
{
  uint32_t color = strip.Color(R,G,B);
  strip.setPixelColor(0, color);
  strip.show();
}

enum keyEvents {
  KEY_UP=0,
  KEY_DOWN=1,
  KEY_CLICK=2,
};

enum mouseEvents {
  MOUSE_UP=0,
  MOUSE_DOWN=1,
  MOUSE_CLICK=2,
  MOUSE_MOVE,
};

enum Events{
  NO_EVENT=0,
  KEYBOARD=1,
  MOUSE=2,
  COPY=3
};

//<uint8_t | Events> <uint8_t keyEvents | mouseEvents> <value>


//keyEvents -> KEY_UP | KEY_DOWN | KEY_CLICK
  //<value> -> virtual key to send

//mouseEvents -> MOUSE_UP | MOUSE_DOWN | MOUSE_CLICK
  //<value> -> virtual key to send

//mouseEvents -> MOUSE_UP | MOUSE_DOWN | MOUSE_CLICK
  //<value> -> virtual key to send

//mouseEvents -> MOUSE_MOVE
  //<value> -> <uint_16_t x> <uint_16_t y> (relative movement)


void setup() {  // initialize the buttons' inputs:

  strip.begin();
  strip.setBrightness(30);
  strip.show();

  Serial.begin(115200);
  // initialize mouse control:
  Mouse.begin();
  Keyboard.begin();
  USB.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    LogColor(255,0,0);
    delay(250);
    LogColor(0,0,0);
    delay(250);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Log("WiFi connected");
  LogColor(0,255,0);
  delay(250);
  LogColor(0,0,0);
  udp.begin(localUdpPort);

  MDNS.begin(serverName);


}

//ritorna il numero di byte letti
int readPacket(char incomingPacket[] )
{
  int packetSize = udp.parsePacket();
  //Log("PacketSize: %d",packetSize);
  if (packetSize) {
    int len = udp.read(incomingPacket, 254);
    if (len > 0) {
      incomingPacket[len] = 0;
      return len;
    }
  }
  return 0; 
}


void loop() {

  char incomingPacket[255];
  int packetSize = readPacket(incomingPacket);
  if (packetSize) {
    Log("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());

    Events eventType = (Events)incomingPacket[0];
    switch (eventType)
    {
    case Events::KEYBOARD:

      manageKeyboardEvent(incomingPacket);
      break;

    case Events::MOUSE :

      manageMouseEvent(incomingPacket);
      break;

    case Events::COPY :
      manageCopyEvent(incomingPacket);
      break;
    case Events::NO_EVENT :
      Log("No event\n");
    default:
    
      break;
    }
      
  }

}


void manageKeyboardEvent(char incomingPacket[])
{
  keyEvents eventType = (keyEvents)incomingPacket[1];
  uint8_t key = (uint8_t)incomingPacket[2];
  switch (eventType)
  {
  case keyEvents::KEY_UP:
    Keyboard.release(key);
    break;
  
  case keyEvents::KEY_DOWN:
    Keyboard.press(key);
    break;

  case keyEvents::KEY_CLICK:
    Keyboard.press(key);
    delay(20);
    Keyboard.release(key);
    break;

  default:
    break;
  }
}

void manageMouseEvent(char incomingPacket[])
{
  mouseEvents eventType = (mouseEvents)incomingPacket[1];
  uint8_t key = (uint8_t)incomingPacket[2];
  switch (eventType)
  {
  case mouseEvents::MOUSE_UP:
    Mouse.release(key);
    break;
  
  case mouseEvents::MOUSE_DOWN:
    Mouse.press(key);
    break;

  case mouseEvents::MOUSE_CLICK:
    Mouse.press(key);
    delay(20);
    Mouse.release(key);
    break;

   case mouseEvents::MOUSE_MOVE:
    {
      int16_t x = (int16_t)((incomingPacket[3] << 8) | incomingPacket[4]);
      int16_t y = (int16_t)((incomingPacket[5] << 8) | incomingPacket[6]);
      Mouse.move(x, y);
    }
    break;

  default:
    break;
  }
}

void manageCopyEvent(char incomingPacket[])
{
  //trova fino a /0 da incomingPacket
  size_t i;
  for (i = 1; i < 255; i++)
  {
    if (incomingPacket[i] == '\0')
    {
      break;
    } 
  }  
  String s = String((char*)&incomingPacket[1], i-1);
  Keyboard.print(s);
}
