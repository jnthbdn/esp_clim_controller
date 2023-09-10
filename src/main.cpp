#include <Arduino.h>
#include <cmath>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>

#include "eeprom_settings.h"
#include "setting_server.h"
#include "utils.h"
#include "panasonic_remote.h"

constexpr byte led_ir = D7;
constexpr byte led_r  = D2;
constexpr byte led_g  = D3;
constexpr byte led_b  = D4;


bool isAP = false;
ESP8266WebServer webServer(80);
EEPROM_Settings settings;
SettingServer settingServer("IR Remote", settings);
PanasonicRemote remote(led_ir);

bool isOn = false;
uint8_t temperature = 16;
bool temp_is_half = false;
StreamMode stream_mode = StreamMode::AUTO;

void set_red(bool enable){ digitalWrite(led_r, enable ? LOW : HIGH); }
void set_green(bool enable){ digitalWrite(led_g, enable ? LOW : HIGH); }
void set_blue(bool enable){ digitalWrite(led_b, enable ? LOW : HIGH); } 

void configure_webserver(){

    webServer.on("/temperature", [](){
        float t = temperature;

        if(temp_is_half) t += 0.5;

        webServer.send(200, "text/plain", String(t, 1));
    });

    webServer.on(UriRegex("^\\/temperature/([0-9]+.?[0-9]*)?"), [&](){
        float new_t = webServer.pathArg(0).toFloat();

        uint8_t int_part = uint8_t(new_t);
        bool is_half = (new_t - int_part) > 0;

        if( int_part > 30 || ( int_part == 30 && is_half) ){
            webServer.send(400, "text/plain", "Bad temperature value");
            return;
        }

        temperature = int_part;
        temp_is_half = is_half;

        webServer.send(200, "text/plain", "");
    });

    webServer.on("/stream_mode", [](){
        String mode = "";
        switch(stream_mode){
            case AUTO:
                mode = "AUTO";
                break;

            case QUIET:
                mode = "QUIET";
                break;

            case POWERFULL:
                mode = "POWERFULL";
                break;
        }

        webServer.send(200, "text/plain", mode);
    });

    webServer.on(UriRegex("^\\/stream_mode/(AUTO|POWERFULL|QUIET)"), [&](){
        String new_m = webServer.pathArg(0);
        
        if      ( new_m == "AUTO" )      stream_mode = StreamMode::AUTO;
        else if ( new_m == "POWERFULL" ) stream_mode = StreamMode::POWERFULL;
        else if ( new_m == "QUIET" )     stream_mode = StreamMode::QUIET;

        webServer.send(200, "text/plain", "");
    });
    
    webServer.on("/on_off", [](){
        webServer.send(200, "text/plain", isOn ? "ON" : "OFF");
    });

    webServer.on(UriRegex("^\\/on_off/(ON|OFF)"), [&](){
        String new_s = webServer.pathArg(0);

        if      ( new_s == "ON" ) isOn = true;
        else if (new_s == "OFF" ) isOn = false;

        webServer.send(200, "text/plain", "");
    });

    webServer.on("/send", [&](){
        remote
            .setStreamMode(stream_mode)
            .setTemperature(temperature, temp_is_half);
            
        if(isOn){
            remote.turnOn();
        }
        else{
            remote.turnOff();
        }

        remote.send();
        webServer.send(200, "text/plain", "Send...");
    });

  webServer.begin();
  Serial.println("HTTP server started");
}

void ap_loop(){
  settingServer.handleClient();
}

void main_loop(){
    webServer.handleClient();
}

void setup(){
    pinMode(led_r, OUTPUT);
    pinMode(led_g, OUTPUT);
    pinMode(led_b, OUTPUT);

    set_red(false);
    set_green(false);
    set_blue(true);

    remote.init();
    Serial.begin(115200);

    Serial.println("Wifi connection...");

    if( tryConnectWiFi(settings.getSSID().c_str(), settings.getPassword().c_str()) ){
        Serial.println("Connection success");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        isAP = false;

        configure_webserver();
        set_blue(false);
        set_green(true);
        delay(1000);
        set_green(false);
    }
    else{
        Serial.println("Connection fail !");
        Serial.println("Start AP Mode");
        isAP = true;
        settingServer.startServer();
        set_blue(false);
        set_red(true);
    }
}

void loop(){
  if(isAP){
    ap_loop();
  }
  else{
    main_loop();
  }
}