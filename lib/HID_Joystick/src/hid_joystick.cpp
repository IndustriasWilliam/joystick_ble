#include "hid_joystick.h"


typedef struct __attribute__((__packed__)) {
  uint8_t x;
  uint8_t y;
  uint8_t z;
  uint8_t rz;
  uint8_t brake;
  uint8_t accel;
  uint8_t hat;
  uint16_t buttons;
} joystick_t;

static const char HID_SERVICE[] = "1812";
static const char HID_REPORT_MAP[] = "2A4B";
static const char HID_REPORT_DATA[] = "2A4D";
static const uint32_t scanTime = 0; /** 0 = scan forever */
static void notifyCB(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
static void scanEndedCB(BLEScanResults results);
static bool doConnect = false;
static BLEAdvertisedDevice advDevice;
static joystick_t Joystick_Report;
static bool Joystick_New_Data = false;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.isAdvertisingService(BLEUUID(HID_SERVICE))) {
      Serial.print("Advertised HID Device found: ");
      Serial.println(advertisedDevice.toString().c_str());

      BLEDevice::getScan()->stop();
      advDevice = advertisedDevice;
      doConnect = true;
    }
  }
};
    
class ClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    Serial.println("Connected");
    
  }

  void onDisconnect(BLEClient* pClient) {
    Serial.print(pClient->getPeerAddress().toString().c_str());
    Serial.println(" Disconnected - Starting scan");
    BLEDevice::getScan()->start(scanTime, scanEndedCB);
  }
};

ClientCallbacks clientCB;


bool connectToServer() {
    BLEClient* pClient = nullptr;

    if (!pClient) {
        pClient = BLEDevice::createClient();

        Serial.println("New client created");

        pClient->setClientCallbacks(&clientCB);
    }

    if (!pClient->isConnected()) {
        if (!pClient->connect(&advDevice)){

            Serial.printf("Failed to connect %s\r\n", advDevice.getName().c_str());
            return false;
        }
    }

    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());

  BLERemoteService* pSvc = nullptr;
  BLERemoteCharacteristic* pChr = nullptr;
  BLERemoteDescriptor* pDsc = nullptr;

  pSvc = pClient->getService(HID_SERVICE);
  if (pSvc) {     
    BLERemoteCharacteristic* report_data_chars;
    report_data_chars = pSvc->getCharacteristic(BLEUUID(HID_REPORT_DATA));

    if(report_data_chars->canNotify()){
        report_data_chars->registerForNotify(notifyCB);
    }

  }
  Serial.println("Done with this device!");
  return true;
}

class SecurityCallbacks: public BLESecurityCallbacks {
    uint32_t onPassKeyRequest(){
        Serial.println("on pass key request");
    }

    void onPassKeyNotify(uint32_t pass_key){
         Serial.printf("on passkey notify %i\r\n", pass_key);
    }

    bool onSecurityRequest(){
        Serial.println("security request");
        return false;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t){
        Serial.println("on atu compl");
    }

    bool onConfirmPIN(uint32_t pin){
        Serial.printf("on confirm pin %i\r\n", pin);
    }
};


void BLE_Client_Joystick::begin() {
    BLEDevice::init("");

    BLEDevice::setSecurityCallbacks(new SecurityCallbacks());
  
    BLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */

    BLEScan* pScan = BLEDevice::getScan();

    pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setActiveScan(true);
    pScan->start(scanTime, scanEndedCB);
}

void BLE_Client_Joystick::loop() {
  /** Loop here until we find a device we want to connect to */
  if (doConnect) {
    doConnect = false;

    /** Found a device we want to connect to, do it now */
    if (!connectToServer()) {
        BLEDevice::getScan()->start(scanTime, scanEndedCB);
    }
  }
  if (Joystick_New_Data) {
    static uint8_t last_x;
    static uint8_t last_y;
    movement_callback_t f = this->get_movement_callback();
    if (f) {
      if ((last_x != Joystick_Report.x) || (last_y != Joystick_Report.y)) {
        (*f)(Joystick_Report.x, Joystick_Report.y);
        last_x = Joystick_Report.x;
        last_y = Joystick_Report.y;
      }
    }
    static uint16_t last_buttons = 0;
    uint16_t changed = last_buttons ^ Joystick_Report.buttons;
    uint16_t buttons = Joystick_Report.buttons;
    for (size_t i = 0; i < 16; i++) {
      button_callback_t f = this->get_button_callback(i);
      if (f && (changed & 1)) {
        (*f)(buttons & 1);
      }
      changed >>= 1;
      buttons >>= 1;
    }
    last_buttons = Joystick_Report.buttons;
    Joystick_New_Data = false;
  }
}

/** Notification / Indication receiving handler callback */
// WARNING: This device has 4 Characteristics = 0x2a4d but with different
// handle values.
static void notifyCB(BLERemoteCharacteristic* pRemoteCharacteristic,
    uint8_t* pData, size_t length, bool isNotify) {
        Serial.println("received");
  if (pRemoteCharacteristic->getHandle() == 56) {
    memcpy(&Joystick_Report, pData, sizeof(Joystick_Report));
    Joystick_New_Data = true;
  }
}

/** Callback to process the results of the last scan or restart it */
static void scanEndedCB(BLEScanResults results) {
  Serial.println("Scan Ended");
}