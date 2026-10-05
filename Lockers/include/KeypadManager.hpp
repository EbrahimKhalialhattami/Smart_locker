#ifndef KEYPAD_MANAGER_HPP
#define KEYPAD_MANAGER_HPP

#include <Keypad_I2C.h>
#include <Arduino.h>
#include "config.hpp"


class KeypadManager final {
public:
    KeypadManager() = default;

void init(){ 
    keypad.begin(makeKeymap(keys));
}
char getPressedKey(){
        char key = keypad.getKey();
        if (key != NO_KEY) {
            return key;
        }
        return NO_KEY;
    }
String getCurrentBuffer() {
    return inputBuffer;
}    
void clearBuffer() {
        inputBuffer = "";
    }
String getPasswordInput (){
            char key = getPressedKey();
            if (key!= NO_KEY) {
                
                if (key == '#') {
                    String tempBuffer = inputBuffer; // Store the current input buffer
                    inputBuffer = "";
                    return tempBuffer;
                }
                else if (key == '*') {
                    inputBuffer = ""; 
                    return "0/"; // Clear the input buffer
                } 
                else {
                    inputBuffer += key; // Store the key in the buffer
                    }
                   
                }
                 return "";
            }
private:
   byte ROW_bins[ROW] = {0, 1, 2, 3};
    byte COL_bins[COL] = {4, 5, 6, 7}; 

    char keys[ROW][COL] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };

    // 2. تعريف الكيباد ثانياً بعد أن أصبحت المصفوفات مليئة بالبيانات الصحيحة
    Keypad_I2C keypad{makeKeymap(keys), ROW_bins, COL_bins, ROW, COL, KEYPAD_I2C_ADDRESS};
    
    String inputBuffer = "";
};











#endif
