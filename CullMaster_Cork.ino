/**
 *
 * HX711 library for Arduino - example file
 * https://github.com/bogde/HX711
 *
 * MIT License
 * (c) 2018 Bogdan Necula
 *
**/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
//#include <BLE2902.h>
//#include <BLE2904.h>
#include <EEPROM.h>
#include <TinyPICO.h>


BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristicA = NULL;
BLECharacteristic* pCharacteristicB = NULL;
BLECharacteristic* pCharacteristicC = NULL;
BLECharacteristic* pCharacteristicD = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
bool initBoot = true;

uint32_t value = 0;

#define LED 2

#define CORK_SERVICE_UUID  "4fafc201-1fb5-459e-8fcc-c5c9c3319141"      //Weight_Scale_Service_CBUUID in CullMaster_V2 Xcode project
#define WEIGHT_CHARACTERISTIC_A_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"    //Weight_Switch_Characteristic_CBUUID in CullMaster_V2 Xcode project
#define BATTERY_CHARACTERISTIC_B_UUID "beb5483e-36e1-4688-b7f6-ea07361b26b9"    //Tare_Characteristic_CBUUID in CullMaster_V2 Xcode project
//#define CHARACTERISTIC_C_UUID "beb5483e-36e1-4688-b7f7-ea07361b26c8"    //Livewell_ONTIME_Characteristic_CBUUIDD
//#define CHARACTERISTIC_D_UUID "beb5483e-36e1-4688-b7f8-ea07361b26d9"    //Livewell_TIMER_Characteristic_CBUUID

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      Serial.println("Characteristic Callback1");
    };
};

class offTimeCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      //isscale.set_scale(732.f);                      // 852 this value is obtained by calibrating the scale with known weights; see the README for details
      ESP.restart();                // reset the scale to 0
      
    };
};



class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Cull Master Cork1");

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  

  // Create the BLE Device
  BLEDevice::init("CORK1");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(CORK_SERVICE_UUID);


  // Create a BLE Characteristic A
  pCharacteristicA = pService->createCharacteristic(
                      WEIGHT_CHARACTERISTIC_A_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE_NR |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
                    
// Create a BLE Characteristic B
  pCharacteristicB = pService->createCharacteristic(
                      BATTERY_CHARACTERISTIC_B_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE_NR  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristicA->setCallbacks(new MyCallbacks());
  pCharacteristicB->setCallbacks(new offTimeCallback());

  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(CORK_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  Serial.println("Initializing CORK");
}

void loop() {
  
  //notify changed value
    if (deviceConnected) {
        
        delay(100); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
        // You can set the DotStar colour directly using r,g,b values
    tp.DotStar_SetPixelColor(0,0,255);
    initBoot = false;
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(100); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
        tp.DotStar_SetPixelColor(255,0,0);
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        Serial.println(" Device is connecting");
        oldDeviceConnected = deviceConnected;
    }
    if (initBoot) {
      tp.DotStar_SetPixelColor(0,255,0);
    }
}
