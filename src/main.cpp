//LEGGIMI
//prima dell'upload occorre tenere premuto il tasto boot e premere reset
//l'esp si collegherà come COM e non tastiera/mouse






#include "variables.h"


#include "USB.h"
#include "USBHIDMouse.h"
#include "USBHIDKeyboard.h"
#include <UrlEncode.h>
#include <Preferences.h>

#include "myServer.h"
#include "Log.h"
#include "Wifi_mng.h"





Preferences preferences;


USBHIDMouse Mouse;
USBHIDKeyboard Keyboard;


MyServer server(webServerPort,webSocketEndpoint,MDNS_name);


Wifi_mng wifi;


void manageKeyboardEvent(char incomingPacket[]);
void manageMouseEvent(char incomingPacket[]);
void manageCopyEvent(char incomingPacket[]);
void managePacket(char incomingPacket[] );


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








void managePacket(char incomingPacket[] )
{

  out.str("%u %u %u %u %u %u",
    (uint8_t)incomingPacket[0],
    (uint8_t)incomingPacket[1],
    (uint8_t)incomingPacket[2],
    (uint8_t)incomingPacket[3],
    (uint8_t)incomingPacket[4],
    (uint8_t)incomingPacket[5]
  );
  

  //se c'è abilitata la seriale, non può funzionare in HID
  #if LOG_ON_SERIAL
    return;
  #endif

  

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
    out.str("No event\n");
  default:
  
    break;
  }
  return ; 
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
  
      //                HID     JS
      //MOUSE_LEFT      0x01    0
      //MOUSE_RIGHT     0x02    2
      //MOUSE_MIDDLE    0x04    1
      //MOUSE_BACKWARD  0x08    3
      //MOUSE_FORWARD   0x10    4
      //MOUSE_ALL       0x1F    
    

  const uint8_t jsTiHID[]={0x01,0x04,0x02,0x08,0x10};
  const int jsTiHIDSize = (sizeof(jsTiHID)/sizeof(jsTiHID[0]));
  for (int i = 0; i < jsTiHIDSize; i++)
  {
    out.str("%d",jsTiHID[i]);
  }
  

 
  mouseEvents eventType = (mouseEvents)incomingPacket[1];
  int16_t data = (int16_t)((incomingPacket[2] << 8) | incomingPacket[3]);

  switch (eventType)
  {
    
  case mouseEvents::MOUSE_UP:
   {
      if (!(data >= 0 && data < jsTiHIDSize)) {
        out.str("ERR: %d - Mouse Key not found!",data);
        break;
      }   
      uint8_t HIDMouseKey = jsTiHID[data];
      out.str("Mouse key released: %u",HIDMouseKey);
    
      Mouse.release(HIDMouseKey);
      break;
    }
  
  case mouseEvents::MOUSE_DOWN:
    {
      if (!(data >= 0 && data < jsTiHIDSize)) {
        out.str("ERR: %d - Mouse Key not found!",data);
        break;
      }   
      uint8_t HIDMouseKey = jsTiHID[data];
      out.str("Mouse key pressed: %u",HIDMouseKey);
    
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
      out.str("Mouse move: %d %d",x,y);
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






void setup() {  // initialize the buttons' inputs:

 
  out.init();

  
  //mouse/keyboard control:
  #if LOG_ON_SERIAL
    
  #else
    Mouse.begin();
    Keyboard.begin();
    USB.begin();
  #endif


  
  wifi.init();
  server.init();

  
  wifi.attachEndPoints(server);


  server.onEvent(managePacket);


  
}
void loop() {
  server.loopCall();
}