#ifndef Relay_Manager_HPP
#define Relay_Manager_HPP

#include <Arduino.h>
#include "config.hpp"

class RelayManager final {
public:
    RelayManager() = default;

    void init() {
        for (uint16_t i = 0; i < 4; i++) {
            pinMode(LOCKER_PINS[i], OUTPUT);
            digitalWrite(LOCKER_PINS[i], Low_Locks_off);
        }
    }
    
    void openLocker(uint16_t lock_num) {
        int index = lock_num - 1;

       
        if (index < 0 || index >= 4) {
            Serial.println("Security/Error: Locker ID out of bounds!");
            return; 
        }

        digitalWrite(LOCKER_PINS[index], High_Locks_on);
        is_open[index] = true;
        open_times[index] = millis();
    }

    void update() {
        for (uint16_t i = 0; i < 4; i++) {
            if (is_open[i]) {
                unsigned long Time_Now = millis();
                unsigned long defrent_time = Time_Now - open_times[i];

                if (defrent_time >= UNLOCK_DURATION_MS) {
                    digitalWrite(LOCKER_PINS[i], Low_Locks_off);
                    is_open[i] = false;
                }
            }
        }
    }

private:
    bool is_open[4]{false, false, false, false};
    unsigned long open_times[4]{0, 0, 0, 0};
};

#endif