
#include <Arduino.h>
#include <ESPmDNS.h> 
#include <ESPAsyncWebServer.h>
#include "HTML/_include_all.h"

#ifndef _MYSERVER_DEF
#define _MYSERVER_DEF


using myServerEventHandler = std::function<void(char incomingPacket[])>;



class MyServer
{
private:
    String MDNS_name;
    String webSocketEndpoint;
    uint16_t port;

    AsyncWebServer* server; //80
    AsyncWebSocket* ws; //"/ws"


    myServerEventHandler _eventHandler;

public:
    MyServer(uint16_t port,String webSocketEndpoint,String MDNS_name);
    ~MyServer();


    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void onEvent(myServerEventHandler eventHandler)
    {
        _eventHandler= eventHandler;
    }
    void loopCall(){
        ws->cleanupClients();
    }
    void init();

    void on(
        const char *uri, 
        WebRequestMethodComposite method, 
        ArRequestHandlerFunction onRequest);
};




MyServer::MyServer(uint16_t port,String webSocketEndpoint,String MDNS_name)
{
   this->MDNS_name=MDNS_name;
   this->webSocketEndpoint=webSocketEndpoint;
   this->port=port;
}

MyServer::~MyServer()
{
    server->end();
    delete server;
    delete ws;
}


void MyServer::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if(info->final && info->index == 0 && info->len == len) {
    if(len == 6) {
      char buffer[6];  // 6 byte fisso
      memcpy(buffer, data, 6);
      if( _eventHandler)
        _eventHandler(buffer);
    } 
    // Altri casi messaggi possono essere gestiti qui
  }
}


void MyServer::init(){
     //MDNS
    MDNS.begin(this->MDNS_name);

    server = new AsyncWebServer(this->port);
    ws = new AsyncWebSocket(this->webSocketEndpoint);

    //web server + websocket
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", PAGE_web);
    });


    ws->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if(type == WS_EVT_DATA) {
            handleWebSocketMessage(arg, data, len);
        }
    });
    server->addHandler(ws);
    server->begin();
}


void MyServer::on(
        const char *uri, 
        WebRequestMethodComposite method, 
        ArRequestHandlerFunction onRequest)
{
    this->server->on(uri,method,onRequest);
}


#endif