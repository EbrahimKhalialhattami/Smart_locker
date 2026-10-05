#include <WString.h>
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"

#include "config.hpp"
#include "FirebaseOperations.hpp"
#include "NVSManager.hpp"
#include "Secret.hpp"


extern SemaphoreHandle_t otpMutex;
extern volatile int pendingAdminLockerId;
extern volatile unsigned long adminRfidTimeout;


void FirebaseOperations::authenticate() {
    LOGLN("Firebase is being started!");
    _config.api_key = WEP_API_KEY_SECRET;
    _config.database_url = REFERENCE_URL_SECRET;

#if LOGIN_ESP_ON_FIREBASE
    _auth.user.email = ESP_FIREBASE_AUTH_EMAIL;
    _auth.user.password = ESP_FIREBASE_AUTH_PWD;
#endif

#if REGISTER_ESP_ON_FIREBASE
    if (Firebase.signUp(&_config, &_auth, ESP_FIREBASE_AUTH_EMAIL, ESP_FIREBASE_AUTH_PWD)) {
        LOG("Device registered successfully!");
    } else {
        LOG(_config.signer.signupError.message.c_str());
    }
    _config.token_status_callback = tokenStatusCallback;
#endif

    Firebase.begin(&_config, &_auth);
    Firebase.reconnectWiFi(true);

#if LOGIN_ESP_ON_FIREBASE
    while (_auth.token.uid.empty()) {
        LOG('.');
        vTaskDelay(pdMS_TO_TICKS(50));
    }
#endif

    LOGLN("Logged In!");
    _isAuthenticated = true;
    startFirebaseStream();
}




void FirebaseOperations::startFirebaseStream() {
    Serial.println("Starting Firebase Stream for Lockers...");
    
    if (!Firebase.RTDB.beginStream(&streamData, "/lockers")) {
        Serial.println("Stream begin error: " + streamData.errorReason());
    }
    
    Firebase.RTDB.setStreamCallback(&streamData, firebaseStreamCallback, firebaseStreamTimeoutCallback);
}


void FirebaseOperations::firebaseStreamTimeoutCallback(bool timeout) {
    if (timeout) {
        Serial.println("Firebase stream timeout, attempting to resume...");
    }
}


void FirebaseOperations::listenForAuthorizationStatus() {
    Serial.println("Starting Firebase Stream for Authorization Status...");
    

    if (!Firebase.RTDB.beginStream(&authStreamData, "/device_status")) {
        Serial.println("Auth Stream begin error: " + authStreamData.errorReason());
    }
    
    Firebase.RTDB.setStreamCallback(&authStreamData, authStreamCallback, authStreamTimeoutCallback);
}


void FirebaseOperations::authStreamCallback(FirebaseStream data) {
    String path = data.dataPath();
    String dataType = data.dataType();
    
    bool newAuthStatus = false;
    bool statusChanged = false;


    if (dataType == "boolean" && path.indexOf("isAuthorized") != -1) {
        newAuthStatus = data.boolData();
        statusChanged = true;
    } 

    else if (dataType == "json") {
        FirebaseJson *json = data.jsonObjectPtr();
        FirebaseJsonData result;
        if (json->get(result, "isAuthorized")) {
            newAuthStatus = result.boolValue;
            statusChanged = true;
        }
    }

    if (statusChanged) {
        Serial.print("Device Authorization updated to: ");
        Serial.println(newAuthStatus ? "TRUE" : "FALSE");
        
       _isAuthorized = newAuthStatus;
    }
}


void FirebaseOperations::authStreamTimeoutCallback(bool timeout) {
    if (timeout) {
        Serial.println("Auth stream timeout, attempting to resume...");
    }
}


void FirebaseOperations::firebaseStreamCallback(FirebaseStream data) {
    String path = data.dataPath();
    String dataType = data.dataType();
    
    String lockerId = "";
    int firstSlash = path.indexOf('/', 0);
    if (firstSlash != -1) {
        int secondSlash = path.indexOf('/', firstSlash + 1);
        lockerId = (secondSlash != -1) ? path.substring(firstSlash + 1, secondSlash) : path.substring(firstSlash + 1);
    }

    String commandToExecute = "";
    String otpToUpdate = "";

    if (dataType == "string") {
        if (path.indexOf("/command") != -1) commandToExecute = data.stringData();
        else if (path.indexOf("/currentOtp") != -1) otpToUpdate = data.stringData();
    } 
    else if (dataType == "json") {
        FirebaseJson *json = data.jsonObjectPtr();
        FirebaseJsonData result;
        if (json->get(result, "command")) commandToExecute = result.stringValue;
        if (json->get(result, "currentOtp")) otpToUpdate = result.stringValue;
    }

    if (commandToExecute != "" && commandToExecute != "none") {
        if (commandToExecute == "prepare_rfid") {
            pendingAdminLockerId = lockerId.toInt();
            adminRfidTimeout = millis();
        } 
    else if (commandToExecute == "revoke_otp" && lockerId != "") {
        if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {
            activeOTPs.erase(lockerId);
            xSemaphoreGive(otpMutex);
            saveOTPsToNVS(); 
    }

        static FirebaseData tempFbdo;
        String basePath = "/lockers/" + lockerId;
    
        Firebase.RTDB.setString(&tempFbdo, basePath + "/command", "none");
    

        Firebase.RTDB.setString(&tempFbdo, basePath + "/status", "available"); 
        
    

        Firebase.RTDB.deleteNode(&tempFbdo, basePath + "/customerName");
        Firebase.RTDB.deleteNode(&tempFbdo, basePath + "/customerPhone");
        Firebase.RTDB.deleteNode(&tempFbdo, basePath + "/depositTimestamp");
        Firebase.RTDB.deleteNode(&tempFbdo, basePath + "/currentOtp");
}
    }

    if (otpToUpdate != "" && lockerId != "") {
        if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {
            activeOTPs[lockerId] = otpToUpdate; 
            xSemaphoreGive(otpMutex);

            saveOTPsToNVS();
        }
    }
}


