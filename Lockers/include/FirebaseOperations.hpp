#ifndef FIREBASE_OPERATIONS_H
#define FIREBASE_OPERATIONS_H

#include <Firebase_ESP_Client.h>
#include <map>
#include "FileSystem.hpp"
extern std::map<String, String> activeOTPs;

class FirebaseOperations final {
public:
    FirebaseOperations() = default;

    

    [[nodiscard]] bool isOrganizationAuthorized() const {
        return _isAuthorized;
    }

    [[nodiscard]] bool isAuthenticated() const {
        return _isAuthenticated;
    }

    void authenticate();
    void startFirebaseStream();
    void listenForAuthorizationStatus();
   template <typename T>
bool setData(const String& path, T value) {
    bool success = false;
    
    if constexpr (std::is_same_v<T, int>) {
        success = Firebase.RTDB.setInt(&_data, path.c_str(), value);
    } 
    else if constexpr (std::is_same_v<T, String>) {
        success = Firebase.RTDB.setString(&_data, path.c_str(), value.c_str());
    }

    else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
        success = Firebase.RTDB.setString(&_data, path.c_str(), value);
    }
    else if constexpr (std::is_same_v<T, bool>) {
        success = Firebase.RTDB.setBool(&_data, path.c_str(), value);
    }
    
    if (success) {
        return true;
    } else {
        LOG("Error updating Firebase: ");
        LOGLN(_data.errorReason());
        return false;
    }
}

bool deleteData(const String& path) {
    if (Firebase.RTDB.deleteNode(&_data, path.c_str())) {
        return true;
    } else {
        LOG("Error deleting Firebase node: ");
        LOGLN(_data.errorReason());
        return false;
    }
}
private:
    FirebaseData streamData;
    static void firebaseStreamCallback(FirebaseStream data);
    static void firebaseStreamTimeoutCallback(bool timeout);
    bool _isAuthenticated{false};
    inline static bool _isAuthorized{false};
    FirebaseData _data;
    FirebaseAuth _auth;
    FirebaseConfig _config;
   
    FirebaseData authStreamData; 
    
    static void authStreamCallback(FirebaseStream data);
    static void authStreamTimeoutCallback(bool timeout);
};
#endif