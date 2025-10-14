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
#include <ESPAsyncWebServer.h>

#define LOGGING_ENABLED 1
//#define LOG_ON_WEBSOCKET 1
//#define LOG_ON_SERIAL 1
#define LED_BUILDIN 48

#include "wifi_credential.h"
#include "HTML/_include_all.h"

//the file "wifi_credential.h" must contains these 2 variables
//const char* ssid = "ssid";         
//const char* password = "password"; 



//const unsigned int localUdpPort = 4210; // Porta UDP di ascolto
const String serverName = "KVESP32"; // Nome del server per mDNS


Adafruit_NeoPixel strip(1, LED_BUILDIN, NEO_GRB + NEO_KHZ800);



USBHIDMouse Mouse;
USBHIDKeyboard Keyboard;


//WiFiUDP udp;


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


void manageKeyboardEvent(char incomingPacket[]);
void manageMouseEvent(char incomingPacket[]);
void manageCopyEvent(char incomingPacket[]);
void managePacket(char incomingPacket[] );



// Log function using Serial.printf
size_t Log(const char *format, ...)
{
 
  #if LOGGING_ENABLED

  //alloco un buffer e tramite vsnprintf formatto la stringa
    char loc_buf[128];
    char *temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);

    //Se len < 0 errore di formattazione, si chiude arg e si ritorna 0
    //Se len >= sizeof(loc_buf), significa che loc_buf non basta ad ospitare la stringa.

    if (len < 0) {
        va_end(arg);
        return 0;
    }
    if (len >= (int)sizeof(loc_buf)) {
      //quindi alloco un buffer più grande con la malloc ed uso quello
        temp = (char *)malloc(len + 1);
        if (temp == NULL) {
            va_end(arg);
            return 0;
        }
        len = vsnprintf(temp, len + 1, format, arg);
    }
    va_end(arg);


     

    #if LOG_ON_WEB  //NON USARE!! CRASHAAAA
        // Costruisci la URL
        String msg = urlEncode(String(temp));

        HTTPClient http;
        http.begin("http://192.168.1.66:5000/LOG");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String payload = "msg=" + msg;
        int httpCode = http.POST(payload);
        http.end();
    #endif

    #if LOG_ON_WEBSOCKET
        ws.textAll(temp);
    #endif

    #if LOG_ON_SERIAL
        len = Serial.write((uint8_t*)temp, len);
        Serial.write('\n');
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
  KEY_PRESS=2,
};

enum mouseEvents {
  MOUSE_UP=0,
  MOUSE_DOWN=1,
  MOUSE_CLICK=2,
  MOUSE_MOVE=3,
  MOUSE_WHEEL=4
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


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if(info->final && info->index == 0 && info->len == len) {
    if(len == 6) {
      char buffer[6];  // 6 byte + terminatore stringa
      memcpy(buffer, data, 6);
      managePacket(buffer);
    } 
    // Altri casi messaggi possono essere gestiti qui
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}


void setup() {  // initialize the buttons' inputs:

  //Led RGB ( status ) 
  strip.begin();
  strip.setBrightness(30);
  strip.show();


  //mouse/keyboard control:
 

  #if LOG_ON_SERIAL
    Serial.begin(115200);
  #else
    Mouse.begin();
    Keyboard.begin();
    USB.begin();
  #endif


  
  //Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    LogColor(255,0,0);
    delay(250);
    LogColor(0,0,0);
    delay(250);
  }
  Log("WiFi connected");
  LogColor(0,255,0);
  delay(250);
  LogColor(0,0,0);

  //MDNS
  MDNS.begin(serverName);


  //web server + websocket
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", PAGE_web);
  });
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();

}

//ritorna il numero di byte letti
/*int readPacket(char incomingPacket[] )
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
}*/

void managePacket(char incomingPacket[] )
{

  Log("%u %u %u %u %u %u",
    (uint8_t)incomingPacket[0],
    (uint8_t)incomingPacket[1],
    (uint8_t)incomingPacket[2],
    (uint8_t)incomingPacket[3],
    (uint8_t)incomingPacket[4],
    (uint8_t)incomingPacket[5]
  );
  

  //se c'è abilitata la seriale, non può funzionare in HID
  //#if LOG_ON_SERIAL
  //  return;
  //#endif

  

  Events eventType = (Events)incomingPacket[0];
  switch (eventType)
  {
  case Events::KEYBOARD:

    //manageKeyboardEvent(incomingPacket);
    break;

  case Events::MOUSE :
    manageMouseEvent(incomingPacket);
    break;

  case Events::COPY :
    //manageCopyEvent(incomingPacket);
    break;
  case Events::NO_EVENT :
    Log("No event\n");
  default:
  
    break;
  }
  return ; 
}


void loop() {
  ws.cleanupClients();
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

  case keyEvents::KEY_PRESS:
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
  /*
                      HID     JS
      MOUSE_LEFT      0x01    0
      MOUSE_RIGHT     0x02    2
      MOUSE_MIDDLE    0x04    1
      MOUSE_BACKWARD  0x08    3
      MOUSE_FORWARD   0x10    4
      MOUSE_ALL       0x1F    
    */

  const uint8_t jsTiHID[]={0x01,0x04,0x02,0x08,0x10};
  const int jsTiHIDSize = (sizeof(jsTiHID)/sizeof(jsTiHID[0]));
  for (int i = 0; i < jsTiHIDSize; i++)
  {
    Log("%d",jsTiHID[i]);
  }
  

 
  mouseEvents eventType = (mouseEvents)incomingPacket[1];
  int16_t data = (int16_t)((incomingPacket[2] << 8) | incomingPacket[3]);

  switch (eventType)
  {
    
  case mouseEvents::MOUSE_UP:
   {
      if (!(data >= 0 && data < jsTiHIDSize)) {
        Log("ERR: %d - Mouse Key not found!",data);
        break;
      }   
      uint8_t HIDMouseKey = jsTiHID[data];
      Log("Mouse key released: %u",HIDMouseKey);
    
      Mouse.release(HIDMouseKey);
      break;
    }
  
  case mouseEvents::MOUSE_DOWN:
    {
      if (!(data >= 0 && data < jsTiHIDSize)) {
        Log("ERR: %d - Mouse Key not found!",data);
        break;
      }   
      uint8_t HIDMouseKey = jsTiHID[data];
      Log("Mouse key pressed: %u",HIDMouseKey);
    
      Mouse.press(HIDMouseKey);
      break;
    }
  case mouseEvents::MOUSE_CLICK:
    //Already managed from up and down
    //Mouse.press(data);
    //delay(20);
    //Mouse.release(data);
    break;

   case mouseEvents::MOUSE_MOVE:
    {
      int16_t x = data;
      int16_t y = (int16_t)((incomingPacket[4] << 8) | incomingPacket[5]);
      Log("Mouse move: %d %d",x,y);
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
