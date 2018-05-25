//Arduino Nano + OLED (SSD 1306) + NFC Reader (PN532)
//Created by Fernando Har on 24/05/2018

//OLED Driver from: https://github.com/greiman/SSD1306Ascii
//PN532 Driver from: https://github.com/elechouse/PN532
//BLE based on BLE_server example Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
//
#include <Wire.h>

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C



#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <NdefMessage.h>

// Define proper RST_PIN if required.
#define RST_PIN -1

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

//Variables for OLED
SSD1306AsciiWire oled;

//Variables for NFC
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
unsigned long previousMillis = 0;
bool nfcDetected = false;

//Variables for BLE Server
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
BLEServer *pServer;
BLEService *pService; 
BLECharacteristic *pCharacteristic;
BLEAdvertising *pAdvertising;
void scanNFCHelper(){
  nfcDetected = nfc.tagPresent();
  if (nfcDetected){
    previousMillis = millis(); //reset timer so that the NFC content is displayed for 2 seconds
      NfcTag tag = nfc.read();
      oled.clear();
      oled.set2X();
      oled.println("NFC\nDetected");
       oled.set1X();
      oled.println("NFC content:");
      if(tag.hasNdefMessage()){
         NdefMessage ndefMessage = tag.getNdefMessage();
         int recordCount = ndefMessage.getRecordCount();
         if(recordCount >= 1){
            NdefRecord record = ndefMessage.getRecord(0);
            int payloadLength = record.getPayloadLength();
            byte payload[payloadLength];
            record.getPayload(payload);

            // The TNF and Type are used to determine how your application processes the payload
            // There's no generic processing for the payload, it's returned as a byte[]
            int startChar = 0;        
            if (record.getTnf() == TNF_WELL_KNOWN && record.getType() == "T") { // text message
              // skip the language code
              startChar = payload[0] + 1;
            } else if (record.getTnf() == TNF_WELL_KNOWN && record.getType() == "U") { // URI
              // skip the url prefix (future versions should decode)
              startChar = 1;
            }
                              
            // Force the data into a String (might fail for some content)
            // Real code should use smarter processing
            String payloadAsString = "";
            for (int c = startChar; c < payloadLength; c++) {
              payloadAsString += (char)payload[c];
            }
             oled.print(payloadAsString);
             pCharacteristic->setValue(payload, payloadLength);
         }
      }
  }else{
    pCharacteristic->setValue("");
  }
}
void scanNFC(){
  unsigned long currentMillis = millis();
  if(!nfcDetected && (currentMillis - previousMillis) >= 3000 ){ //No card detected, wait for 3 seconds and re scan
    oled.clear();
    oled.set2X();
    oled.print("Scanning ");
    previousMillis = currentMillis;
    scanNFCHelper();
  }else if ((currentMillis - previousMillis) >= 2000 ){
    oled.clear();
    oled.set2X();
    oled.print("Scanning ");
    previousMillis = currentMillis;
    scanNFCHelper();
  }
}
//------------------------------------------------------------------------------
void setup() {
  nfc.begin();
  Serial.begin(9600);
#if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0

  oled.setFont(Adafruit5x7);

  //uint32_t m = micros();
  oled.clear();
  oled.set2X();
  oled.println("Ble-NFC Reader"); 
  oled.set1X();
  oled.println("BLE Device: \nBleNFCReader");
  BLEDevice::init("BleNFCReader");
  pServer = BLEDevice::createServer();
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue("");
  pService->start();
  pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  
  delay(1000);
}
//------------------------------------------------------------------------------
void loop() {
 scanNFC();
}
