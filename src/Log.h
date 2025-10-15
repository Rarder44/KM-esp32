#include "variables.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>



class Log
{
private:

    Adafruit_NeoPixel strip;
    bool stripFirstUse=true;

public:
    Log();
    ~Log();


    
    size_t str(const char *format, ...);
    void color(int R,int G, int B);

    void init();

};


#ifndef _LOG_DEF
#define _LOG_DEF




Log::Log()
{
    strip = Adafruit_NeoPixel(1, LED_BUILDIN, NEO_GRB + NEO_KHZ800);
    

    #if LOG_ON_SERIAL
        Serial.begin(115200);
    #endif
}

Log::~Log()
{
    
}


size_t Log::str(const char *format, ...)
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
void Log::color(int R,int G, int B)
{
    uint32_t color = strip.Color(R,G,B);
    strip.setPixelColor(0, color);
    strip.show();
}


void Log::init(){
    strip.begin();
    strip.setBrightness(30);
    strip.show();
}

#endif

Log out;
