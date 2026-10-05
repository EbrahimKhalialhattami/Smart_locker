#ifndef DISPLAY_MANAGER_I2C_HPP
#define DISPLAY_MANAGER_I2C_HPP
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "config.hpp"

class DisplayManager final {
public:
    DisplayManager() = default;

    void init() {
        lcd.init();
        lcd.backlight();
    }

    void displayMessage(const String& message,const String& message2) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(message);
        lcd.setCursor(0, 1);
        lcd.print(message2);
    }
    void displayStatus(const String& status) {
        lcd.setCursor(0, 1);
        lcd.print(status);
        lcd.setCursor(0, 1);
        lcd.print("   "); // Clear the rest of the line
    }
    void displayReadyMessage() {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Please Enter The");
        lcd.setCursor(0,1);
        lcd.print("Code...");
    }

    void displaySuccessMessage(const String& NumberOfLocker) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Locker: ");
        lcd.print(NumberOfLocker);
    }
    void displayErrorMessage(const String& error) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Error: ");
            lcd.setCursor(0,1);
            lcd.print(error);
    }
    void displayRFIDMessage() {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Please Swipe The");
            lcd.setCursor(0,1);
            lcd.print("Card...");
    }
    void updateCountdown(int seconds) {
    
        lcd.setCursor(8, 1); 
        
    
        if(seconds < 10) {
            lcd.print("0"); 
        }
        lcd.print(seconds);
        lcd.print("s ");
    }
    
        private:
            LiquidCrystal_I2C lcd{0x27, 16, 2};
};
#endif