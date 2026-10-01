#ifndef CONFIG
#define CONFIG

#include<Arduino.h>
#include <WString.h>
#include <array>
#include <cstdint>


#include "Secret.hpp"

struct Device {
    static inline constexpr const char* FIRMWARE_VERSION = "2.0.0";
    static inline constexpr const std::uint32_t BAUD_RATE = 115200;
};

//Opration Mode 
#define PROD_MODE false
#define DEV_MODE !(PROD_MODE)
#define ENABLE_LOGGING !(PROD_MODE)


//Logs Mode
#if ENABLE_LOGGING 
#define LOG(val) Serial.print(val) 
#define LOGLN(val) Serial.println(val)
#else
#define LOG(val)
#define LOGLN(val)
#endif

//Signing up the ESP on Firebase Cloud
#define LOGIN_ESP_ON_FIREBASE true
#define REGISTER_ESP_ON_FIREBASE !(LOGIN_ESP_ON_FIREBASE)

#define FIREBASE_WEB_API_KEY (WEP_API_KEY_SECRET)
#define FIREBASE_RTDB_REFERENCE_URL (REFERENCE_URL_SECRET)

#if DEV_MODE
#define ESP_FIREBASE_AUTH_EMAIL (AUTH_DEV_EMAIL_SECRET)
#define ESP_FIREBASE_AUTH_PWD (AUTH_DEV_PWD_SECRET)
#else
#define ESP_FIREBASE_AUTH_EMAIL (AUTH_PROD_EMAIL_SECRET)
#define ESP_FIREBASE_AUTH_PWD (AUTH_PROD_PWD_SECRET)
#endif

//WiFi Credentials
#define ESP_DEFAULT_SSID (AP_SSID_SECRET)
#define ESP_DEFAULT_PWD (AP_PWD_SECRET)

// Local Web Interface Authentication 
#define ESP_ADMIN_WEB_AUTH_USERNAME (LOGIN_USER_SECRET)
#define ESP_ADMIN_WEB_AUTH_PWD (LOGIN_PASSWORD_SECRET)


// for 4 lockers

#define GPIO32 32
#define GPIO33 33
#define GPIO25 25
#define GPIO26 26

const std::array<std::uint16_t,4> LOCKER_PINS = {GPIO32,
     GPIO33,
      GPIO25,
       GPIO26
    };
const std::uint16_t UNLOCK_DURATION_MS = 2000; //2s

#define Low_Locks_off LOW
#define High_Locks_on HIGH

//I2C Pins
#define SDA 21
#define SCL 22

//RFID Pins
#define MOSI 23
#define MISO 19
#define SCK 18
#define CS 5
#define RST 4

//I2C Keypad 
const uint8_t KEYPAD_I2C_ADDRESS = 0x20;
const byte ROW = 4;
const byte COL = 4;


/// HTTP Status Codes...
enum StatusCode {
    OK_CODE = 200,
    FOUND = 302,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    CONFLICT = 409,
    TOO_MANY_REQUESTS = 429,
    INTERNAL_SERVER_ERROR = 500,
    SERVICE_UNREACHABLE = 503,
};

// LittleFS File System Modes...
struct LittleFSFileMode {
    static inline constexpr const char* WRITE = "w";
    static inline constexpr const char* READ = "r";
    static inline constexpr const char* WRITE_READ = "w+";
    static inline constexpr const char* READ_WRITE = "r+";
};

// Configuration Files path...
struct CfgFilePath {
    static inline constexpr const char* LOGIN = "/login.cfg";
    static inline constexpr const char* HOME = "/home_wifi.cfg";
    static inline constexpr const char* DEVICE = "/device_wifi.cfg";
};

struct AuthKeys {
    static inline constexpr const char* ADMIN = ADMIN_API_KEY_SECRET;
};

struct RouteFilePath {
    static inline constexpr const char* INDEX = "/index.html";
    static inline constexpr const char* HOME_WIFI = "/home-wifi.html";
    static inline constexpr const char* CHANGE_PASSWORD = "/change-password.html";
    static inline constexpr const char* DEVICE_WIFI = "/device-wifi.html";
    static inline constexpr const char* RESTART = "/restart.html";
    static inline constexpr const char* OTA = "/ota.html";
    static inline constexpr const char* NOT_FOUND = "/404.html";
};

struct ContentType {
    static inline constexpr const char* PLAIN = "text/plain";
    static inline constexpr const char* HTML = "text/html";
};

struct ResponseMessage {
    static inline constexpr const char* EMPTY_BODY = "";
    static inline constexpr const char* INVALID_API_KEY = "Invalid API-Key.";
    static inline constexpr const char* TOKEN_QUERY_PARAM_REQUIRED = "token query param is required!";
    static inline constexpr const char* NOT_FOUND = "404! Not Found";
    static inline constexpr const char* TOO_MANY_REQUESTS = "Too many requests. Try again Later!";
    static inline constexpr const char* REBOOT = "Rebooted!";
    static inline constexpr const char* ERROR_OTA = "OTA Page Not Found!";
    static inline constexpr const char* FILED_OTA = "OTA FILED UPDATE";
    static inline constexpr const char* SUCCESS_OTA = "Update Success! Rebooting...";
};



//COMPANY_NAME
#define COMPANY_NAME ORGANIZATION_NAME_SECRET


#endif