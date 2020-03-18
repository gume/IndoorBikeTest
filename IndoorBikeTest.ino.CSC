#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

//https://os.mbed.com/users/tenfoot/code/BLE_CycleSpeedCadence/file/7e334e81da21/CyclingSpeedAndCadenceService.h/

bool BleIF_deviceConnected;
bool BleIF_pDeviceConnected;

class GattService {
public:
  enum {
     UUID_CYCLING_SPEED_AND_CADENCE = 0x1816
  };
};

class GattCharacteristic {
public:
  enum {
    UUID_CSC_MEASUREMENT_CHAR = 0x2A5B,
    UUID_CSC_FEATURE_CHAR = 0x2A5C,
    UUID_SENSOR_LOCATION_CHAR = 0x2A5D,
    UUID_SC_CONTROL_POINT_CHAR = 0x2A55
  };
};

class BleIF_ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    BleIF_deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    BleIF_deviceConnected = false;
  }
};

class BleIF_CCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    for (int i = 0; i < value.length(); i++) Serial.print(value[i], HEX);
  }
};

BLECharacteristic *pCsc, *pCscFeat, *pScLocation, *pControlPoint;

uint32_t c = 0;

void setup() {
  Serial.begin(115200);
  
  BLEDevice::init("Testbike");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BleIF_ServerCallbacks());

  BLEService *pService = pServer->createService((uint16_t)GattService::UUID_CYCLING_SPEED_AND_CADENCE);
  pCsc = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_CSC_MEASUREMENT_CHAR,
             BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCsc->addDescriptor(new BLE2902());
  pCscFeat = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_CSC_FEATURE_CHAR,
             BLECharacteristic::PROPERTY_READ);
  uint16_t vf = 2;  // Crank counter only
  pCscFeat->setValue((uint8_t*)&vf, 2);
  pScLocation = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_SENSOR_LOCATION_CHAR,
             BLECharacteristic::PROPERTY_READ);
  uint8_t vl = 6;  // LOCATION_RIGHT_CRANK
  pScLocation->setValue((uint8_t*)&vl, 1);
  pControlPoint = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_SC_CONTROL_POINT_CHAR,
             BLECharacteristic::PROPERTY_WRITE);
  pControlPoint->setCallbacks(new BleIF_CCallbacks());
  pService->start();               

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID((uint16_t)GattService::UUID_CYCLING_SPEED_AND_CADENCE);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}

void BleIF_update(uint16_t revs) {

  uint8_t v[5];
  uint16_t revsx = (uint32_t)revs * (uint32_t)1024 / 1000;
  uint16_t now16 = (uint16_t) millis();
  v[0] = 2; // Crank only
  v[1] = revsx % 256;
  v[2] = revsx / 256;
  v[3] = now16 % 256;
  v[4] = now16 / 256;
  
  pCsc->setValue(v, 5);
  pCsc->notify();
}

void loop() {
  Serial.println(c);
  int i;
  for (i=0; i<5; i++) {
    BleIF_update(c++);
    delay(60 * 1000 / 50);
  }
  for (i=0; i<5; i++) {
    BleIF_update(c++);
    delay(60 * 1000 / 100);
  }
  for (i=0; i<5; i++) {
    BleIF_update(c++);
    delay(60 * 1000 / 75);
  }
}
