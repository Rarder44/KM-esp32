#include <WiFi.h>
#include "Log.h"
#include "MyServer.h"
#include "Preferences.h"
#include "variables.h"

enum Wifi_type{
    AP,
    CONNECTED
};

class Wifi_mng
{
private:
    String networksList="";
    Preferences preferences;
public:
    Wifi_mng(/* args */);
    ~Wifi_mng();

    Wifi_type init();
    void attachEndPoints(MyServer& server);
};


#ifndef _WIFI_MNG_DEF
#define _WIFI_MNG_DEF

Wifi_mng::Wifi_mng(/* args */)
{
}

Wifi_mng::~Wifi_mng()
{
}


Wifi_type Wifi_mng::init(){
    //scansiono le reti esistenti
    
    if(networksList=="" )
    {
        out.color(255,255,0);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
        int n = WiFi.scanNetworks();
        out.str("num reti wifi: %d",n);
        networksList = "";
        for (int i = 0; i < n; ++i) {
            networksList += WiFi.SSID(i)+"\n";
        }
        WiFi.scanDelete();
        out.color(0,0,0);
    }

    


    //se non vengono trovati, vengono usati i valori di default ( vedi wifi_credential.h )
    preferences.begin("wifiCreds", false);
    String P_ssid = preferences.getString("ssid", SSID_DEFAULT);
    String P_password = preferences.getString("pwd", PSW_DEFAULT);
    preferences.end();

    unsigned long wifiStart = millis();
    bool wifiConnected= false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(P_ssid, P_password);
    while (millis() - wifiStart < wifi_timeOut) {
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            break;
        }
        out.color(255,0,0);
        delay(250);
        out.color(0,0,0);
        delay(250);
    }


    if (wifiConnected) {
        out.str("WiFi connected");
        out.color(0,255,0);
        delay(250);
        out.color(0,0,0);
        return Wifi_type::CONNECTED;
    } else {


        out.str("WiFi timeout, starting AP");
        out.color(0,0,255);
        delay(250);
        out.color(0,0,0);

        WiFi.disconnect(true);  // Disconnette e resetta la configurazione wifi
        delay(100);

        // Start an open Access Point (change SSID/password if needed)
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        if( AP_password=="")
            WiFi.softAP(AP_ssid);
        else
            WiFi.softAP(AP_ssid,AP_password);


        IPAddress apIP = WiFi.softAPIP();
        out.str("AP started: %s IP: %s", AP_ssid, apIP.toString().c_str());

        return Wifi_type::AP;
    }
}
void Wifi_mng::attachEndPoints(MyServer& server)
{
    //WS - set wifi
    server.on("/setWIFIcred", HTTP_GET, [this](AsyncWebServerRequest *request){
        String ssid, pwd;


        if (request->hasParam("SSID") && request->hasParam("PWD")) {
            ssid = request->getParam("SSID")->value();
            pwd = request->getParam("PWD")->value();

            out.str("ricevuto %s - %s",ssid,pwd);


            // Apertura namespace Preferences
            preferences.begin("wifiCreds", false);

            // Salvataggio valori
            preferences.putString("ssid", ssid);
            preferences.putString("pwd", pwd);

            preferences.end();

            request->send(200, "text/html", "OK");
            ESP.restart();
        } 
        else {
            request->send(400, "text/html", "Parametri mancanti SSID o PWD");
        }
    });


    //WS - get wifi list
    server.on("/getWIFI", HTTP_GET, [this](AsyncWebServerRequest *request){
        // scansione bloccante (può impiegare qualche centinaio di ms)
        request->send(200, "text/html", networksList);        
    });


}

#endif