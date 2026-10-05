#include "NVSManager.hpp"
#include "config.hpp"
#include <Arduino.h>
#include <Preferences.h>
#include <map>


extern std::map<String, String> activeOTPs;
extern SemaphoreHandle_t otpMutex;

Preferences nvs;

void saveOTPsToNVS() {
    String serializedData = "";
    if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& pair : activeOTPs) {
            serializedData += pair.first + ":" + pair.second + ",";
        }
        xSemaphoreGive(otpMutex);
    }
    
    nvs.begin("lockers_db", false); 
    nvs.putString("otps", serializedData);
    nvs.end();
    LOGLN("OTPs saved to NVS.");
}

void loadOTPsFromNVS() {
    nvs.begin("lockers_db", true); 
    String data = nvs.getString("otps", "");
    nvs.end();
    
    if (data == "") {
        LOGLN("No saved OTPs found in NVS.");
        return; 
    }

    if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {
        activeOTPs.clear();
        int startIndex = 0;
        while (startIndex < data.length()) {
            int commaPos = data.indexOf(',', startIndex);
            if (commaPos == -1) break;
            
            String pair = data.substring(startIndex, commaPos);
            int colonPos = pair.indexOf(':');
            if (colonPos != -1) {
                String id = pair.substring(0, colonPos);
                String otp = pair.substring(colonPos + 1);
                activeOTPs[id] = otp; 
            }
            startIndex = commaPos + 1;
        }
        xSemaphoreGive(otpMutex);
    }
    LOGLN("OTPs successfully loaded from NVS!");
}
