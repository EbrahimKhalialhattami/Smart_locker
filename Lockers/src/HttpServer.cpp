#define __USE_STAGING_CHANNEL__ false

#include <LittleFS.h>
#include <UUID.h>
#include <Update.h>
#include <cstdint>
#include <functional>


#include "AsyncCookieAuthMiddleware.hpp"
#include "config.hpp"
#include "CustomAsyncRateLimitMiddleware.hpp"
#include "FileSystem.hpp"
#include "FirebaseOperations.hpp"
#include "HttpServer.hpp"
#include "NetworkOperationManager.hpp"
#include "TaskCorePinController.hpp"


extern TaskCorePinController hardwareTaskController;
extern TaskCorePinController firebaseTaskController;


CfgFileSystem HttpServer::configFile;
AsyncWebServer HttpServer::server(80);
TaskCorePinController taskHandler;
FirebaseOperations HttpServer::firebase;
CookieIdGenerator cookie;
static NetworkOperationManager network;
static AsyncCookieAuthMiddleware authMiddleware(cookie);
static CustomAsyncRateLimitMiddleware rateLimitMiddleware;


void HttpServer::start() {

    server.begin();
 //   taskHandler.pinTaskToCpuCore(RTOSTask::firebaseListenerTask);

}

void HttpServer::setupRoutes() {
#if PROD_MODE
    rateLimitMiddleware.setMaxRequests(3);
#else
    rateLimitMiddleware.setMaxRequests(5);
#endif

    rateLimitMiddleware.setWindowSize(10);

    using Uri = const char*;
    using Method = WebRequestMethod;
    using RequestCallback = std::function<void(AsyncWebServerRequest*)>;
    using RouteDefinition = std::tuple<Uri, Method, RequestCallback, AsyncMiddleware*>;
    const std::uint16_t TOTAL_ROUTES = 12;

    const std::array<RouteDefinition, TOTAL_ROUTES> routes = {
        std::make_tuple("/", HTTP_GET, loginHandler_GET, &rateLimitMiddleware),
        std::make_tuple("/login", HTTP_POST, loginHandler_POST, &rateLimitMiddleware),
        std::make_tuple("/device-wifi", HTTP_GET, deviceWifiHandler_GET, &authMiddleware),
        std::make_tuple("/device-wifi", HTTP_POST, deviceWifiHandler_POST, &authMiddleware),
        std::make_tuple("/home-wifi", HTTP_GET, homeWifiHandler_GET, &authMiddleware),
        std::make_tuple("/home-wifi", HTTP_POST, homeWifiHandler_POST, &authMiddleware),
        std::make_tuple("/device-password", HTTP_GET, changePasswordHandler_GET, &authMiddleware),
        std::make_tuple("/device-password", HTTP_POST, changePasswordHandler_POST, &authMiddleware),
        std::make_tuple("/device/reboot", HTTP_GET, rebootDeviceHandler_GET, &authMiddleware),
        std::make_tuple("/api/health", HTTP_GET, healthEndpointHandler_GET, &rateLimitMiddleware),
        std::make_tuple("/api/device/reboot", HTTP_GET, rebootEndpointHandler_GET, &rateLimitMiddleware),
    };
    for (const auto& [uri, method, handler, middleware] : routes) {
        server.on(uri, method, handler).addMiddleware(const_cast<AsyncMiddleware*>(middleware));
    }
   

    server.on("/device/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!LittleFS.exists(RouteFilePath::OTA)) {
            request->send(StatusCode::NOT_FOUND, ContentType::PLAIN, ResponseMessage::ERROR_OTA);
            return;
        }
        request->send(LittleFS, RouteFilePath::OTA, ContentType::HTML);
    }).addMiddleware(&authMiddleware);

   server.on("/device/update", HTTP_POST, 
  
    [](AsyncWebServerRequest *request) {
        bool error = Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(StatusCode::OK_CODE, ContentType::PLAIN, error ? ResponseMessage::FILED_OTA : ResponseMessage::SUCCESS_OTA);
        response->addHeader("Connection", "close");
        request->send(response);
        
        delay(1000);
        if (!error) {
            ESP.restart(); 
        } else {
            LOGLN("OTA Failed! Resuming tasks...");
            hardwareTaskController.resumeTask();
            firebaseTaskController.resumeTask();
        }
    }, 

    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        
        if (!index) { 
            LOGLN("Starting OTA Update: " + filename);
            
            int updateCommand = U_FLASH;
            if (filename == "littlefs.bin" || filename == "spiffs.bin") {
                updateCommand = U_SPIFFS;
                LOGLN("Target: File System Update");
            } else {
                LOGLN("Target: Firmware Update");
            }
            
            hardwareTaskController.suspendTask();
            firebaseTaskController.suspendTask();

            size_t updateSize = request->contentLength();
    
            yield(); 

           if (!Update.begin(updateSize, updateCommand)) {
             Update.printError(Serial);
            }
        }

        if (!Update.hasError()) {
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }
        }

        if (final) { 
            if (Update.end(true)) {
                LOGLN("Update Success: " + String(index + len) + " Bytes");
            } else {
                Update.printError(Serial);
            }
        }
    }).addMiddleware(&authMiddleware);
 

        

    server.onNotFound(notFoundHandler_GET);


}


void HttpServer::loginHandler_GET(AsyncWebServerRequest* request) {
    servePage(request, RouteFilePath::INDEX);
}


void HttpServer::deviceWifiHandler_GET(AsyncWebServerRequest* request) {
    servePage(request, RouteFilePath::DEVICE_WIFI);
}

void HttpServer::homeWifiHandler_GET(AsyncWebServerRequest* request) {
    servePage(request, RouteFilePath::HOME_WIFI);
}

void HttpServer::changePasswordHandler_GET(AsyncWebServerRequest* request) {
    servePage(request, RouteFilePath::CHANGE_PASSWORD);
}

void HttpServer::notFoundHandler_GET(AsyncWebServerRequest* request) {
    servePage(request, RouteFilePath::NOT_FOUND);
}

void HttpServer::rebootDeviceHandler_GET(AsyncWebServerRequest* request) {
    servePage(request, RouteFilePath::RESTART);
    request->onDisconnect([]() -> void {
        WiFi.disconnect(true);
        ESP.restart();
    });
}

void HttpServer::servePage(AsyncWebServerRequest* request, const String& filename) {
    if (!LittleFS.exists(filename)) {
        LOGLN("Failed to open " + filename);
        request->send(StatusCode::NOT_FOUND, ContentType::PLAIN, ResponseMessage::NOT_FOUND);
        return;
    }

    request->send(LittleFS, filename, ContentType::HTML);
}

void HttpServer::loginHandler_POST(AsyncWebServerRequest* request) {
    String usernameArgument = request->arg("username");
    String passwordArgument = request->arg("password");

    usernameArgument.trim();
    passwordArgument.trim();

    String currentPassword = ESP_ADMIN_WEB_AUTH_PWD;
    CfgFormat loginPassword = configFile.read(CfgFilePath::LOGIN);

    CfgFormat::iterator password = loginPassword.find("password");
    if (password != loginPassword.end()) {
        auto& [_, customPassword] = *password;
        customPassword.trim();
        currentPassword = customPassword.isEmpty() ? currentPassword : customPassword;
    }

    if ((usernameArgument == ESP_ADMIN_WEB_AUTH_USERNAME) && (currentPassword == passwordArgument)) {
        AsyncWebServerResponse* response = request->beginResponse(StatusCode::FOUND, ContentType::PLAIN, ResponseMessage::EMPTY_BODY);
        cookie.generate();
        response->addHeader("Location", "/home-wifi");
        response->addHeader("Cache-Control", "no-cache");
        response->addHeader("Set-Cookie", "COOKIE_ID=" + cookie.getId() + "; Path=/; HttpOnly; Max-Age=600");  // 600 secs = 10mins
        request->send(response);
        return;
    }

    LOGLN("Login failed!");
    request->redirect("/");
}


void HttpServer::deviceWifiHandler_POST(AsyncWebServerRequest* request) {
    String deviceSSID = request->arg("device_ssid");
    String devicePassword = request->arg("device_pwd");

    deviceSSID.trim();
    devicePassword.trim();

    CfgFormat deviceNetworkConfig;
    deviceNetworkConfig.emplace("device_ssid", deviceSSID);
    deviceNetworkConfig.emplace("device_pwd", devicePassword);

    bool saved = configFile.write(CfgFilePath::DEVICE, deviceNetworkConfig);

    if (!saved) {
        LOGLN("Failed to save device wifi configuration");
        request->send(StatusCode::INTERNAL_SERVER_ERROR, ContentType::PLAIN, "Failed to save device wifi configuration");
        return;
    }

    LOGLN("Device Wifi configuration saved successfully");
    request->redirect("/device/reboot");
}

void HttpServer::homeWifiHandler_POST(AsyncWebServerRequest* request) {
    String homeSSID = request->arg("home_ssid");
    String homePassword = request->arg("home_pwd");

    homeSSID.trim();
    homePassword.trim();

    CfgFormat homeNetworkConfig;
    homeNetworkConfig.emplace("home_ssid", homeSSID);
    homeNetworkConfig.emplace("home_pwd", homePassword);

    bool saved = configFile.write(CfgFilePath::HOME, homeNetworkConfig);

    if (!saved) {
        LOGLN("Failed to save home wifi configuration");
        request->send(StatusCode::INTERNAL_SERVER_ERROR, ContentType::PLAIN, "Failed to save home wifi configuration");
        return;
    }

    LOGLN("Home Wifi configuration saved successfully");
    request->redirect("/device/reboot");
}

void HttpServer::changePasswordHandler_POST(AsyncWebServerRequest* request) {
    String password = request->arg("password");
    password.trim();

    CfgFormat loginCfg;
    loginCfg.emplace("password", password);

    bool saved = configFile.write(CfgFilePath::LOGIN, loginCfg);

    if (!saved) {
        LOGLN("Failed to save login password!");
        request->send(StatusCode::INTERNAL_SERVER_ERROR, ContentType::PLAIN, "Failed to change password!");
        return;
    }

    LOGLN("Login password changed saved successfully");
    request->redirect("/device/reboot");
}
void HttpServer::rebootEndpointHandler_GET(AsyncWebServerRequest* request) {
    if (!request->hasParam("token")) {
        request->send(StatusCode::BAD_REQUEST, ContentType::PLAIN, ResponseMessage::TOKEN_QUERY_PARAM_REQUIRED);
        return;
    }

    const String& token = request->getParam("token")->value();
    if (!token.equals(AuthKeys::ADMIN)) {
        request->send(StatusCode::FORBIDDEN, ContentType::PLAIN, ResponseMessage::INVALID_API_KEY);
        return;
    }

    request->send(StatusCode::OK_CODE, ContentType::PLAIN, ResponseMessage::REBOOT);
    request->onDisconnect([]() -> void {
        WiFi.disconnect(true);
        ESP.restart();
    });
}

void HttpServer::healthEndpointHandler_GET(AsyncWebServerRequest* request) {
    if (!request->hasParam("token")) {
        request->send(StatusCode::BAD_REQUEST, ContentType::PLAIN, ResponseMessage::TOKEN_QUERY_PARAM_REQUIRED);
        return;
    }


    const String& token = request->getParam("token")->value();
    if (!token.equals(AuthKeys::ADMIN)) {
        request->send(StatusCode::FORBIDDEN, ContentType::PLAIN, ResponseMessage::INVALID_API_KEY);
        return;
    }

    unsigned long uptimeSeconds = millis() / 1000;
    unsigned long uptimeMinutes = uptimeSeconds / 60;
    unsigned long uptimeHours = uptimeMinutes / 60;
    unsigned long uptimeDays = uptimeHours / 24;

    String log =
        "Build Version: " + String(Device::FIRMWARE_VERSION) + "\n" +
        "RSSI: " + String(WiFi.RSSI()) + "\n" +
        "Uptime: " + String(uptimeDays) + "d " + String(uptimeHours % 24) + "h " + String(uptimeMinutes % 60) + "m " + String(uptimeSeconds % 60) + "s\n" +
        "Wifi Status: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "\n" +
        "Cloud Status: " + String(HttpServer::firebase.isAuthenticated() ? "Connected" : "Disconnected") + "\n" +
        "Free Heap: " + String(ESP.getFreeHeap() / 1024) + " Kb - " + String(ESP.getFreeHeap()) + " Bytes\n" +
        "Connected Clients: " + String(WiFi.softAPgetStationNum()) + "\n" +

        "ORG Authorization: " + String(HttpServer::firebase.isOrganizationAuthorized() ? "Granted " : "Revoked");

    LOGLN(log);
    request->send(StatusCode::OK_CODE, ContentType::PLAIN, log);
}
