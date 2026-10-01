#ifndef RTOS_TASK_H
#define RTOS_TASK_H

#define __USE_STAGING_CHANNEL__ false

#include "config.hpp"
#include "HttpServer.hpp"
#include "NetworkOperationManager.hpp"


namespace RTOSTask {

    void firebaseListenerTask(void* _) {
         unsigned long previousMillisClient = 0;
         unsigned long previousMillisFirebase = 0;
         const unsigned long CLIENT_INTERVAL = 1000;  // 1 sec . . .

         #if PROD_MODE
         const unsigned long FIREBASE_INTERVAL = 600 * 1000;  // 600 seconds (10 minutes) . . .
         #else
         const unsigned long FIREBASE_INTERVAL = 60 * 1000;  // 60 seconds (1 minute) . . .
         #endif

         for (;;) {
             unsigned long currentMillis = millis();
             if ((currentMillis - previousMillisFirebase) >= FIREBASE_INTERVAL) {
                 previousMillisFirebase = currentMillis;
                 if (WiFi.status() == WL_CONNECTED) {
                     if (HttpServer::firebase.isAuthenticated()) {
                         HttpServer::firebase.listenForAuthorizationStatus();
                        }
                    }
                }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}  // namespace RTOSTask

#endif
