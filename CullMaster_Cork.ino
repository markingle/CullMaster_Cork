/**
 *
 * HX711 library for Arduino - example file
 * https://github.com/bogde/HX711
 *
 * MIT License
 * (c) 2018 Bogdan Necula
 *
 * How to setup the TinyPICO in Arduino https://www.tinypico.com/gettingstarted
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
boolean minWeightNotify = false;
boolean timer_started = false;
int flag_temp = 0;

uint32_t value = 0;

#define LED 2

//CORK1 = RED
//CORK2 = GREEN
//CORK3 = BLACK
//CORK4 = YELLOW
//CORK5 = WHITE
//CORK6 = BLUE

// UID IN IPHONE APP
//let RED_Service_UUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c331914c")
//let GREEN_Service_UUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c331914d")
//let BLACK_Service_UUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c331914e")
//let YELLO_Service_UUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c331914f")
//let WHITE_Service_UUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c3319141")
//let BLUE_Service_UUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c3319142")

//CORK CHARACTERISTICS DEFINED IN IPHONE APP
//let RED_FlashRGB_Characteristic_CBUUID = CBUUID(string: "BEB5483E-36E1-4688-B7F6-EA07361B2611")
//let GREEN_FlashRGB_Characteristic_CBUUID = CBUUID(string: "BEB5483E-36E1-4688-B7F6-EA07361B2612")
//let BLACK_FlashRGB_Characteristic_CBUUID = CBUUID(string: "BEB5483E-36E1-4688-B7F6-EA07361B2613")
//let YELLO_FlashRGB_Characteristic_CBUUID = CBUUID(string: "BEB5483E-36E1-4688-B7F6-EA07361B2614")
//let WHITE_FlashRGB_Characteristic_CBUUID = CBUUID(string: "BEB5483E-36E1-4688-B7F6-EA07361B2615")
//let BLUE_FlashRGB_Characteristic_CBUUID = CBUUID(string: "BEB5483E-36E1-4688-B7F6-EA07361B2616")

#define CORK_SERVICE_UUID  "4fafc201-1fb5-459e-8fcc-c5c9c331914c"                // << SERVICE CHANGE HERE.....Weight_Scale_Service_CBUUID in CullMaster_V2 Xcode project
#define FISH_WEIGHT_CHARACTERISTIC_B_UUID "beb5483e-36e1-4688-b7f7-ea07361b26c8" //Fish_Weight_Characteristic_CBUUID
#define BATTERY_CHARACTERISTIC_C_UUID "beb5483e-36e1-4688-b7f8-ea07361b26d9"     // << BATTERY CHANGE HEREBattery_Characteristic_CBUUID
#define FLASHRGB_CHARACTERISTIC_D_UUID "BEB5483E-36E1-4688-B7F6-EA07361B2611"    // << FLASHING RGB CHANGE HERE......FlashRGB_Characteristic_CBUUID

#define PASSKEY 999999

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();

hw_timer_t * timer = NULL;

void IRAM_ATTR callRGB(){
  tp.DotStar_CycleColor(1);
        initBoot = false;
};

void flashRGB_On(){
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &callRGB, true);
  timerAlarmWrite(timer, 1000000, true);
  yield();
  timerAlarmEnable(timer);
  timer_started = true;
  Serial.println("Flash RGB On Called");
};

void flashRGB_Off(){
    timerAlarmDisable(timer);
    timerDetachInterrupt(timer);
    timerEnd(timer);
    timer = NULL;
    timer_started = false;
    Serial.println("FLash RGB Off Called");
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      Serial.println("Characteristic Callback1");
    };
};

class flashRGBCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("Flag Set to : ");
        
        char Str[] = {value[0], value[1], value[2], value[3], value[4]};
        
        Serial.println(atoi(Str));
        flag_temp = atoi(Str);
        Serial.println();
        Serial.println("*********");

        if (flag_temp == 1)
                  {
                    minWeightNotify = true;
                  }
         if (flag_temp == 0)
                  {
                    minWeightNotify = false;
                  }
      }
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

class SecurityCallback : public BLESecurityCallbacks {

  uint32_t onPassKeyRequest(){
    return 000000;
  }

  void onPassKeyNotify(uint32_t pass_key){}

  bool onConfirmPIN(uint32_t pass_key){
    vTaskDelay(5000);
    return true;
  }

  bool onSecurityRequest(){
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
    if(cmpl.success){
      Serial.println("   - SecurityCallback - Authentication Success");       
    }else{
      Serial.println("   - SecurityCallback - Authentication Failure*");
      pServer->removePeerDevice(pServer->getConnId(), true);
    }
    BLEDevice::startAdvertising();
  }
};

void bleSecurity(){
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
  esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;          
  uint8_t key_size = 16;     
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint32_t passkey = PASSKEY;
  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
};


void setup() {
  Serial.begin(115200);
  Serial.println("Cull Master RED");   //<<<<< CHANGE HERE FOR LOADING EACH CORK

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  

  // Create the BLE Device
  BLEDevice::init("RED");        //<<<<< CHANGE HERE FOR LOADING EACH CORK
  //BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  //BLEDevice::setSecurityCallbacks(new SecurityCallback());

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(CORK_SERVICE_UUID);


  // Create a BLE Characteristic C
  pCharacteristicC = pService->createCharacteristic(
                      BATTERY_CHARACTERISTIC_C_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE_NR |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
                    
// Create a BLE Characteristic D
  pCharacteristicD = pService->createCharacteristic(
                      FLASHRGB_CHARACTERISTIC_D_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE_NR  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristicC->setCallbacks(new MyCallbacks());
  pCharacteristicD->setCallbacks(new flashRGBCallback());

  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(CORK_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  Serial.println("Initializing RED");          //<<<<< CHANGE HERE FOR LOADING EACH CORK

  tp.DotStar_Clear();
  tp.DotStar_SetBrightness(255);
}

void loop() {
  
  //notify changed value
    if (deviceConnected) {
        
        delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
        // You can set the DotStar colour directly using r,g,b values
        if (minWeightNotify) {
          //tp.DotStar_CycleColor(1);
          tp.DotStar_SetPixelColor(0,0,255);
          delay(100);
          tp.DotStar_SetPixelColor(0,0,0);
          delay(100);
          tp.DotStar_SetPixelColor(0,0,255);
        } else {
          tp.DotStar_SetPixelColor(0,0,255);
        }
        initBoot = false;
        Serial.print("Battery Volage - ");
        Serial.println(tp.GetBatteryVoltage());
        Serial.println();
        
        if (tp.IsChargingBattery()) {
          Serial.println("Battery is Charging");
        } else {
          Serial.println("Battery is not charging");
        }
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
