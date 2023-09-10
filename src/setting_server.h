#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <vector>

#include "eeprom_settings.h"
#include "utils.h"

struct WiFi_Network{
  String ssid;
  String bssid;
  int32_t rssi;
};

class SettingServer {

    public:
        SettingServer(const char* ssid, EEPROM_Settings& settings) : 
        settings(settings), ipAP(8, 8, 8, 8), netMsk(255, 255, 255, 0),
        ssidAP(ssid),
        askForRestart(false)
        {}

        void startServer(){
            askForRestart = false;

            WiFi.mode(WIFI_STA);
            WiFi.disconnect();

            WiFi.softAPConfig(ipAP, ipAP, netMsk);
            WiFi.softAP(ssidAP);

            webServer = new ESP8266WebServer();
            dnsServer = new DNSServer();

            dnsServer->start(53, "*", ipAP);

            webServer->on("/", std::bind(&SettingServer::handleRoot, this));
            webServer->on("/setSettings", std::bind(&SettingServer::handleSettings, this));
            webServer->on("/restart", std::bind(&SettingServer::handleRestart, this));
            webServer->onNotFound(std::bind(&SettingServer::handleNotFound, this));
            webServer->begin(); 
        }

        void handleClient(){
            dnsServer->processNextRequest();
            webServer->handleClient();
        }

    private:
        EEPROM_Settings& settings;
        ESP8266WebServer *webServer;
        DNSServer *dnsServer;
        
        IPAddress ipAP;
        IPAddress netMsk;

        const char* ssidAP;
        bool askForRestart;


        bool captivePortalRedirect(){
            if(WiFi.softAPIP().toString() != webServer->hostHeader()){
                webServer->sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
                webServer->send(302, "text/plain", "");
                webServer->client().stop();
                return true;
            }
            return false;
        }

        String generateMessage(){
            String result = "<div class=\"message #KIND#\">#MSG#</div>";

            if(webServer->hasArg("error")){
                result.replace("#KIND#", "error");
                result.replace("#MSG#", "Failed to save settings.<br/>Missing parameters");
            }
            else if(webServer->hasArg("success")){
                result.replace("#KIND#", "success");
                result.replace("#MSG#", "Settings saved.<br/>You can restart the alerter.");
            }
            else{
                result.replace("#KIND#", "none");
            }

            return result;
        }

        void handleRoot(){
            if(captivePortalRedirect()){return;}

        String page = "<!DOCTYPE html>"
            "<html>"
            "<head>"
                "<title>WiFi Setup</title>"
                "<style>"
                    "body{ font-family: 'Helvetica Neue', Helvetica, 'Arial Nova', Arial, sans-serif; }"
                    "#content { position: absolute; min-width: 400px; max-width: 90%; top: 50%; left: 50%; transform: translate(-50%, -50%); -webkit-box-shadow: 0px 0px 8px 2px #AAA; box-shadow: 0px 0px 8px 2px #AAA; border-radius: 5px; padding: 16px; box-sizing: border-box; }"
                    "h2 { margin-top: 0; text-align: center;}"
                    "table{ width: 80%; margin: auto; }"
                    "td{ vertical-align: middle; }"
                    "select{ width: 100%; padding: 4px;}"
                    "input[type=text]{ width: 100%; padding: 4px; box-sizing: border-box;}"
                    "input[type=submit] { width: 100%; font-variant: small-caps; font-size: 1em;}"
                    ".message{ width: 100%; padding: 4px; border-radius: 4px; }"
                    ".error{ background-color: #f5c6cb; }"
                    ".success{ background-color: #c3e6cb; }"
                    ".none { display: none; }"
                    ".restart{ width: 80%; background-color: #ffeeba; padding: 8px; border-radius: 5px; margin: auto; text-align: center; cursor: pointer; border: 1px solid black; } .restart:hover{ background-color: #eedda9; } .restart:active{ background-color: #ddcc98; }"
                "</style>"
            "</head>"
            "<body>"
                "<div id=\"content\">"
                "<h2>WiFi Setup</h2>"
                    + generateMessage() +
                    "<p>Connect the device to your WiFi</p>"
                    "<form method=\"post\" action=\"/setSettings\">"
                        "<table>"
                        "<tr><td><label for=\"ssid\"><b>SSID:</b></label></td><td><select id=\"ssid\" name=\"ssid\">" + vectorToHTMLOptions(getAvailableNetworks()) + "</select></td></tr>"
                        "<tr><td><label for=\"password\"><b>Password:</b></label></td><td><input type=\"text\" name=\"password\" id=\"password\"/></td></tr>"
                        "<tr><td colspan=\"2\"><br/><input type=\"submit\" value=\"Save Settings\"/></td></tr>"
                        "</table>"
                    "</form>"
                    "<div class=\"restart\" onclick=\"location.assign('/restart')\">Restart the device</div>"
                "</div>"
            "</body>"
            "</html>";
        

        webServer->setContentLength(page.length());
        webServer->send(200, "text/html", page);
        }

        void handleSettings(){
            if(captivePortalRedirect()){return;}
            
            String url = "http://" + WiFi.softAPIP().toString() + "/";

            if(webServer->hasArg("ssid") && webServer->hasArg("password")){
                settings.setSSID(webServer->arg("ssid").c_str());
                settings.setPassword(webServer->arg("password").c_str());

                webServer->sendHeader("Location", url + "?success=", true);
            }
            else{
                webServer->sendHeader("Location", url + "?error=", true);
            }

            webServer->send(302, "text/plain", "");
            webServer->client().stop();
        }

        void handleNotFound(){
            if(captivePortalRedirect()){return;}
            
        String page = 
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
                "<title>WiFi Setup</title>"
                "<style>"
                    "body{ font-family: 'Helvetica Neue', Helvetica, 'Arial Nova', Arial, sans-serif; }"
                    "#content { position: absolute; min-width: 400px; max-width: 90%; top: 50%; left: 50%; transform: translate(-50%, -50%); -webkit-box-shadow: 0px 0px 8px 2px #AAA; box-shadow: 0px 0px 8px 2px #AAA; border-radius: 5px; padding: 16px; box-sizing: border-box; }"
                    "h2 { margin-top: 0; text-align: center;}"
                "</style>"
            "</head>"
            "<body>"
                "<div id=\"content\">"
                "<h2>WiFi Setup</h2>"
                    "<p>404 - Page not found..."
                    "<p><a href=\"/\">Return to index</a></p>"
                "</div>"
            "</body>"
            "</html>";

            webServer->setContentLength(page.length());
            webServer->send(404, "text/html", page);
        }

        void handleRestart(){
            if(captivePortalRedirect()){return;}
            
            if(webServer->hasArg("valid")){
                if(askForRestart){
                    ESP.restart();
                }
                else{
                    webServer->sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
                }
            }
            else{
                askForRestart = true;
                webServer->sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/restart?valid=", true);
            }

            webServer->send(302, "text/plain", "");
            webServer->client().stop();
        }

        std::vector<WiFi_Network> getAvailableNetworks(){
            int8_t n = WiFi.scanNetworks(false, true);

            std::vector<WiFi_Network> networks(n);

            for( int8_t i = 0; i < n; i++){
                networks.push_back({ WiFi.SSID(i), WiFi.BSSIDstr(i), WiFi.RSSI(i)});
            }

            return networks;
        }

        String vectorToHTMLOptions(std::vector<WiFi_Network> arr){
            String result = "";
            
            for(size_t i = 0; i < arr.size(); ++i){
                result += "<option value=\"" + arr[i].ssid + "\">" + arr[i].ssid + "(" + arr[i].rssi + " dBm) - " + arr[i].bssid + "</option>";
            }

            return result;
        }
};