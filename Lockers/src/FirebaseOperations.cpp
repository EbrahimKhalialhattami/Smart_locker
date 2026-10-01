#include <WString.h>
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"

#include "config.hpp"
#include "FirebaseOperations.hpp"
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

void FirebaseOperations::listenForAuthorizationStatus() {
    Firebase.RTDB.getBool(&_data, RTDB_PATH);
    _isAuthorized = _data.boolData();
}

// -------------------------------------------------------------------------
// 1. دالة بدء الاستماع (تستدعى مرة واحدة بعد نجاح تسجيل الدخول)
// -------------------------------------------------------------------------
void FirebaseOperations::startFirebaseStream() {
    Serial.println("Starting Firebase Stream for Lockers...");
    
    // توجيه الاستماع إلى المسار الرئيسي "/lockers"
    if (!Firebase.RTDB.beginStream(&streamData, "/lockers")) {
        Serial.println("Stream begin error: " + streamData.errorReason());
    }
    
    // تعيين دوال الـ Callbacks التي ستعمل عند وصول بيانات جديدة
    Firebase.RTDB.setStreamCallback(&streamData, firebaseStreamCallback, firebaseStreamTimeoutCallback);
}

// -------------------------------------------------------------------------
// 2. دالة معالجة انقطاع الاتصال (Timeout)
// -------------------------------------------------------------------------
void FirebaseOperations::firebaseStreamTimeoutCallback(bool timeout) {
    if (timeout) {
        Serial.println("Firebase stream timeout, attempting to resume...");
    }
}

// -------------------------------------------------------------------------
// 3. الدالة الرئيسية التي تلتقط الأوامر وتنفذها فوراً
// -------------------------------------------------------------------------
void FirebaseOperations::firebaseStreamCallback(FirebaseStream data) {
    static FirebaseData tempFbdo;
    String path = data.dataPath();
    String dataType = data.dataType();
    
    // ---------------------------------------------------------
    // 0. استخراج رقم الخزانة من المسار بذكاء
    // (إذا كان المسار "/1/command" أو "/1" فإن lockerId سيكون "1")
    // ---------------------------------------------------------
    String lockerId = "";
    int firstSlash = path.indexOf('/', 0);
    if (firstSlash != -1) {
        int secondSlash = path.indexOf('/', firstSlash + 1);
        if (secondSlash != -1) {
            lockerId = path.substring(firstSlash + 1, secondSlash);
        } else {
            lockerId = path.substring(firstSlash + 1);
        }
    }

    // ---------------------------------------------------------
    // 1. معالجة البيانات إذا جاءت كنص مفرد (String)
    // ---------------------------------------------------------
    if (dataType == "string") {
        if (path.indexOf("/command") != -1) {
            String command = data.stringData();
            if (command != "none" && command != "") {
             
                
                if (command == "prepare_rfid") {
                    pendingAdminLockerId = lockerId.toInt();
                    adminRfidTimeout = millis();
                } 
                else if (command == "revoke_otp") {
                    if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {
                        activeOTPs.erase(lockerId);
                        xSemaphoreGive(otpMutex);
                    }
                    Firebase.RTDB.setString(&tempFbdo, "/lockers/" + lockerId + "/command", "none");
                    Firebase.RTDB.setString(&tempFbdo, "/lockers/" + lockerId + "/status", "available");
                }
            }
        }
        else if (path.indexOf("/currentOtp") != -1) {
            String newOtp = data.stringData();
            if (newOtp != "" && lockerId != "") {
                if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {
                    activeOTPs[lockerId] = newOtp; 
                    xSemaphoreGive(otpMutex);
                }
            }
        }
    }
    // ---------------------------------------------------------
    // 2. معالجة البيانات إذا جاءت كحزمة شاملة (JSON)
    // (هنا يكمن الحل لمشكلة التطبيق الذي يرسل التحديث دفعة واحدة)
    // ---------------------------------------------------------
    else if (dataType == "json") {
        FirebaseJson *json = data.jsonObjectPtr();
        FirebaseJsonData result;

        // البحث داخل الحزمة عن الرمز (currentOtp) واستخراجه
        if (json->get(result, "currentOtp")) {
            String newOtp = result.stringValue;
            if (newOtp != "" && lockerId != "") {
                // تخزين الرمز في الشريحة بأمان
                if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {
                    activeOTPs[lockerId] = newOtp; 
                    xSemaphoreGive(otpMutex);
                }
            }
        }

        // (كإجراء أمني إضافي) البحث داخل الحزمة عن أي أوامر مرسلة
        if (json->get(result, "command")) {
            String command = result.stringValue;
            if (command == "revoke_otp" && lockerId != "") {
                if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {
                    activeOTPs.erase(lockerId);
                    xSemaphoreGive(otpMutex);
                }
                Firebase.RTDB.setString(&tempFbdo, "/lockers/" + lockerId + "/command", "none");
                Firebase.RTDB.setString(&tempFbdo, "/lockers/" + lockerId + "/status", "available");
            }
        }
    }
}
bool FirebaseOperations::setIntData(const String& path, int value) {
    // تتطلب مكتبة Firebase_ESP_Client تمرير كائن الـ data والمسار والقيمة
    if (Firebase.RTDB.setInt(&_data, path.c_str(), value)) {
        return true;
    } else {
        LOG("Error updating Firebase: ");
        LOGLN(_data.errorReason());
        return false;
    }
}

bool FirebaseOperations::setStringData(const String& path, const String& value) {
    if (Firebase.RTDB.setString(&_data, path.c_str(), value.c_str())) {
        return true;
    } else {
        Serial.print("Error updating Firebase: ");
        Serial.println(_data.errorReason());
        return false;
    }
}
