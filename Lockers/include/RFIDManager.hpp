#ifndef RFID_MANAGER_HPP
#define RFID_MANAGER_HPP

#include <SPI.h>
#include <MFRC522.h>
#include "config.hpp"

class RFIDReader final {
public:
    RFIDReader() = default;
void init() {
        SPI.begin();
        rfid.PCD_Init();
    }
String ScanCard(){
        if (!rfid.PICC_IsNewCardPresent()){
            return" ";
        }
        if (!rfid.PICC_ReadCardSerial()){

            return" ";
        }
        String id = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
    id += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    id += String(rfid.uid.uidByte[i], HEX);
  }
  id.toUpperCase();     
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
return id;
};

private:
        MFRC522 rfid{CS, RST};

};




     






#endif