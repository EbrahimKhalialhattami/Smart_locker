#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <LittleFS.h>
#include <time.h>
#include <map>

#include "TaskCorePinController.hpp"
#include "DisplayManagerI2C.hpp"
#include "RFIDManager.hpp"
#include "KeypadManager.hpp"
#include "RelayManager.hpp"
#include "NetworkOperationManager.hpp"
#include "FirebaseOperations.hpp"
#include "HttpServer.hpp"
#include "config.hpp"


TaskCorePinController hardwareTaskController;
TaskCorePinController firebaseTaskController;
SemaphoreHandle_t otpMutex = NULL;
std::map<String, String> activeOTPs;

volatile bool isFirebaseConnected = false; 
bool lastFirebaseState = false;

volatile int pendingAdminLockerId = -1; 
volatile unsigned long adminRfidTimeout = 0;
volatile bool flagNotifyAppAdminOpened = false;
volatile int adminOpenedLockerId = -1;
volatile bool flagNotifyAppCustomerOpened = false;
volatile int customerOpenedLockerId = -1;


NetworkOperationManager networkManager;
FirebaseOperations firebase;

DisplayManager Screen;
RFIDReader RFID;
KeypadManager keypad;
RelayManager relay;


void taskNetworkAndFirebase(void *pvParameters);
void taskHardwareMonitor(void *pvParameters);

void setup() {
 otpMutex = xSemaphoreCreateMutex();

  Serial.begin(Device::BAUD_RATE);
  LOGLN(Device::FIRMWARE_VERSION);
  delay(1000);
  LOGLN("\n--- Starting System ---");

  
  Wire.begin(SDA,SCL);
  Screen.init();
  
  if (!LittleFS.begin(true))
  {
    LOGLN("Error: LittleFS mount failed!");
    Screen.displayMessage("Error:", "LittleFS mount failed!");
  }
  LOGLN("LittleFS mounted successfully!");

  relay.init();
  keypad.init();
  RFID.init();

  Screen.displayMessage("Smart Locker", "System Starting");
  
 
  
  networkManager.setupNetworks();
  HttpServer::setupRoutes();
  HttpServer::start();
  LOGLN(">>> Web Server is successfully running! <<<");

hardwareTaskController.setTaskStackSize(4096).setTaskPriorityLevel(2).pinTaskToCpuCore(taskHardwareMonitor, 0);

firebaseTaskController.setTaskStackSize(20480).setTaskPriorityLevel(1).pinTaskToCpuCore(taskNetworkAndFirebase, 1);
 
 LOGLN("FreeRTOS taking control...");

}

void loop() {

  
vTaskDelete(NULL);
}

void taskNetworkAndFirebase(void *pvParameters) {
  //while (!networkManager.isWiFiConnected()) {
    //LOGLN(".");
    //vTaskDelay(pdMS_TO_TICKS(1000));
  //}
  if (!networkManager.isWiFiConnected()) {
  LOGLN("Error: WiFi connection failed!");
    Screen.displayMessage("Smart Locker", "WiFi connection failed!");
  }
 else {
    LOGLN("Syncing time via NTP for SSL...");
    configTime(10800, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    
    while (now < 24 * 3600) {
      LOGLN(".");
      vTaskDelay(pdMS_TO_TICKS(500));
      now = time(nullptr);
    }
        
    LOGLN("");
    LOGLN("Time synchronized successfully!");
    firebase.authenticate();
    firebase.listenForAuthorizationStatus();
    Screen.displayReadyMessage();
  }
   
    
  
 for(;;){
  isFirebaseConnected = Firebase.ready();
  if (flagNotifyAppAdminOpened) {
    // إلغاء تفعيل العلم فوراً
    flagNotifyAppAdminOpened = false; 
    
    String commandPath = "/lockers/" + String(adminOpenedLockerId) + "/command";
    String statusPath = "/lockers/" + String(adminOpenedLockerId) + "/status"; 
    
    firebase.setStringData(commandPath, "none");
    
    if(firebase.setStringData(statusPath, "opened_by_admin")) {
         LOGLN("App notified via Network Task.");
    }
}
if (flagNotifyAppCustomerOpened) {
    flagNotifyAppCustomerOpened = false; // إطفاء العلم فوراً
    
    String lockerStrId = String(customerOpenedLockerId);
    
    // إرسال 3 تحديثات: إعادة الحالة إلى available، مسح الأمر، ومسح الرمز القديم
    firebase.setStringData("/lockers/" + lockerStrId + "/status", "available");
    firebase.setStringData("/lockers/" + lockerStrId + "/command", "none");
    firebase.setStringData("/lockers/" + lockerStrId + "/currentOtp", ""); 
    
    LOGLN("Customer opened locker " + lockerStrId + ". Firebase updated to 'available'.");
}
  vTaskDelay(pdMS_TO_TICKS(500));
 }
}


void taskHardwareMonitor(void *pvParameters) {
  static String lastBuffer = "";
  static unsigned long lastKeyPressTime = 0;
  unsigned long screenTimer = 0;
  bool isMessageShowing = false;
   for(;;){
  
     relay.update();
     if (isMessageShowing && (millis() - screenTimer >= 2000)) {
      isMessageShowing = false;         
      Screen.displayReadyMessage();     
    }
       if (isFirebaseConnected != lastFirebaseState) {
         lastFirebaseState = isFirebaseConnected; 
            
         if (!isFirebaseConnected) {
        
           Screen.displayErrorMessage("Firebase Offline NOW!");
           screenTimer = millis();
           isMessageShowing = true;
        } else {
      
         Screen.displayMessage("Firebase", "Online NOW!");
         screenTimer = millis();
         isMessageShowing = true;
       }
     }
    
    String inputOTP = keypad.getPasswordInput();
    String currentBuffer = keypad.getCurrentBuffer();
    
    if (currentBuffer != lastBuffer) {
      lastKeyPressTime = millis(); 
      
      if (inputOTP == "" ||inputOTP == "0/") {
        Screen.displayMessage("Enter OTP:", currentBuffer);
      }
      lastBuffer = currentBuffer;
    }

    
    if (currentBuffer.length() > 0 && (millis() - lastKeyPressTime >= 30000)) {
      keypad.clearBuffer();          
      lastBuffer = "";                
      
      Screen.displayErrorMessage("Timeout!"); 
      screenTimer = millis();                 
      isMessageShowing = true;                
    }


    if (inputOTP != "" && inputOTP != "0/") {


      bool isOtpValid = false; 
        
      
      if (xSemaphoreTake(otpMutex, portMAX_DELAY) == pdTRUE) {

        for (auto it = activeOTPs.begin(); it != activeOTPs.end(); ++it) {



            
          if (it->second == inputOTP) {


           String lockerId = it->first; 
                
           relay.openLocker(lockerId.toInt());
           
           activeOTPs.erase(it);
           isOtpValid = true;
           
           customerOpenedLockerId = lockerId.toInt();
           flagNotifyAppCustomerOpened = true;
           
           Screen.displaySuccessMessage(lockerId);
                      
           lastBuffer = "";
           
           screenTimer = millis();   
           isMessageShowing = true;  
           
           break; 
          }
        }
        xSemaphoreGive(otpMutex);
      }
      if (!isOtpValid) {

        Screen.displayErrorMessage("Invalid OTP");
        lastBuffer = "";
        screenTimer = millis();
        isMessageShowing = true;
      }
    
    }

   static int lastSecondsRemaining = -1; 

   
   if (pendingAdminLockerId != -1) {
      unsigned long elapsed = millis() - adminRfidTimeout;
            
      if (elapsed <= 30000) {
        
        int secondsRemaining = 30 - (elapsed / 1000);
                
        
        if (secondsRemaining != lastSecondsRemaining) {
          if (lastSecondsRemaining == -1) {
            Screen.displayRFIDMessage(); 
          }
          Screen.updateCountdown(secondsRemaining); 
          lastSecondsRemaining = secondsRemaining;
        }
      } else {
        
        Screen.displayErrorMessage("Time Expired!");
        pendingAdminLockerId = -1;   
        lastSecondsRemaining = -1;   
                
        screenTimer = millis();
        isMessageShowing = true;
      }
    } else {
      
      lastSecondsRemaining = -1; 
    }

   
   
   String cardID = RFID.ScanCard(); 
   
  if (cardID != " ") {
        
   
     if (cardID == "572E4402") {
       
       if (pendingAdminLockerId != -1 && (millis() - adminRfidTimeout <= 30000)) {
         Screen.displayMessage("Admin Access", "Locker " + String(pendingAdminLockerId) + " Open");
         relay.openLocker(pendingAdminLockerId); 
         
         adminOpenedLockerId = pendingAdminLockerId;
         flagNotifyAppAdminOpened = true;
         pendingAdminLockerId = -1;
         lastSecondsRemaining = -1;       

        } else {
          
          Screen.displayErrorMessage("Select Locker 1st");
        }       
      
        pendingAdminLockerId = -1; 
        lastSecondsRemaining = -1;

      } else {
        
        Screen.displayErrorMessage("Unknown Card");
      }
      screenTimer = millis();
      isMessageShowing = true;
    }
       if (isMessageShowing && (millis() - screenTimer >= 2000)) {

         isMessageShowing = false;         
         Screen.displayReadyMessage();     
    }
     vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
