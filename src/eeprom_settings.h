#pragma once

#include <EEPROM.h>
#include <string>

constexpr size_t EEPROM_SIZE = 512; 

constexpr size_t EEPROM_SIZE_SSID = 32;
constexpr size_t EEPROM_SIZE_PWD = 32;

constexpr size_t EEPROM_ADDR_SSID = 0;
constexpr size_t EEPROM_ADDR_PWD = EEPROM_ADDR_SSID + EEPROM_SIZE_SSID;

class EEPROM_Settings {

    public:
        EEPROM_Settings(){}

        void init(){
        }

        std::string getSSID(){

            std::string result = "";


            EEPROM.begin(EEPROM_SIZE);
            char r = char(EEPROM.read(EEPROM_ADDR_SSID));

            for(size_t i = 1; (i < EEPROM_SIZE_SSID ) && (r != '\0'); ++i){
                result += r;
                r = char(EEPROM.read(EEPROM_ADDR_SSID+i));
            }
            
            EEPROM.end();

            return result;
        }
        
        std::string getPassword(){

            std::string result = "";

            EEPROM.begin(EEPROM_SIZE);
            char r = char(EEPROM.read(EEPROM_ADDR_PWD));

            for(size_t i = 1; (i < EEPROM_SIZE_PWD ) && (r != '\0'); ++i){
                result += r;
                r = char(EEPROM.read(EEPROM_ADDR_PWD+i));
            }
            EEPROM.end();
            
            return result.c_str();
        }

        void setSSID(const char* ssid){

            std::string str(ssid);
            str += '\0';

            EEPROM.begin(512);
            for(size_t i = 0; i < 32; ++i){
                EEPROM.write(0+i, i < str.length() ? str[i] : '\0');
            }

            EEPROM.commit();
            EEPROM.end();
        }

        void setPassword(const char* pwd){

            std::string str(pwd);
            str += '\0';

            EEPROM.begin(EEPROM_SIZE);
            for(size_t i = 0; i < EEPROM_SIZE_PWD; ++i){
                EEPROM.write(EEPROM_ADDR_PWD+i, i < str.length() ? (uint8_t)str[i] : (uint8_t)'\0');
            }

            EEPROM.commit();
            EEPROM.end();
        }

};