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
    bool setIntData(const String& path, int value);
    bool setStringData(const String& path, const String& value);
private:
    FirebaseData streamData;
    static void firebaseStreamCallback(FirebaseStream data);
    static void firebaseStreamTimeoutCallback(bool timeout);
    bool _isAuthenticated{false};
    bool _isAuthorized{false};
    FirebaseData _data;
    FirebaseAuth _auth;
    FirebaseConfig _config;
};
#endif